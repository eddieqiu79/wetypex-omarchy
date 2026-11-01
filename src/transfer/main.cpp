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
#include <QLocale>
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
#include <QResizeEvent>
#include <QSet>
#include <QSignalBlocker>
#include <QShortcut>
#include <QStandardPaths>
#include <QStyle>
#include <QTabBar>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <functional>

namespace {
QLabel *transferLabel(const QString &text, const char *name = "") {
  auto *label = new QLabel(text);
  label->setObjectName(QString::fromLatin1(name));
  label->setTextFormat(Qt::PlainText);
  label->setWordWrap(true);
  return label;
}

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
    setWindowTitle(QStringLiteral("WeTypeX · 隔空投送"));
    setMinimumSize(520, 360);
    resize(920, 780);
    setAcceptDrops(true);
    setStyleSheet(R"(
      QWidget#transferBody { background:#15171b; }
      QFrame#transferCard { background:#1e2127; border:1px solid #373d47; border-radius:12px; }
      QLabel#title { font-size:26px; font-weight:600; }
      QLabel#sectionTitle,QLabel#receiveTitle { font-size:17px; font-weight:600; }
      QLabel#hint,QLabel#summary { color:#a5adba; }
      QLabel#dropHint { background:#191c21; border:1px dashed #46505c; border-radius:10px; padding:20px; color:#c9d0da; }
      QPushButton { min-height:32px; border-radius:8px; padding:5px 12px; background:#292e37; border-color:#46505c; }
      QPushButton#send,QPushButton#acceptInvite { background:#94e4bf; color:#13271e; border-color:#94e4bf; font-weight:600; }
      QPushButton#send:disabled { background:#292e37; color:#939ba8; border-color:#46505c; }
      QPushButton#deviceCard { text-align:left; padding:14px; min-height:54px; }
      QPushButton#deviceCard:checked { background:#243a31; border:2px solid #83dfb5; }
      QPushButton:focus { border:2px solid #83dfb5; }
      QTabBar::tab { background:#292e37; color:#a5adba; padding:10px 20px; }
      QTabBar::tab:selected { background:#1e2127; color:#94e4bf; border-bottom:2px solid #94e4bf; }
      QListWidget#files,QTreeWidget#tasks { background:#1e2127; border:0; }
      QListWidget#files::item { padding:9px 4px; }
      QProgressBar { background:#373d47; color:#edf0f5; min-height:18px; border:0; border-radius:5px; }
      QProgressBar::chunk { background:#327e5f; border-radius:5px; }
    )");
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *body = new QWidget;
    body->setObjectName("transferBody");
    scroll->setWidget(body);
    outer->addWidget(scroll);
    auto *layout = new QVBoxLayout(body);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);
    auto *heading = new QHBoxLayout;
    title_ = transferLabel("隔空投送", "title");
    heading->addWidget(title_, 1);
    auto *help = new QPushButton("连接帮助");
    help->setObjectName("help");
    help->setCheckable(true);
    heading->addWidget(help);
    layout->addLayout(heading);
    layout->addWidget(transferLabel("选好文件，发送到你的另一台设备。", "hint"));
    tabs_ = new QTabBar;
    tabs_->setObjectName("transferTabs");
    tabs_->addTab("发送文件");
    tabs_->addTab("接收文件");
    tabs_->setExpanding(false);
    tabs_->setDrawBase(false);
    layout->addWidget(tabs_);
    auto *sendPage = new QWidget;
    sendPage_ = sendPage;
    layout->addWidget(sendPage);
    auto *sendLayout = new QVBoxLayout(sendPage);
    sendLayout->setContentsMargins(0, 0, 0, 0);
    sendLayout->setSpacing(18);
    selectionLayout_ = new QBoxLayout(QBoxLayout::LeftToRight);
    selectionLayout_->setSpacing(18);
    sendLayout->addLayout(selectionLayout_);
    auto card = [](QVBoxLayout *&contents) {
      auto *frame = new QFrame;
      frame->setObjectName("transferCard");
      contents = new QVBoxLayout(frame);
      contents->setContentsMargins(18, 18, 18, 18);
      contents->setSpacing(12);
      return frame;
    };
    QVBoxLayout *fileLayout;
    selectionLayout_->addWidget(card(fileLayout), 5);
    auto *fileHeading = new QHBoxLayout;
    fileHeading->addWidget(transferLabel("1  选择文件", "sectionTitle"), 1);
    fileLayout->addLayout(fileHeading);
    auto *drop = transferLabel("把文件拖到这里\n选择文件后，点击发送才会传输。", "dropHint");
    drop->setAlignment(Qt::AlignCenter);
    fileLayout->addWidget(drop);
    auto *choose = new QPushButton("选择文件…");
    choose->setObjectName("choose");
    choose->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    fileLayout->addWidget(choose);
    files_ = new QListWidget;
    files_->setObjectName("files");
    files_->setMinimumHeight(86);
    files_->setMaximumHeight(180);
    files_->setWordWrap(true);
    files_->setTextElideMode(Qt::ElideNone);
    files_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    fileLayout->addWidget(files_, 1);
    auto *remove = new QPushButton("移除选中");
    removeFiles_ = remove;
    remove->setObjectName("removeFiles");
    fileHeading->addWidget(remove);
    fileLayout->addStretch();
    QVBoxLayout *deviceLayout;
    selectionLayout_->addWidget(card(deviceLayout), 4);
    auto *deviceHeading = new QHBoxLayout;
    deviceHeading->addWidget(transferLabel("2  发给哪台设备", "sectionTitle"), 1);
    refresh_ = new QPushButton("刷新");
    refresh_->setObjectName("refresh");
    deviceHeading->addWidget(refresh_);
    deviceLayout->addLayout(deviceHeading);
    // Keep the existing selection model used by the invitation/connection flow.
    // The visible cards below mirror it; no second device identity is stored.
    peers_ = new QComboBox(this);
    peers_->setObjectName("peers");
    peers_->addItem("正在读取关联设备…", QString());
    peers_->hide();
    deviceCards_ = new QVBoxLayout;
    deviceLayout->addLayout(deviceCards_);
    deviceEmpty_ = transferLabel("正在读取关联设备…", "hint");
    deviceLayout->addWidget(deviceEmpty_);
    auto *settings = new QPushButton("关联设备");
    settings->setObjectName("pairDevices");
    deviceLayout->addWidget(settings);
    deviceLayout->addWidget(transferLabel("已关联不代表当前在线。发送时会确认设备是否可连接。", "hint"));
    deviceLayout->addStretch();
    auto *selection = new QHBoxLayout;
    summary_ = transferLabel("先添加要发送的文件", "summary");
    selection->addWidget(summary_, 1);
    send_ = new QPushButton("发送文件");
    send_->setObjectName("send");
    selection->addWidget(send_);
    sendLayout->addLayout(selection);
    auto *receivePage = new QWidget;
    receivePage_ = receivePage;
    layout->addWidget(receivePage);
    receivePage->hide();
    auto *receiveLayout = new QVBoxLayout(receivePage);
    receiveLayout->setContentsMargins(0, 0, 0, 0);
    QVBoxLayout *receiveCard;
    receiveLayout->addWidget(card(receiveCard));
    receiveTitle_ = transferLabel("等待其他设备发送", "receiveTitle");
    receiveTitle_->setAlignment(Qt::AlignCenter);
    receiveCard->addSpacing(20);
    receiveCard->addWidget(receiveTitle_);
    receiveHint_ = transferLabel("在 Mac 或手机的 WeType 中选择这台 Linux。\n收到邀请后，你可以选择接受或拒绝。", "hint");
    receiveHint_->setAlignment(Qt::AlignCenter);
    receiveCard->addWidget(receiveHint_);
    downloads_ = qEnvironmentVariable(
        "WETYPE_TRANSFER_DOWNLOAD_DIR",
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) +
            "/WeTypeX");
    auto *receivePath = transferLabel("接收位置：" + QDir::toNativeSeparators(downloads_), "hint");
    receivePath->setAlignment(Qt::AlignCenter);
    receiveCard->addWidget(receivePath);
    invitationActions_ = new QWidget;
    auto *inviteButtons = new QHBoxLayout(invitationActions_);
    inviteButtons->addStretch();
    rejectInvite_ = new QPushButton("拒绝");
    rejectInvite_->setObjectName("rejectInvite");
    acceptInvite_ = new QPushButton("接受并连接");
    acceptInvite_->setObjectName("acceptInvite");
    inviteButtons->addWidget(rejectInvite_);
    inviteButtons->addWidget(acceptInvite_);
    inviteButtons->addStretch();
    receiveCard->addWidget(invitationActions_);
    invitationActions_->hide();
    receiveCard->addSpacing(20);
    auto *sessionHeading = new QHBoxLayout;
    sessionTitle_ = transferLabel("准备就绪", "sectionTitle");
    sessionHeading->addWidget(sessionTitle_, 1);
    stop_ = new QPushButton("结束连接");
    stop_->setObjectName("stop");
    sessionHeading->addWidget(stop_);
    layout->addLayout(sessionHeading);
    status_ = transferLabel("保持窗口打开即可接收邀请；发送文件前请先选择设备。", "status");
    layout->addWidget(status_);
    progress_ = new QProgressBar;
    progress_->hide();
    progress_->setRange(0, 1000);
    layout->addWidget(progress_);
    tasks_ = new QTreeWidget;
    tasks_->setObjectName("tasks");
    tasks_->setColumnCount(3);
    tasks_->setHeaderLabels({"本次窗口的传输", "进度", "结果"});
    tasks_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    tasks_->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tasks_->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    tasks_->setRootIsDecorated(false);
    tasks_->setMinimumHeight(120);
    tasks_->setMaximumHeight(200);
    tasks_->hide();
    layout->addWidget(tasks_);
    helpPanel_ = new QWidget;
    auto *helpLayout = new QVBoxLayout(helpPanel_);
    helpLayout->setContentsMargins(0, 0, 0, 0);
    helpLayout->addWidget(transferLabel("没有找到设备？\n确认设备已关联，打开对端 WeType，并检查两台设备的网络，然后刷新设备列表。", "hint"));
    auto *connection = new QHBoxLayout;
    connect_ = new QPushButton("向所选设备发起接收连接");
    connect_->setObjectName("connect");
    auto *scan = new QPushButton("手机扫码连接");
    scan->setObjectName("scan");
    scan_ = scan;
    connection->addWidget(connect_);
    connection->addWidget(scan);
    helpLayout->addLayout(connection);
    layout->addWidget(helpPanel_);
    helpPanel_->hide();
    qr_ = new QLabel;
    qr_->setAlignment(Qt::AlignCenter);
    qr_->hide();
    layout->addWidget(qr_);
    auto *destination = new QHBoxLayout;
    destination->addWidget(transferLabel("窗口打开时可接收邀请\n保存到：" + QDir::toNativeSeparators(downloads_), "hint"), 1);
    auto *open = new QPushButton("打开接收文件夹");
    open->setObjectName("openDownloads");
    destination->addWidget(open);
    layout->addLayout(destination);
    layout->addStretch();
    connect(tabs_, &QTabBar::currentChanged, this, [this](int index) {
      sendPage_->setVisible(index == 0);
      receivePage_->setVisible(index == 1);
    });
    connect(help, &QPushButton::toggled, helpPanel_, &QWidget::setVisible);
    connect(acceptInvite_, &QPushButton::clicked, this, [this] { answerInvitation(true); });
    connect(rejectInvite_, &QPushButton::clicked, this, [this] { answerInvitation(false); });
    connect(files_, &QListWidget::itemSelectionChanged, this, [this, remove] {
      remove->setEnabled(!files_->selectedItems().isEmpty());
    });
    remove->setEnabled(false);
    connect(open, &QPushButton::clicked, this, [this] {
      if (QDir(downloads_).exists())
        QDesktopServices::openUrl(QUrl::fromLocalFile(downloads_));
      else {
        sessionEnded_ = true;
        status_->setText("尚无接收目录，首次连接时会创建。");
        updateActions();
      }
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
          devicesQueryFailed_ = true;
          status_->setText(result.value("failure_stage") == "runtime"
              ? "本机账户核心依赖缺失或无法加载，请检查安装；无需重新关联设备。"
              : "设备列表获取失败，请检查账户工具与网络后重试。");
          return;
        }
        updatePeerList(result);
        status_->setText(peers_->currentData().toString().isEmpty()
            ? "请先在设置中关联设备，再刷新。"
            : "已读取关联设备；列表可见不代表对端在线。");
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
      if (!connected_) {
        stopSession("已取消等待，待发送文件已保留。");
        return;
      }
      if (!hasPendingTasks() && outgoing_.isEmpty() && currentUpload_.isEmpty()) {
        stopSession("连接已结束，继续等待其他设备发送。");
        return;
      }
      QMessageBox prompt(QMessageBox::Question, "停止当前连接？",
          "所有收发任务都会停止。未完成的文件需要重新传输。",
          QMessageBox::NoButton, this);
      auto *keep = prompt.addButton("继续传输", QMessageBox::RejectRole);
      auto *stop = prompt.addButton("停止全部传输", QMessageBox::DestructiveRole);
      prompt.setDefaultButton(keep);
      prompt.exec();
      if (prompt.clickedButton() == stop)
        stopSession("传输已停止，未完成的文件未确认成功。");
    });
    invitations_.setInterval(3000);
    connect(&invitations_, &QTimer::timeout, this, [this] { pollInvitations(); });
#ifndef WETYPE_GUI_TEST
    invitations_.start();
    QTimer::singleShot(0, this, [this] { pollInvitations(); });
#endif
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
  void updateDeviceCards() {
    QStringList signature;
    for (int i = 0; i < peers_->count(); ++i)
      if (!peers_->itemData(i).toString().isEmpty())
        signature << peers_->itemText(i) << peers_->itemData(i).toString();
    if (signature != deviceSignature_) {
      deviceSignature_ = signature;
      while (auto *item = deviceCards_->takeAt(0)) {
        delete item->widget();
        delete item;
      }
      for (int i = 0; i < peers_->count(); ++i) {
        const auto id = peers_->itemData(i).toString();
        if (id.isEmpty())
          continue;
        const auto name = peers_->itemText(i);
        auto *button = new QPushButton;
        // QPushButton uses plain text; escape mnemonic markers, not HTML.
        button->setText(fontMetrics().elidedText(name, Qt::ElideRight, 190).replace('&', "&&") + "\n已关联");
        button->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        button->setObjectName("deviceCard");
        button->setAccessibleName(name + "，已关联");
        button->setToolTip(name + "\n发送时确认设备是否可连接");
        button->setProperty("peerId", id);
        button->setCheckable(true);
        button->setIcon(QIcon::fromTheme("computer", style()->standardIcon(QStyle::SP_ComputerIcon)));
        button->setIconSize(QSize(28, 28));
        deviceCards_->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, id] {
          peers_->setCurrentIndex(peers_->findData(id));
          updateActions();
        });
      }
    }
    for (int i = 0; i < deviceCards_->count(); ++i) {
      auto *button = qobject_cast<QPushButton *>(deviceCards_->itemAt(i)->widget());
      button->setChecked(button->property("peerId").toString() == peers_->currentData().toString());
      button->setEnabled(peers_->isEnabled());
    }
    deviceEmpty_->setVisible(deviceSignature_.isEmpty());
    deviceEmpty_->setText(devicesQueryFailed_ ? "暂时无法读取设备。请检查网络后刷新；无需重新关联。"
                        : devicesLoaded_ ? "还没有关联设备。先关联 Mac 或手机，再返回这里选择。"
                                        : "正在读取关联设备…");
  }

  void answerInvitation(bool accept) {
    if (!invitationPrompt_)
      return;
    const auto invite = pendingInvitation_;
    const auto code = invite.value("transfer_code").toString();
    pendingInvitation_ = {};
    invitationPrompt_ = false;
    if (ignoredInvitations_.size() >= 64)
      ignoredInvitations_.clear();
    ignoredInvitations_.insert(code);
    if (accept)
      acceptInvitation(invite);
    else
      status_->setText("已拒绝邀请，继续等待其他设备发送。");
    updateActions();
  }

  void updateActions() {
    const bool idle = !active_ && !invitationPrompt_ && session_.state() == QProcess::NotRunning;
    const bool peer = !peers_->currentData().toString().isEmpty();
    peers_->setEnabled(idle);
    refresh_->setEnabled(idle && !accountBusy_);
    refresh_->setText(idle && accountBusy_ ? "更新中…" : "刷新");
    refresh_->setToolTip(!idle ? "当前连接结束后可以刷新设备" : "重新读取关联设备列表");
    connect_->setEnabled(idle && peer && !accountBusy_);
    scan_->setEnabled(idle && !accountBusy_);
    send_->setEnabled(files_->count() &&
                      ((idle && peer && !accountBusy_) || connected_));
    const auto peerName = peers_->currentText();
    send_->setText(retryPending_ ? "重试发送" : peer
        ? "发送到 " + fontMetrics().elidedText(peerName, Qt::ElideRight, 150)
        : "发送文件");
    send_->setToolTip(peer ? "发送到 " + peerName : "请先关联并选择设备");
    files_->setVisible(files_->count() > 0);
    files_->setFixedHeight(qBound(54, files_->count() * 44 + 12, 170));
    removeFiles_->setVisible(files_->count() > 0);
    stop_->setEnabled(active_);
    stop_->setVisible(active_);
    stop_->setText(!connected_ ? "取消等待" : currentUpload_.isEmpty() && outgoing_.isEmpty()
        && !hasPendingTasks() ? "结束连接" : "停止全部传输");
    qint64 bytes = 0;
    for (int i = 0; i < files_->count(); ++i)
      bytes += QFileInfo(files_->item(i)->data(Qt::UserRole).toString()).size();
    summary_->setText(files_->count()
        ? QString("%1 个文件 · %2\n%3").arg(files_->count())
              .arg(QLocale().formattedDataSize(bytes))
              .arg(connected_ ? "发送到当前已连接的设备" : "对方接受后开始传输")
        : !outgoing_.isEmpty() || !currentUpload_.isEmpty()
              ? "文件已加入本次传输" : "先添加要发送的文件");
    sessionTitle_->setText(invitationPrompt_ ? "收到文件邀请"
        : active_ ? connected_ ? hasPendingTasks() ? "正在传输" : "连接已建立"
                              : "正在建立连接"
        : retryPending_ ? "文件已保留，可以重试" : "准备就绪");
    sessionTitle_->setVisible(active_ || retryPending_ || tasks_->topLevelItemCount() > 0);
    status_->setVisible(active_ || retryPending_ || devicesQueryFailed_ || sessionEnded_);
    tasks_->setVisible(tasks_->topLevelItemCount() > 0);
    invitationActions_->setVisible(invitationPrompt_);
    if (invitationPrompt_) {
      receiveTitle_->setText(pendingInvitation_.value("device_name").toString("关联设备") + " 想向你发送文件");
      receiveHint_->setText("来自已关联的设备。接受后，本次连接收到的文件会保存到下方接收位置。");
    } else {
      receiveTitle_->setText(active_ ? connected_ ? "连接已建立，可接收文件" : "正在建立连接…"
                                    : "等待其他设备发送");
      receiveHint_->setText(active_
          ? "请保持窗口打开。本次连接收到的文件会自动保存，结果显示在下方。"
          : "在 Mac 或手机的 WeType 中选择这台 Linux。\n收到邀请后，你可以选择接受或拒绝。");
    }
    updateDeviceCards();
  }

  bool hasPendingTasks() const {
    for (auto *row : taskRows_)
      if (!row->data(0, Qt::UserRole).toBool())
        return true;
    return false;
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
      auto *item = new QListWidgetItem(info.fileName() + "  ·  " +
                                           QLocale().formattedDataSize(info.size()),
                                       files_);
      item->setSizeHint(QSize(180, 44));
      item->setData(Qt::UserRole, info.canonicalFilePath());
      item->setToolTip(info.canonicalFilePath());
      known.insert(info.canonicalFilePath());
    }
    updateActions();
  }
  void updatePeerList(const QJsonObject &result) {
    if (!result.value("devices").isArray())
      return;
    devicesQueryFailed_ = false;
    const QSignalBlocker block(peers_);
    const auto selected = peers_->currentData().toString();
    peers_->clear();
    const auto self = wetype::jsonInteger(result.value("self_uin"));
    for (const auto &value : result.value("devices").toArray()) {
      const auto device = value.toObject();
      const auto uin = wetype::jsonInteger(device.value("uin"));
      if (uin <= 0 || uin == self)
        continue;
      peers_->addItem(device.value("name").toString("关联设备"), QString::number(uin));
    }
    if (!peers_->count())
      peers_->addItem("尚无关联设备", QString());
    const int index = peers_->findData(selected);
    if (index >= 0)
      peers_->setCurrentIndex(index);
    if (!devicesLoaded_) {
      devicesLoaded_ = true;
      status_->setText(peers_->currentData().toString().isEmpty()
          ? "尚无关联设备，请先在设置中关联。"
          : "关联设备已加载，正在等待文件邀请。也可选择设备主动连接。");
    }
    updateActions();
  }

  void pollInvitations() {
    if (active_ || accountBusy_ || invitationPrompt_ ||
        session_.state() != QProcess::NotRunning)
      return;
    runAccount({"pending-file-invite"}, [this](const QJsonObject &result) {
      if (active_)
        return;
      if (!result.value("ok").toBool()) {
        devicesQueryFailed_ = true;
        if (!devicesLoaded_)
          status_->setText("设备查询失败，正在重试；请检查账户工具和网络。");
        return;
      }
      updatePeerList(result);
      const auto invite = result.value("pending_invite").toObject();
      const auto code = invite.value("transfer_code").toString();
      if (code.isEmpty() || wetype::jsonInteger(invite.value("sender_uin")) <= 0 ||
          ignoredInvitations_.contains(code))
        return;
      // Invitations are in-memory only; never persist codes or start a backend
      // before the user accepts. The account tool validates the paired sender.
      pendingInvitation_ = invite;
      invitationPrompt_ = true;
      tabs_->setCurrentIndex(1);
      updateActions();
    });
  }
  void acceptInvitation(const QJsonObject &invite) {
    const auto code = invite.value("transfer_code").toString();
    const auto sender = wetype::jsonInteger(invite.value("sender_uin"));
    if (active_ || accountBusy_ || code.isEmpty() || sender <= 0 ||
        session_.state() != QProcess::NotRunning)
      return;
    active_ = true;
    connected_ = false;
    incoming_ = true;
    retryPending_ = false;
    sessionEnded_ = false;
    ++generation_;
    boundUin_ = QString::number(sender);
    int peerIndex = peers_->findData(boundUin_);
    if (peerIndex < 0) {
      peers_->addItem(invite.value("device_name").toString("关联设备"), boundUin_);
      peerIndex = peers_->count() - 1;
    }
    peers_->setCurrentIndex(peerIndex);
    transferCode_ = code;
    sessionOutput_.clear();
    outgoing_.clear();
    currentUpload_.clear();
    qr_->hide();
    status_->setText("正在接受文件传输邀请…");
    deadline_.start(180000);
    updateActions();
    startSession();
  }
  void begin(bool qr, bool sendAfterConnect) {
    if (active_ || session_.state() != QProcess::NotRunning || accountBusy_)
      return;
    boundUin_ = qr ? QString() : peers_->currentData().toString();
    if (!qr && boundUin_.isEmpty())
      return;
    active_ = true;
    connected_ = false;
    incoming_ = false;
    retryPending_ = false;
    sessionEnded_ = false;
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
    retryPending_ = false;
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
    sessionEnded_ = true;
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
    retryPending_ = files_->count() > 0;
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
      bool retained = false;
      if (!success && upload) {
        addFiles({path});
        for (int i = 0; i < files_->count(); ++i)
          retained |= files_->item(i)->data(Qt::UserRole).toString() == info.canonicalFilePath();
        retryPending_ |= retained;
      }
      row->setText(2, !success ? upload ? retained ? "失败，文件已保留可重试"
                                                 : "失败，请重新选择可读取的文件"
                                      : "接收失败，请让对方重新发送"
                      : upload ? "发送端报告完成"
                      : saved  ? "已落盘（大小已核对）"
                               : "对端报告完成，落盘未核实");
      if (saved)
        row->setToolTip(2, info.canonicalFilePath());
      row->setData(0, Qt::UserRole, true);
    }
    updateActions();
  }

protected:
  void resizeEvent(QResizeEvent *event) override {
    QWidget::resizeEvent(event);
    selectionLayout_->setDirection(width() < 760 ? QBoxLayout::TopToBottom
                                                 : QBoxLayout::LeftToRight);
  }

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
    tabs_->setCurrentIndex(0);
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
    directStarted_ = false;
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
    // Dispatch and peer credentials need not arrive in the same poll. Do not
    // consume the dispatch until both have been persisted successfully, or a
    // later response containing the certificate would be silently discarded.
    const auto encodedCa = peer.value(QStringLiteral("public_cer"));
    if (encodedCa.isUndefined() || encodedCa.isNull() ||
        (encodedCa.isString() && encodedCa.toString().isEmpty())) {
      status_->setText(QStringLiteral("等待手机返回安全凭据…（连接尚未就绪）"));
      return;
    }
    const auto decodedCa = QByteArray::fromBase64Encoding(
        encodedCa.toString().toLatin1(), QByteArray::AbortOnBase64DecodingErrors);
    if (!encodedCa.isString() || !decodedCa || decodedCa.decoded.isEmpty()) {
      status_->setText(QStringLiteral("手机安全凭据格式无效，等待有效响应…"));
      return;
    }
    const QByteArray &peerCa = decodedCa.decoded;
    const QString root = dataRoot();
    const QString state = root + QStringLiteral("/state/transfer");
    const QString path = state + QStringLiteral("/wxp2p-dispatch.bin");
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) ||
        file.write(dispatch) != dispatch.size() || !file.commit()) {
      status_->setText(QStringLiteral("无法保存手机传输调度信息：%1")
                           .arg(file.errorString()));
      return;
    }
    QFile::setPermissions(path,
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    const QString peerCaPath = state + QStringLiteral("/peer-ca.pem");
    QSaveFile peerCaFile(peerCaPath);
    if (!peerCaFile.open(QIODevice::WriteOnly) ||
        !peerCaFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner) ||
        peerCaFile.write(peerCa) != peerCa.size() || !peerCaFile.commit()) {
      status_->setText(QStringLiteral("手机凭据已收到，但本地保存失败：%1")
                           .arg(peerCaFile.errorString()));
      return;
    }
    lastDispatch_ = dispatch;
    if (wxpStarted_)
      grpcAdvertised_ = false;
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
        status_->setText(event.value("status").toInt(-1) != 0 || type == "read_error"
            ? type == "upload_done" || type == "read_error"
                ? "发送失败。可重试待发送列表中的文件；无法读取的文件请重新选择。"
                : "接收失败，请让对方重新发送。"
            : type == "upload_done" ? "发送端已完成，请在对端查看文件。"
                : "接收任务已结束，文件是否成功保存请查看下方结果。");
        progress_->hide();
        if ((type == "upload_done" || type == "read_error") &&
            event.value("path").toString() == currentUpload_) {
          currentUpload_.clear();
          sendNext();
        }
        updateActions();
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
      if (incoming_)
        joinInvitation(path);
      else
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
    if (incoming_)
      joinInvitation(path);
    else
      requestCodeFromAccount(accountArguments);
  }

  void joinInvitation(const QString &path) {
    runAccount({"p2p-update", transferCode_, path}, [this](const QJsonObject &result) {
      if (!result.value("ok").toBool()) {
        stopSession("无法接受邀请，邀请可能已过期；请让对端重新发送。");
        return;
      }
      status_->setText("已接受邀请，正在建立连接…");
      poll_.start();
      handlePeer(result);
    });
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
              : QStringLiteral("请在 %1 上接受文件邀请；邀请将在 %2 分钟后失效。")
                    .arg(peers_->currentText()).arg(minutes));
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
        [this](const QJsonObject &result) { handlePeer(result); });
  }

  void handlePeer(const QJsonObject &result) {
    const QJsonObject peer =
        result.value(QStringLiteral("peer")).toObject();
    if (!result.value(QStringLiteral("ok")).toBool())
      return;
    if (incoming_ && wetype::jsonInteger(peer.value("uin")) <= 0)
      return; // The peer identity can arrive in a subsequent response.
    if (incoming_ && QString::number(wetype::jsonInteger(peer.value("uin"))) != boundUin_) {
      stopSession("对端身份与邀请不符，连接已停止。");
      return;
    }
    const int peerState = peer.value("state").toInt();
    if (peerState == 1 || peerState == 2 || peerState == 4) {
      stopSession(peerState == 2 ? "对端已拒绝连接，请重新发起。"
                                : "对端已取消传输，请让对端重新发送。");
      return;
    }
    if (directStarted_)
      return; // Already chose direct TLS; late dispatch must not switch roles.
    const QByteArray dispatch = QByteArray::fromBase64(
        result.value(QStringLiteral("dispatch_buf"))
            .toString()
            .toLatin1());
    if (!dispatch.isEmpty()) {
      startWxp2p(dispatch, peer);
      return;
    }
    if (!incoming_ && !peer.value(QStringLiteral("ready")).toBool())
      return;
    const auto grpc = parseGrpcInfo(
        QByteArray::fromBase64(peer.value(QStringLiteral("grpc_net_info"))
                                   .toString()
                                   .toLatin1()));
    const QByteArray remoteCa = incoming_
        ? QByteArray::fromBase64(peer.value("public_cer").toString().toLatin1(),
                                QByteArray::AbortOnBase64DecodingErrors)
        : grpc.ca.toUtf8();
    if (remoteCa.isEmpty())
      return;
    const QString caPath =
        dataRoot() + QStringLiteral("/state/transfer/peer-ca.pem");
    QSaveFile ca(caPath);
    if (!ca.open(QIODevice::WriteOnly) ||
        ca.write(remoteCa) != remoteCa.size() || !ca.commit()) {
      status_->setText(QStringLiteral("无法保存对端安全凭据"));
      return;
    }
    QFile::setPermissions(caPath, QFileDevice::ReadOwner |
                                      QFileDevice::WriteOwner);
    directStarted_ = true;
    poll_.stop();
    sendSession({{QStringLiteral("action"), QStringLiteral("server")},
                 {QStringLiteral("remote_ca_file"), caPath}});
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
    updateActions();
  }

  void sendSession(const QJsonObject &command) {
    if (session_.state() == QProcess::Running) {
      session_.write(QJsonDocument(command).toJson(QJsonDocument::Compact));
      session_.write("\n");
    }
  }

  QComboBox *peers_ = nullptr;
  QTabBar *tabs_ = nullptr;
  QWidget *sendPage_ = nullptr, *receivePage_ = nullptr;
  QPushButton *removeFiles_ = nullptr;
  QBoxLayout *selectionLayout_ = nullptr;
  QVBoxLayout *deviceCards_ = nullptr;
  QStringList deviceSignature_;
  QLabel *deviceEmpty_ = nullptr, *summary_ = nullptr,
         *receiveTitle_ = nullptr, *receiveHint_ = nullptr, *sessionTitle_ = nullptr;
  QWidget *helpPanel_ = nullptr, *invitationActions_ = nullptr;
  QPushButton *acceptInvite_ = nullptr, *rejectInvite_ = nullptr;
  QJsonObject pendingInvitation_;
  bool retryPending_ = false;
  bool devicesQueryFailed_ = false;
  bool sessionEnded_ = false;
  QListWidget *files_ = nullptr;
  QTreeWidget *tasks_ = nullptr;
  QMap<QString, QTreeWidgetItem *> taskRows_;
  QPushButton *refresh_ = nullptr, *connect_ = nullptr, *scan_ = nullptr,
              *stop_ = nullptr;
  QProcess *accountProcess_ = nullptr;
  QTimer deadline_, activity_, invitations_;
  QSet<QString> ignoredInvitations_;
  bool incoming_ = false, invitationPrompt_ = false, devicesLoaded_ = false;
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
  bool directStarted_ = false;
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
