#include "../common/desktop_style.hpp"
#include "../common/qt_helpers.hpp"
#include "../common/single_instance.hpp"
#include <QAbstractSocket>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QNetworkInterface>
#include <QProcess>
#include <QProcessEnvironment>
#include <QProgressBar>
#include <QPushButton>
#include <QQueue>
#include <QSaveFile>
#include <QScrollArea>
#include <QSet>
#include <QShortcut>
#include <QStandardPaths>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <functional>

namespace {
QString dataRoot() {
  return qEnvironmentVariable(
      "WETYPE_DATA_ROOT",
      QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
          QStringLiteral("/fcitx5-wetypex"));
}
QString transferAsset(const QString &directory, const QString &name) {
  return QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                QStringLiteral("fcitx5-wetypex/ui/") +
                                    directory + QLatin1Char('/') + name);
}

QString localAddress() {
  QString defaultInterface;
  QFile routes(QStringLiteral("/proc/net/route"));
  if (routes.open(QIODevice::ReadOnly)) {
    for (const auto &line : routes.readAll().split('\n')) {
      const auto columns = line.simplified().split(' ');
      if (columns.size() >= 4 && columns[1] == "00000000" &&
          (columns[3].toUInt(nullptr, 16) & 2)) {
        defaultInterface = QString::fromLocal8Bit(columns[0]);
        break;
      }
    }
  }
  auto virtualInterface = [](const QString &name) {
    for (const auto &prefix :
         {QStringLiteral("docker"), QStringLiteral("veth"),
          QStringLiteral("virbr"), QStringLiteral("vmnet"),
          QStringLiteral("br-"), QStringLiteral("tun"), QStringLiteral("tap")})
      if (name.startsWith(prefix))
        return true;
    return false;
  };
  QString fallback;
  QString privateAddress;
  for (const auto &interface : QNetworkInterface::allInterfaces()) {
    if (!(interface.flags() & QNetworkInterface::IsUp) ||
        (interface.flags() & QNetworkInterface::IsLoopBack))
      continue;
    for (const auto &entry : interface.addressEntries()) {
      if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol)
        continue;
      const QString address = entry.ip().toString();
      if (address.startsWith(QStringLiteral("169.254.")))
        continue;
      if (interface.name() == defaultInterface &&
          !virtualInterface(interface.name()))
        return address;
      if (privateAddress.isEmpty() && !virtualInterface(interface.name()) &&
          (address.startsWith(QStringLiteral("10.")) ||
           address.startsWith(QStringLiteral("192.168.")) ||
           address.startsWith(QStringLiteral("172."))))
        privateAddress = address;
      if (fallback.isEmpty())
        fallback = address;
    }
  }
  return privateAddress.isEmpty() ? fallback : privateAddress;
}

bool readVarint(const QByteArray &data, qsizetype &offset, quint64 &value) {
  value = 0;
  for (unsigned shift = 0; shift < 64 && offset < data.size(); shift += 7) {
    const quint8 byte = quint8(data.at(offset++));
    value |= quint64(byte & 0x7f) << shift;
    if (!(byte & 0x80))
      return true;
  }
  return false;
}

struct GrpcInfo {
  QString ip;
  int port = 0;
  QString ca;
  QString serverName;
};

void parseEndpoint(const QByteArray &data, GrpcInfo &info) {
  qsizetype offset = 0;
  while (offset < data.size()) {
    quint64 tag = 0;
    if (!readVarint(data, offset, tag))
      return;
    const int field = int(tag >> 3), wire = int(tag & 7);
    if (wire == 0) {
      quint64 value = 0;
      if (!readVarint(data, offset, value))
        return;
      if (field == 2 && !info.port)
        info.port = int(value);
    } else if (wire == 2) {
      quint64 size = 0;
      if (!readVarint(data, offset, size) ||
          size > quint64(data.size() - offset))
        return;
      const QByteArray value = data.mid(offset, qsizetype(size));
      offset += qsizetype(size);
      if (field == 1 && info.ip.isEmpty())
        info.ip = QString::fromUtf8(value);
    } else {
      return;
    }
  }
}

void parseAvailableNets(const QByteArray &data, GrpcInfo &info) {
  qsizetype offset = 0;
  while (offset < data.size()) {
    quint64 tag = 0, size = 0;
    if (!readVarint(data, offset, tag) || (tag & 7) != 2 ||
        !readVarint(data, offset, size) || size > quint64(data.size() - offset))
      return;
    if ((tag >> 3) == 2)
      parseEndpoint(data.mid(offset, qsizetype(size)), info);
    offset += qsizetype(size);
  }
}

GrpcInfo parseGrpcInfo(const QByteArray &data) {
  GrpcInfo info;
  qsizetype offset = 0;
  while (offset < data.size()) {
    quint64 tag = 0;
    if (!readVarint(data, offset, tag))
      break;
    const int field = int(tag >> 3), wire = int(tag & 7);
    if (wire == 0) {
      quint64 value = 0;
      if (!readVarint(data, offset, value))
        break;
      if (field == 2)
        info.port = int(value);
    } else if (wire == 2) {
      quint64 size = 0;
      if (!readVarint(data, offset, size) ||
          size > quint64(data.size() - offset))
        break;
      const QByteArray value = data.mid(offset, qsizetype(size));
      offset += qsizetype(size);
      if (field == 1)
        info.ip = QString::fromUtf8(value);
      else if (field == 3)
        info.ca = QString::fromUtf8(value);
      else if (field == 4)
        info.serverName = QString::fromUtf8(value);
      else if (field == 9)
        parseAvailableNets(value, info);
    } else {
      break;
    }
  }
  return info;
}
} // namespace

class TransferWindow final : public QWidget {
public:
  TransferWindow() {
    setWindowTitle(QStringLiteral("WeTypeX · 文件传输"));
    setMinimumSize(520, 360);
    resize(900, 720);
    setAcceptDrops(true);
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *body = new QWidget;
    body->setMinimumSize(650, 600);
    scroll->setWidget(body);
    outer->addWidget(scroll);
    auto *layout = new QVBoxLayout(body);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);
    title_ = new QLabel("文件传输");
    title_->setObjectName("title");
    layout->addWidget(title_);
    auto *hint = new QLabel("选择设备和文件，再发起连接。连接期间，对端发送的文"
                            "件将保存到接收目录。");
    hint->setWordWrap(true);
    hint->setObjectName("hint");
    layout->addWidget(hint);
    auto *devices = new QHBoxLayout;
    peers_ = new QComboBox;
    peers_->setObjectName("peers");
    peers_->setMinimumWidth(220);
    peers_->addItem("请刷新关联设备", QString());
    refresh_ = new QPushButton("刷新设备");
    refresh_->setObjectName("refresh");
    auto *settings = new QPushButton("设置 / 关联设备");
    devices->addWidget(peers_, 1);
    devices->addWidget(refresh_);
    devices->addWidget(settings);
    layout->addLayout(devices);
    files_ = new QListWidget;
    files_->setObjectName("files");
    files_->setMinimumHeight(100);
    files_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    layout->addWidget(
        new QLabel("待发送文件 · 支持拖放，选择文件不会立即发送"));
    layout->addWidget(files_, 1);
    auto *selection = new QHBoxLayout;
    auto *choose = new QPushButton("选择文件…");
    choose->setObjectName("choose");
    auto *remove = new QPushButton("移除选中");
    send_ = new QPushButton("连接并发送");
    send_->setObjectName("send");
    selection->addWidget(choose);
    selection->addWidget(remove);
    selection->addStretch();
    selection->addWidget(send_);
    layout->addLayout(selection);
    auto *connection = new QHBoxLayout;
    connect_ = new QPushButton("连接 / 接收文件");
    connect_->setObjectName("connect");
    auto *scan = new QPushButton("扫码连接");
    scan->setObjectName("scan");
    scan_ = scan;
    stop_ = new QPushButton("断开并停止全部任务");
    stop_->setEnabled(false);
    stop_->setObjectName("stop");
    connection->addWidget(connect_);
    connection->addWidget(scan);
    connection->addStretch();
    connection->addWidget(stop_);
    layout->addLayout(connection);
    qr_ = new QLabel;
    qr_->setAlignment(Qt::AlignCenter);
    qr_->hide();
    layout->addWidget(qr_);
    status_ = new QLabel("尚未连接。设备需已关联，并能建立传输连接。");
    status_->setObjectName("status");
    status_->setWordWrap(true);
    status_->setTextFormat(Qt::PlainText);
    layout->addWidget(status_);
    progress_ = new QProgressBar;
    progress_->hide();
    progress_->setRange(0, 1000);
    layout->addWidget(progress_);
    tasks_ = new QTreeWidget;
    tasks_->setObjectName("tasks");
    tasks_->setColumnCount(3);
    tasks_->setHeaderLabels({"方向 / 文件", "进度", "结果"});
    tasks_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    tasks_->setMinimumHeight(150);
    layout->addWidget(tasks_, 1);
    downloads_ = qEnvironmentVariable(
        "WETYPE_TRANSFER_DOWNLOAD_DIR",
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) +
            "/WeTypeX");
    auto *destination = new QHBoxLayout;
    auto *directory = new QLabel("接收目录：" + downloads_);
    directory->setTextFormat(Qt::PlainText);
    directory->setWordWrap(true);
    auto *open = new QPushButton("打开接收目录");
    destination->addWidget(directory, 1);
    destination->addWidget(open);
    layout->addLayout(destination);
    connect(open, &QPushButton::clicked, this, [this] {
      if (QDir(downloads_).exists())
        QDesktopServices::openUrl(QUrl::fromLocalFile(downloads_));
      else
        status_->setText("尚无接收目录，首次连接时会创建。");
    });
    connect(settings, &QPushButton::clicked, this, [] {
      QProcess::startDetached(QStringLiteral(WETYPE_SETTINGS), {"--page", "5"});
    });
    connect(choose, &QPushButton::clicked, this, [this] {
      addFiles(QFileDialog::getOpenFileNames(this, "选择要发送的文件"));
    });
    connect(remove, &QPushButton::clicked, this, [this] {
      qDeleteAll(files_->selectedItems());
      updateActions();
    });
    connect(peers_, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this] { updateActions(); });
    connect(refresh_, &QPushButton::clicked, this, [this] {
      refresh_->setEnabled(false);
      runAccount({"group-info"}, [this](const QJsonObject &result) {
        refresh_->setEnabled(!active_);
        if (!result.value("ok").toBool()) {
          status_->setText(result.value("failure_stage") == "runtime"
              ? "本机账户核心依赖缺失或无法加载，请检查安装；无需重新关联设备。"
              : "设备列表获取失败，请检查账户工具与网络后重试。");
          return;
        }
        peers_->clear();
        const auto self = wetype::jsonInteger(result.value("self_uin"));
        for (const auto &value : result.value("devices").toArray()) {
          const auto device = value.toObject();
          const auto uin = wetype::jsonInteger(device.value("uin"));
          if (uin <= 0 || uin == self)
            continue;
          peers_->addItem(device.value("name").toString("关联设备"),
                          QString::number(uin));
        }
        if (!peers_->count()) {
          peers_->addItem("尚无关联设备", QString());
          status_->setText("请先在设置中关联设备，再刷新。");
        } else
          status_->setText("已读取关联设备；列表可见不代表对端在线。");
        updateActions();
      });
    });
    connect(connect_, &QPushButton::clicked, this,
            [this] { begin(false, false); });
    connect(scan, &QPushButton::clicked, this, [this] { begin(true, false); });
    connect(send_, &QPushButton::clicked, this, [this] {
      if (!active_)
        begin(false, true);
      else if (connected_)
        queueSelection();
    });
    connect(stop_, &QPushButton::clicked, this, [this] {
      if (QMessageBox::question(
              this, "停止连接",
              "停止当前连接及所有收发任务？未完成的文件不能视为完整文件。") ==
          QMessageBox::Yes)
        stopSession("已停止连接；未完成任务未确认成功。");
    });
    poll_.setInterval(250);
    connect(&poll_, &QTimer::timeout, this, [this] { pollPeer(); });
    activity_.setSingleShot(true);
    connect(&activity_, &QTimer::timeout, this, [this] {
      stopSession("传输长时间没有进展，已停止；请检查对端后重试。");
    });
    deadline_.setSingleShot(true);
    connect(&deadline_, &QTimer::timeout, this,
            [this] { stopSession("连接超时，请确认对端可用后重新连接。"); });
    connect(&session_, &QProcess::readyReadStandardOutput, this,
            [this] { readSessionEvents(); });
    connect(&session_, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError error) {
              if (error == QProcess::FailedToStart)
                stopSession("传输核心无法启动，请检查运行时安装。");
            });
    connect(&session_,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int, QProcess::ExitStatus) {
              if (active_ && !restarting_)
                stopSession("传输进程已退出；未完成任务未确认成功。");
              updateActions();
            });
    auto *pick = new QShortcut(QKeySequence("Ctrl+O"), this);
    connect(pick, &QShortcut::activated, choose, &QPushButton::click);
    auto *submit = new QShortcut(QKeySequence("Ctrl+Return"), this);
    connect(submit, &QShortcut::activated, send_, &QPushButton::click);
    updateActions();
  }

  ~TransferWindow() override {
    if (session_.state() != QProcess::NotRunning) {
      sendSession({{QStringLiteral("action"), QStringLiteral("quit")}});
      session_.waitForFinished(500);
    }
  }

private:
  void updateActions() {
    const bool idle = !active_ && session_.state() == QProcess::NotRunning;
    const bool peer = !peers_->currentData().toString().isEmpty();
    peers_->setEnabled(idle);
    refresh_->setEnabled(idle && !accountBusy_);
    connect_->setEnabled(idle && peer && !accountBusy_);
    scan_->setEnabled(idle && !accountBusy_);
    send_->setEnabled(files_->count() &&
                      ((idle && peer && !accountBusy_) || connected_));
    send_->setText(connected_ ? "发送所选文件" : "连接并发送");
    stop_->setEnabled(active_);
  }
  void addFiles(const QStringList &paths) {
    QSet<QString> known;
    for (int i = 0; i < files_->count(); ++i)
      known.insert(files_->item(i)->data(Qt::UserRole).toString());
    for (const auto &path : paths) {
      const QFileInfo info(path);
      if (!info.isFile() || !info.isReadable() ||
          known.contains(info.canonicalFilePath()) || files_->count() >= 100)
        continue;
      auto *item = new QListWidgetItem(info.fileName() + " · " +
                                           QString::number(info.size()) + " B",
                                       files_);
      item->setData(Qt::UserRole, info.canonicalFilePath());
      item->setToolTip(info.canonicalFilePath());
      known.insert(info.canonicalFilePath());
    }
    updateActions();
  }
  void begin(bool qr, bool sendAfterConnect) {
    if (active_ || session_.state() != QProcess::NotRunning || accountBusy_)
      return;
    boundUin_ = qr ? QString() : peers_->currentData().toString();
    if (!qr && boundUin_.isEmpty())
      return;
    active_ = true;
    connected_ = false;
    ++generation_;
    transferCode_.clear();
    sessionOutput_.clear();
    outgoing_.clear();
    currentUpload_.clear();
    if (sendAfterConnect)
      captureSelection();
    status_->setText("正在初始化连接…");
    qr_->clear();
    qr_->setVisible(qr);
    deadline_.start(180000);
    updateActions();
    startSession();
  }
  void captureSelection() {
    for (int i = 0; i < files_->count(); ++i) {
      const auto path = files_->item(i)->data(Qt::UserRole).toString();
      if (path != currentUpload_ && !outgoing_.contains(path))
        outgoing_.enqueue(path);
    }
    files_->clear();
  }
  void queueSelection() {
    captureSelection();
    sendNext();
    updateActions();
  }
  void sendNext() {
    if (!active_ || !connected_ || !currentUpload_.isEmpty() ||
        outgoing_.isEmpty())
      return;
    currentUpload_ = outgoing_.dequeue();
    activity_.start(120000);
    QFileInfo info(currentUpload_);
    if (!info.isFile() || !info.isReadable()) {
      recordTask(
          {{"event", "read_error"}, {"path", currentUpload_}, {"status", 1}});
      currentUpload_.clear();
      QTimer::singleShot(0, this, [this] { sendNext(); });
      return;
    }
    // Commands are serialized; only an actual backend completion releases the
    // next file.
    recordTask({{"event", "upload_start"}, {"path", currentUpload_}});
    sendSession({{"action", "upload"}, {"path", currentUpload_}});
  }
  void stopSession(const QString &reason) {
    active_ = false;
    connected_ = false;
    ++generation_;
    poll_.stop();
    deadline_.stop();
    activity_.stop();
    QStringList retry = outgoing_;
    if (!currentUpload_.isEmpty())
      retry.prepend(currentUpload_);
    addFiles(retry);
    outgoing_.clear();
    currentUpload_.clear();
    transferCode_.clear();
    qr_->hide();
    for (auto *row : taskRows_)
      if (!row->data(0, Qt::UserRole).toBool()) {
        row->setText(2, "已中断 / 未确认完成");
        row->setData(0, Qt::UserRole, true);
      }
    if (session_.state() != QProcess::NotRunning) {
      sendSession({{"action", "quit"}});
      QTimer::singleShot(1000, &session_, [this] {
        if (!active_ && session_.state() != QProcess::NotRunning)
          session_.kill();
      });
    }
    if (accountProcess_)
      accountProcess_->kill();
    progress_->hide();
    status_->setText(reason);
    updateActions();
  }
  void recordTask(const QJsonObject &event) {
    const auto type = event.value("event").toString();
    const auto path = event.value("path").toString();
    if (path.isEmpty())
      return;
    const bool upload = type.startsWith("upload") || type == "read_error";
    const auto key = QString(upload ? "send:" : "receive:") + path;
    auto *row = taskRows_.value(key, nullptr);
    if (!row ||
        (row->data(0, Qt::UserRole).toBool() && type.endsWith("_start"))) {
      if (tasks_->topLevelItemCount() >= 200) {
        for (int i = 0; i < tasks_->topLevelItemCount(); ++i) {
          auto *old = tasks_->topLevelItem(i);
          if (!old->data(0, Qt::UserRole).toBool())
            continue;
          for (auto it = taskRows_.begin(); it != taskRows_.end();) {
            if (it.value() == old)
              it = taskRows_.erase(it);
            else
              ++it;
          }
          delete tasks_->takeTopLevelItem(i);
          break;
        }
      }
      if (tasks_->topLevelItemCount() >= 200) {
        stopSession("活动任务过多，已停止连接；请分批传输。");
        return;
      }
      row =
          new QTreeWidgetItem(tasks_, {QString(upload ? "发送 · " : "接收 · ") +
                                           QFileInfo(path).fileName(),
                                       "", "处理中"});
      taskRows_[key] = row;
    }
    if (row->data(0, Qt::UserRole).toBool())
      return;
    if (type.endsWith("_progress")) {
      const auto total = wetype::jsonInteger(event.value("total"));
      const auto done = wetype::jsonInteger(event.value("transferred"));
      row->setText(
          1, total > 0
                 ? QString::number(qBound(
                       0, int(100.0 * double(done) / double(total)), 100)) +
                       "%"
                 : "传输中");
    } else if (type.endsWith("_done") || type == "read_error") {
      const bool success =
          event.value("status").toInt(-1) == 0 && type != "read_error";
      const auto info = QFileInfo(path);
      const auto expected = wetype::jsonInteger(event.value("total"), -1);
      const auto root = QDir(downloads_).canonicalPath();
      const bool saved = success && !upload && expected >= 0 &&
                         !root.isEmpty() && info.isFile() &&
                         info.size() == expected &&
                         info.canonicalFilePath().startsWith(root + '/');
      row->setText(2, !success ? "失败，可重新选择文件重试"
                      : upload ? "发送端报告完成"
                      : saved  ? "已落盘（大小已核对）"
                               : "对端报告完成，落盘未核实");
      if (saved)
        row->setToolTip(2, info.canonicalFilePath());
      row->setData(0, Qt::UserRole, true);
    }
  }

protected:
  void closeEvent(QCloseEvent *event) override {
    if (active_ &&
        QMessageBox::question(this, "关闭传输",
                              "关闭将断开连接并停止所有收发任务，继续？") !=
            QMessageBox::Yes) {
      event->ignore();
      return;
    }
    stopSession("已关闭");
    event->accept();
  }
  void dragEnterEvent(QDragEnterEvent *event) override {
    if (event->mimeData()->hasUrls())
      event->acceptProposedAction();
  }
  void dropEvent(QDropEvent *event) override {
    QStringList paths;
    for (const auto &url : event->mimeData()->urls())
      if (url.isLocalFile())
        paths << url.toLocalFile();
    addFiles(paths);
    event->acceptProposedAction();
  }

private:
  void startSession() {
    const QString root = dataRoot();
    const QString runtime = qEnvironmentVariable(
        "WETYPE_FLURRY_RUNTIME", root + QStringLiteral("/runtime/flurry"));
    const QString host = qEnvironmentVariable(
        "WETYPE_ENGINE_HOST", QStringLiteral(WETYPE_ENGINE_HOST));
    const QString state = root + QStringLiteral("/state/transfer");
    const QString downloads = qEnvironmentVariable(
        "WETYPE_TRANSFER_DOWNLOAD_DIR",
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) +
            QStringLiteral("/WeTypeX"));
    if (!QDir().mkpath(state + QStringLiteral("/parts")) ||
        !QDir().mkpath(downloads)) {
      stopSession("无法创建传输缓存或接收目录，请检查权限。");
      return;
    }
    wxpStarted_ = false;
    grpcAdvertised_ = false;
    lastDispatch_.clear();
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(
        QStringLiteral("WETYPE_SUPPORT_DIR"),
        qEnvironmentVariable("WETYPE_SUPPORT_DIR",
                             QStringLiteral(WETYPE_SUPPORT_DIR)));
    environment.insert(QStringLiteral("WETYPE_FLURRY_DOWNLOAD_DIR"), downloads);
    environment.insert(QStringLiteral("WETYPE_FLURRY_TEMP_DIR"),
                       state + QStringLiteral("/parts"));
    session_.setProcessEnvironment(environment);
    session_.setStandardErrorFile(
        qEnvironmentVariable("WETYPE_TRANSFER_LOG",
                             QStringLiteral("/dev/null")),
        QIODevice::Append);
    const QString wxp2p =
        QFileInfo(runtime).absolutePath() + QStringLiteral("/wxp2p");
    preloadedWxp2p_ =
        QFileInfo::exists(wxp2p + QStringLiteral("/manifest.txt"));
    if (preloadedWxp2p_) {
      environment.insert(QStringLiteral("WETYPE_AUX_RUNTIME"), wxp2p);
      environment.remove(QStringLiteral("WETYPE_WXP2P_DISPATCH_FILE"));
      session_.setProcessEnvironment(environment);
      session_.start(host, {runtime, QStringLiteral("flurry-wxp2p-server")});
    } else {
      session_.start(host, {runtime, QStringLiteral("flurry-server")});
    }
  }

  void startWxp2p(const QByteArray &dispatch, const QJsonObject &peer) {
    if (dispatch.isEmpty() || dispatch == lastDispatch_)
      return;
    lastDispatch_ = dispatch;
    if (wxpStarted_)
      grpcAdvertised_ = false;
    const QString root = dataRoot();
    const QString state = root + QStringLiteral("/state/transfer");
    const QString path = state + QStringLiteral("/wxp2p-dispatch.bin");
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) ||
        file.write(dispatch) != dispatch.size() || !file.commit()) {
      status_->setText(QStringLiteral("无法保存手机传输调度信息"));
      return;
    }
    QFile::setPermissions(path,
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    const QString peerCaPath = state + QStringLiteral("/peer-ca.pem");
    const QByteArray peerCa = QByteArray::fromBase64(
        peer.value(QStringLiteral("public_cer")).toString().toLatin1());
    QSaveFile peerCaFile(peerCaPath);
    if (peerCa.isEmpty() || !peerCaFile.open(QIODevice::WriteOnly) ||
        peerCaFile.write(peerCa) != peerCa.size() || !peerCaFile.commit()) {
      status_->setText(QStringLiteral("无法保存手机安全凭据"));
      return;
    }
    QFile::setPermissions(peerCaPath,
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    if (preloadedWxp2p_ && !wxpStarted_ &&
        session_.state() == QProcess::Running) {
      wxpStarted_ = true;
      status_->setText(QStringLiteral("正在通过安全直连或中继连接手机…"));
      sendSession({{QStringLiteral("action"), QStringLiteral("wxp2p_start")},
                   {QStringLiteral("dispatch_path"), path},
                   {QStringLiteral("remote_ca_file"), peerCaPath}});
      return;
    }
    restarting_ = true;
    if (session_.state() != QProcess::NotRunning) {
      sendSession({{QStringLiteral("action"), QStringLiteral("quit")}});
      if (!session_.waitForFinished(1000)) {
        session_.kill();
        session_.waitForFinished(1000);
      }
    }
    sessionOutput_.clear();
    const QString runtime = qEnvironmentVariable(
        "WETYPE_FLURRY_RUNTIME", root + QStringLiteral("/runtime/flurry"));
    const QString host = qEnvironmentVariable(
        "WETYPE_ENGINE_HOST", QStringLiteral(WETYPE_ENGINE_HOST));
    auto environment = session_.processEnvironment();
    environment.insert(QStringLiteral("WETYPE_AUX_RUNTIME"),
                       QFileInfo(runtime).absolutePath() +
                           QStringLiteral("/wxp2p"));
    environment.insert(QStringLiteral("WETYPE_WXP2P_DISPATCH_FILE"), path);
    environment.insert(QStringLiteral("WETYPE_FLURRY_REMOTE_CA_FILE"),
                       peerCaPath);
    session_.setProcessEnvironment(environment);
    status_->setText(QStringLiteral("正在通过安全直连或中继连接手机…"));
    wxpStarted_ = true;
    session_.start(host, {runtime, QStringLiteral("flurry-wxp2p-server")});
    restarting_ = false;
  }

  void readSessionEvents() {
    sessionOutput_ += session_.readAllStandardOutput();
    if (!active_) {
      sessionOutput_.clear();
      return;
    }
    if (sessionOutput_.size() > 4 * 1024 * 1024) {
      stopSession("传输核心返回过大的消息，连接已停止。");
      return;
    }
    qsizetype newline = -1;
    while ((newline = sessionOutput_.indexOf('\n')) >= 0) {
      if (!active_) {
        sessionOutput_.clear();
        break;
      }
      const QByteArray line = sessionOutput_.left(newline);
      sessionOutput_.remove(0, newline + 1);
      const QJsonObject event = QJsonDocument::fromJson(line).object();
      const QString type = event.value(QStringLiteral("event")).toString();
      recordTask(event);
      if (!active_)
        break;
      if (type.endsWith("_progress") || type.endsWith("_start"))
        activity_.start(120000);
      if (type.endsWith("_done") || type == "read_error") {
        bool pending = false;
        for (auto *row : taskRows_)
          pending |= !row->data(0, Qt::UserRole).toBool();
        if (!pending)
          activity_.stop();
      }
      if (type == QStringLiteral("ready")) {
        preloadedWxp2p_ = event.value(QStringLiteral("transport")).toString() ==
                          QStringLiteral("wxp2p");
        requestCode(event);
      } else if (type == QStringLiteral("grpc_ready")) {
        advertiseGrpc(event);
      } else if (type == QStringLiteral("role_started")) {
        status_->setText(QStringLiteral("已找到设备，正在建立安全连接…"));
      } else if (type == QStringLiteral("connected")) {
        poll_.stop();
        deadline_.stop();
        connected_ = true;
        qr_->hide();
        status_->setText("已连接；对端发送的文件会自动保存到下方接收目录。");
        updateActions();
        sendNext();
      } else if (type == QStringLiteral("disconnected")) {
        stopSession("设备连接已断开，请重新连接。");
      } else if (type == QStringLiteral("connect_end")) {
        status_->setText(QStringLiteral("未能连接设备，正在等待手机重试…"));
        connected_ = false;
        updateActions();
        if (!transferCode_.isEmpty())
          poll_.start();
      } else if (type.endsWith(QStringLiteral("_progress"))) {
        const qint64 done =
            wetype::jsonInteger(event.value(QStringLiteral("transferred")));
        const qint64 total =
            wetype::jsonInteger(event.value(QStringLiteral("total")));
        progress_->show();
        progress_->setValue(
            total > 0
                ? qBound(0, int(1000.0 * double(done) / double(total)), 1000)
                : 0);
        status_->setText(type.startsWith(QStringLiteral("upload"))
                             ? QStringLiteral("正在发送文件…")
                             : QStringLiteral("正在接收文件…"));
      } else if (type.endsWith(QStringLiteral("_done")) ||
                 type == "read_error") {
        status_->setText("任务结果已更新，请查看下方列表。");
        progress_->hide();
        if ((type == "upload_done" || type == "read_error") &&
            event.value("path").toString() == currentUpload_) {
          currentUpload_.clear();
          sendNext();
        }
      } else if (type == QStringLiteral("start_failed")) {
        stopSession(QStringLiteral("无法建立安全传输连接，请检查网络后重试。"));
      }
    }
  }

  void requestCode(const QJsonObject &identity) {
    const bool wxp2p = identity.value(QStringLiteral("transport")).toString() ==
                       QStringLiteral("wxp2p");
    if (wxp2p) {
      const QString path = dataRoot() + QStringLiteral("/state/p2p-info.json");
      QDir().mkpath(QFileInfo(path).absolutePath());
      QSaveFile file(path);
      const QJsonObject info{{QStringLiteral("ip"), localAddress()},
                             {QStringLiteral("ca_cert"),
                              identity.value(QStringLiteral("ca_cert"))},
                             {QStringLiteral("server_name"),
                              identity.value(QStringLiteral("server_name"))}};
      if (!file.open(QIODevice::WriteOnly) ||
          file.write(QJsonDocument(info).toJson(QJsonDocument::Compact)) < 0 ||
          !file.commit()) {
        status_->setText(QStringLiteral("无法保存传输会话"));
        return;
      }
      QFile::setPermissions(path,
                            QFileDevice::ReadOwner | QFileDevice::WriteOwner);
      const QStringList accountArguments =
          boundUin_.isEmpty()
              ? QStringList{QStringLiteral("p2p-code"), path}
              : QStringList{QStringLiteral("p2p-init"), boundUin_, path};
      requestCodeFromAccount(accountArguments);
      return;
    }
    const QString ip = localAddress();
    if (ip.isEmpty()) {
      status_->setText(QStringLiteral("未找到可用于设备直连的局域网地址"));
      return;
    }
    const QString path = dataRoot() + QStringLiteral("/state/p2p-info.json");
    QDir().mkpath(QFileInfo(path).absolutePath());
    QJsonObject info{
        {QStringLiteral("ip"), ip},
        {QStringLiteral("port"), identity.value(QStringLiteral("port"))},
        {QStringLiteral("ca_cert"), identity.value(QStringLiteral("ca_cert"))},
        {QStringLiteral("server_name"),
         identity.value(QStringLiteral("server_name"))}};
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) ||
        file.write(QJsonDocument(info).toJson(QJsonDocument::Compact)) < 0 ||
        !file.commit()) {
      status_->setText(QStringLiteral("无法保存传输会话"));
      return;
    }
    QFile::setPermissions(path,
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    const QStringList accountArguments =
        boundUin_.isEmpty()
            ? QStringList{QStringLiteral("p2p-code"), path}
            : QStringList{QStringLiteral("p2p-init"), boundUin_, path};
    requestCodeFromAccount(accountArguments);
  }

  void requestCodeFromAccount(const QStringList &accountArguments) {
    runAccount(accountArguments, [this](const QJsonObject &result) {
      const QByteArray png = QByteArray::fromBase64(
          result.value(QStringLiteral("transfer_image")).toString().toLatin1());
      QPixmap image;
      transferCode_ = result.value(QStringLiteral("transfer_code")).toString();
      lastDispatch_.clear();
      if (!result.value(QStringLiteral("ok")).toBool() ||
          transferCode_.isEmpty() ||
          (boundUin_.isEmpty() && !image.loadFromData(png, "PNG"))) {
        stopSession(
            QStringLiteral("无法创建传输邀请，请检查配对和网络状态后重试。"));
        return;
      }
      if (boundUin_.isEmpty())
        qr_->setPixmap(image.scaled(286, 286, Qt::KeepAspectRatio,
                                    Qt::SmoothTransformation));
      const int minutes =
          qMax(1, result.value(QStringLiteral("expiration")).toInt() / 60000);
      status_->setText(
          boundUin_.isEmpty()
              ? QStringLiteral("请使用手机扫描；二维码将在 %1 分钟后失效")
                    .arg(minutes)
              : QStringLiteral("已发送邀请；邀请将在 %1 分钟后失效")
                    .arg(minutes));
      deadline_.start(
          qBound(10000, result.value("expiration").toInt(180000), 300000));
      poll_.start();
    });
  }

  void pollPeer() {
    if (!active_ || accountBusy_ || transferCode_.isEmpty())
      return;
    runAccount(
        {QStringLiteral("p2p-peer"), transferCode_},
        [this](const QJsonObject &result) {
          const QJsonObject peer =
              result.value(QStringLiteral("peer")).toObject();
          if (!result.value(QStringLiteral("ok")).toBool())
            return;
          const QByteArray dispatch = QByteArray::fromBase64(
              result.value(QStringLiteral("dispatch_buf"))
                  .toString()
                  .toLatin1());
          if (!dispatch.isEmpty()) {
            startWxp2p(dispatch, peer);
            return;
          }
          if (!peer.value(QStringLiteral("ready")).toBool())
            return;
          const auto grpc = parseGrpcInfo(
              QByteArray::fromBase64(peer.value(QStringLiteral("grpc_net_info"))
                                         .toString()
                                         .toLatin1()));
          if (grpc.ca.isEmpty())
            return;
          poll_.stop();
          const QString caPath =
              dataRoot() + QStringLiteral("/state/transfer/peer-ca.pem");
          QSaveFile ca(caPath);
          if (!ca.open(QIODevice::WriteOnly) ||
              ca.write(grpc.ca.toUtf8()) < 0 || !ca.commit()) {
            status_->setText(QStringLiteral("无法保存对端安全凭据"));
            return;
          }
          QFile::setPermissions(caPath, QFileDevice::ReadOwner |
                                            QFileDevice::WriteOwner);
          sendSession({{QStringLiteral("action"), QStringLiteral("server")},
                       {QStringLiteral("remote_ca_file"), caPath}});
        });
  }

  void advertiseGrpc(const QJsonObject &identity) {
    if (grpcAdvertised_ || transferCode_.isEmpty())
      return;
    const QString ip = localAddress();
    const int port = identity.value(QStringLiteral("port")).toInt();
    if (ip.isEmpty() || port <= 0) {
      status_->setText(QStringLiteral("无法发布手机传输端点"));
      return;
    }
    grpcAdvertised_ = true;
    poll_.stop();
    const QString path =
        dataRoot() + QStringLiteral("/state/transfer/p2p-update.json");
    QSaveFile file(path);
    const QJsonObject info{
        {QStringLiteral("ip"), ip},
        {QStringLiteral("port"), port},
        {QStringLiteral("ca_cert"), identity.value(QStringLiteral("ca_cert"))},
        {QStringLiteral("server_name"),
         identity.value(QStringLiteral("server_name"))},
        {QStringLiteral("preferred_client"), false}};
    if (!file.open(QIODevice::WriteOnly) ||
        file.write(QJsonDocument(info).toJson(QJsonDocument::Compact)) < 0 ||
        !file.commit()) {
      status_->setText(QStringLiteral("无法保存传输端点"));
      grpcAdvertised_ = false;
      poll_.start();
      return;
    }
    QFile::setPermissions(path,
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    runAccount(
        {QStringLiteral("p2p-update"), transferCode_, path},
        [this](const QJsonObject &result) {
          if (!result.value(QStringLiteral("ok")).toBool()) {
            status_->setText(QStringLiteral("无法向手机发布安全传输端点"));
            grpcAdvertised_ = false;
            poll_.start();
            return;
          }
          status_->setText(QStringLiteral("已发布安全端点，正在连接手机…"));
          poll_.start();
        });
  }

  void runAccount(const QStringList &arguments,
                  std::function<void(const QJsonObject &)> callback) {
    if (accountBusy_)
      return;
    accountBusy_ = true;
    auto *process = new QProcess(this);
    accountProcess_ = process;
    const auto generation = generation_;
    auto *timeout = new QTimer(process);
    timeout->setSingleShot(true);
    connect(timeout, &QTimer::timeout, process, [process] { process->kill(); });
    timeout->start(45000);
    wetype::onProcessDone(
        process, this,
        [this, process, generation,
         callback = std::move(callback)](int code, QProcess::ExitStatus) {
          accountBusy_ = false;
          accountProcess_ = nullptr;
          QJsonObject result =
              QJsonDocument::fromJson(process->readAllStandardOutput())
                  .object();
          if (code != 0) result["ok"] = false;
          process->deleteLater();
          if (generation == generation_)
            callback(result);
          updateActions();
        });
    process->start(qEnvironmentVariable("WETYPE_ACCOUNT_TOOL",
                                        QStringLiteral(WETYPE_ACCOUNT_TOOL)),
                   arguments);
  }

  void sendSession(const QJsonObject &command) {
    if (session_.state() == QProcess::Running) {
      session_.write(QJsonDocument(command).toJson(QJsonDocument::Compact));
      session_.write("\n");
    }
  }

  QComboBox *peers_ = nullptr;
  QListWidget *files_ = nullptr;
  QTreeWidget *tasks_ = nullptr;
  QMap<QString, QTreeWidgetItem *> taskRows_;
  QPushButton *refresh_ = nullptr, *connect_ = nullptr, *scan_ = nullptr,
              *stop_ = nullptr;
  QProcess *accountProcess_ = nullptr;
  QTimer deadline_, activity_;
  QQueue<QString> outgoing_;
  QString currentUpload_, downloads_;
  quint64 generation_ = 0;
  bool active_ = false, connected_ = false, restarting_ = false;
  friend class TransferTests;
  QLabel *title_ = nullptr;
  QLabel *qr_ = nullptr;
  QLabel *status_ = nullptr;
  QProgressBar *progress_ = nullptr;
  QPushButton *send_ = nullptr;
  QProcess session_;
  QByteArray sessionOutput_;
  QTimer poll_;
  QString transferCode_;
  QString boundUin_;
  QByteArray lastDispatch_;
  bool accountBusy_ = false;
  bool preloadedWxp2p_ = false;
  bool wxpStarted_ = false;
  bool grpcAdvertised_ = false;
};

#ifndef WETYPE_GUI_TEST
int main(int argc, char **argv) {
  QApplication app(argc, argv);
  app.setApplicationName(QStringLiteral("fcitx5-wetypex-transfer"));
  app.setDesktopFileName(QStringLiteral("fcitx5-wetypex-transfer"));
  wetype::applyDesktopStyle(app);
  wetype::SingleInstance instance;
  if (!instance.acquire("transfer", app.arguments().contains("--activate")))
    return 0;
  TransferWindow window;
  instance.bind(window);
  window.show();
  return app.exec();
}

#endif
