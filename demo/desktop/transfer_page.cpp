// SPDX-License-Identifier: GPL-3.0-only
#include "transfer_page.hpp"
#include <QButtonGroup>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QLabel>
#include <QListWidget>
#include <QLocale>
#include <QMimeData>
#include <QProgressBar>
#include <QPainter>
#include <QPushButton>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

namespace {
QLabel *label(const QString &value, const char *name = "") {
  auto *result = new QLabel(value);
  result->setTextFormat(Qt::PlainText);
  result->setWordWrap(true);
  result->setObjectName(name);
  return result;
}
QPushButton *button(const QString &value, const char *name) {
  auto *result = new QPushButton(value);
  result->setObjectName(name);
  return result;
}
QString sizeText(qint64 bytes) { return QLocale().formattedDataSize(bytes); }
QIcon deviceIcon(bool phone) {
  QPixmap image(36,36);
  image.fill(Qt::transparent);
  QPainter painter(&image);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(QPen(QColor("#24C891"),1.8));
  if (phone) {
    painter.drawRoundedRect(QRectF(10,3,16,29),3,3);
    painter.drawLine(15,7,21,7);
    painter.drawLine(15,28,21,28);
  } else {
    painter.drawRoundedRect(QRectF(5,6,26,19),2,2);
    painter.drawLine(2,29,34,29);
    painter.drawLine(5,25,2,29);
    painter.drawLine(31,25,34,29);
  }
  return QIcon(image);
}
}

TransferPage::TransferPage(demo::Model &data, QWidget *parent)
    : QWidget(parent), model(data) {
  setAcceptDrops(true);
  setStyleSheet(R"(
    QPushButton[device="true"] { text-align:left; padding:14px 18px; min-height:52px; background:#323231; border:1px solid #555552; }
    QPushButton[device="true"]:checked { border:2px solid #24C891; background:#293e34; color:#e5f8f0; }
    QPushButton#sendButton:disabled { background:#393936; border-color:#484845; color:#96968f; }
    QFrame#dropZone { background:#2e2e2c; border:1px dashed #71716b; border-radius:10px; }
    QFrame#dropZone QLabel { background:transparent; border:0; }
    QListWidget#pendingFiles, QListWidget#transferQueue { background:transparent; border:0; }
    QListWidget#transferQueue::item { margin-bottom:8px; }
    QListWidget#transferQueue::item:selected { background:#244a3b; border-radius:8px; }
    QFrame#taskCard { background:#323231; border:1px solid #444443; border-radius:8px; }
    QFrame#taskCard QLabel { background:transparent; border:0; }
    QProgressBar { border:0; background:#444443; height:5px; border-radius:2px; }
    QProgressBar::chunk { background:#24C891; border-radius:2px; }
  )");
  auto *body = new QVBoxLayout(this);
  body->setContentsMargins(26,22,26,20);
  body->setSpacing(12);
  body->addWidget(label("文件传输", "heading"));
  body->addWidget(label("选好设备，放入文件，一次发送。", "muted"));
  body->addWidget(label("发送给", "brand"));
  auto *devices = new QHBoxLayout;
  auto *group = new QButtonGroup(this);
  const QStringList peers = {"Mac", "iPhone"};
  for (int i = 0; i < peers.size(); ++i) {
    auto *device = button(peers[i] + "\n演示设备 · 未检测连接", i == 0 ? "peerMac" : "peerPhone");
    device->setCheckable(true);
    device->setIcon(deviceIcon(i == 1));
    device->setIconSize({36,36});
    device->setProperty("device", true);
    group->addButton(device, i);
    devices->addWidget(device, 1);
    connect(device, &QPushButton::clicked, this, [this, peers, i] {
      selectedPeer = peers[i]; refreshFiles();
    });
  }
  body->addLayout(devices);
  auto *drop = new QFrame;
  drop->setObjectName("dropZone");
  auto *dropLayout = new QVBoxLayout(drop);
  dropLayout->setContentsMargins(20,16,20,16);
  auto *hint = label("将文件拖到这里", "brand");
  hint->setAlignment(Qt::AlignCenter);
  dropLayout->addWidget(hint);
  auto *sub = label("支持多文件 · 选择后再发送", "muted");
  sub->setAlignment(Qt::AlignCenter);
  dropLayout->addWidget(sub);
  auto *picks = new QHBoxLayout;
  auto *choose = button("选择文件…", "chooseButton");
  auto *sample = button("用示例试一下", "sampleFiles");
  picks->addStretch(); picks->addWidget(choose); picks->addWidget(sample); picks->addStretch();
  dropLayout->addLayout(picks);
  body->addWidget(drop);
  files = new QListWidget;
  files->setObjectName("pendingFiles");
  files->setSelectionMode(QAbstractItemView::SingleSelection);
  files->setAcceptDrops(false); // Let the page validate local file URLs.
  body->addWidget(files);
  auto *composeActions = new QHBoxLayout;
  summary = label("尚未选择文件", "muted");
  auto *remove = button("移除所选", "removeFile");
  remove->setEnabled(false);
  auto *clear = button("清空", "clearFiles");
  composeActions->addWidget(summary,1); composeActions->addWidget(remove); composeActions->addWidget(clear);
  body->addLayout(composeActions);
  send = button("选择设备和文件", "sendButton");
  body->addWidget(send);
  notice = label("演示只读取选中文件的名称和大小，不读取内容或上传。", "muted");
  body->addWidget(notice);
  connect(choose, &QPushButton::clicked, this, &TransferPage::pickFiles);
  connect(sample, &QPushButton::clicked, this, [this] {
    pending = {{"installation-notes.md", "demo:md", 2048}, {"quick-start.pdf", "demo:pdf", 245760}};
    refreshFiles();
  });
  connect(files, &QListWidget::currentRowChanged, this, [remove](int row) { remove->setEnabled(row >= 0); });
  connect(remove, &QPushButton::clicked, this, [this] {
    const int row = files->currentRow();
    if (row >= 0 && row < pending.size()) pending.removeAt(row);
    refreshFiles();
  });
  connect(clear, &QPushButton::clicked, this, [this] { pending.clear(); refreshFiles(); });
  connect(send, &QPushButton::clicked, this, &TransferPage::sendFiles);

  body->addWidget(label("传输任务", "brand"));
  empty = label("还没有传输任务。发出邀请后，可在这里查看进度。", "muted");
  body->addWidget(empty);
  queue = new QListWidget;
  queue->setObjectName("transferQueue");
  queue->setAccessibleName("演示传输任务");
  queue->setAcceptDrops(false);
  body->addWidget(queue);
  detail = label({}, "detail");
  body->addWidget(detail);
  taskActions = new QWidget;
  auto *actions = new QHBoxLayout(taskActions);
  actions->setContentsMargins(0,0,0,0);
  auto *acceptIncoming = button("接受文件", "acceptIncoming");
  auto *rejectIncoming = button("拒绝", "rejectIncoming");
  cancel = button("取消传输", "cancelButton");
  retry = button("重新发送邀请", "retryButton");
  for (auto *b : {acceptIncoming, rejectIncoming, cancel, retry}) actions->addWidget(b);
  actions->addStretch();
  body->addWidget(taskActions);
  connect(acceptIncoming, &QPushButton::clicked, this, [this] { model.accept(selectedJob()); refresh(); });
  connect(rejectIncoming, &QPushButton::clicked, this, [this] { model.reject(selectedJob()); refresh(); });
  connect(cancel, &QPushButton::clicked, this, [this] { model.cancel(selectedJob()); refresh(); });
  connect(retry, &QPushButton::clicked, this, [this] { model.retry(selectedJob()); refresh(); });
  connect(queue, &QListWidget::currentRowChanged, this, [this] { refresh(); });
  auto *history = new QToolButton;
  history->setObjectName("showHistory");
  history->setCheckable(true);
  history->setText("最近传输（0）");
  history->setArrowType(Qt::RightArrow);
  history->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  body->addWidget(history,0,Qt::AlignLeft);
  connect(history, &QToolButton::toggled, this, [this,history](bool on) {
    history->setArrowType(on ? Qt::DownArrow : Qt::RightArrow); refresh();
  });

  auto *toggle = new QToolButton;
  toggle->setText("演示工具");
  toggle->setCheckable(true);
  toggle->setArrowType(Qt::RightArrow);
  toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  body->addWidget(toggle,0,Qt::AlignLeft);
  auto *simulation = new QWidget;
  auto *simLayout = new QVBoxLayout(simulation);
  simLayout->setContentsMargins(0,0,0,0);
  auto *simRow = new QHBoxLayout;
  accept = button("模拟对端接受", "acceptButton");
  reject = button("模拟对端拒绝", "rejectButton");
  fail = button("模拟连接失败", "failButton");
  for (auto *b : {accept,reject,fail}) simRow->addWidget(b);
  simLayout->addLayout(simRow);
  auto *incoming = button("模拟收到 iPhone 文件", "incomingButton");
  simLayout->addWidget(incoming);
  body->addWidget(simulation);
  simulation->hide();
  connect(toggle, &QToolButton::toggled, this, [toggle,simulation](bool on) {
    toggle->setArrowType(on ? Qt::DownArrow : Qt::RightArrow); simulation->setVisible(on);
  });
  connect(incoming, &QPushButton::clicked, this, &TransferPage::addIncoming);
  connect(accept, &QPushButton::clicked, this, [this] { model.accept(selectedJob()); refresh(); });
  connect(reject, &QPushButton::clicked, this, [this] { model.reject(selectedJob()); refresh(); });
  connect(fail, &QPushButton::clicked, this, [this] { model.fail(selectedJob()); refresh(); });
  body->addWidget(label("接收目录：Downloads/WeTypeX（演示）\n未连接真实传输；关闭窗口结束模拟任务。", "muted"));
  body->addStretch();
  refreshFiles(); refresh();
}

void TransferPage::pickFiles() {
  addPaths(QFileDialog::getOpenFileNames(this,"选择文件（演示不上传）",{},"All files (*)"));
}
void TransferPage::addPaths(const QStringList &paths) {
  int skipped = 0;
  for (const auto &path : paths) {
    QFileInfo info(path);
    if (!info.isFile() || pending.size() >= 100) { ++skipped; continue; }
    const auto identity = info.canonicalFilePath();
    bool duplicate = false;
    for (const auto &file : pending) duplicate |= file.identity == identity;
    if (!duplicate) pending.push_back({info.fileName(),identity,info.size()});
  }
  refreshFiles();
  if (skipped) notice->setText("已忽略目录、不可用文件或超出 100 项的内容；没有上传任何文件。");
}
void TransferPage::dragEnterEvent(QDragEnterEvent *event) {
  if (!event->mimeData()->hasUrls()) return;
  for (const auto &url : event->mimeData()->urls())
    if (url.isLocalFile()) { event->acceptProposedAction(); return; }
}
void TransferPage::dropEvent(QDropEvent *event) {
  QStringList paths;
  for (const auto &url : event->mimeData()->urls()) if (url.isLocalFile()) paths << url.toLocalFile();
  if (!paths.isEmpty()) { addPaths(paths); event->acceptProposedAction(); }
}
void TransferPage::refreshFiles() {
  files->clear();
  qint64 bytes = 0;
  for (const auto &file : pending) {
    auto *item = new QListWidgetItem(file.name + "    ·    " + sizeText(file.size), files);
    item->setToolTip(file.name);
    item->setSizeHint({0,32});
    bytes += file.size;
  }
  files->setVisible(!pending.isEmpty());
  findChild<QPushButton *>("removeFile")->setVisible(!pending.isEmpty());
  findChild<QPushButton *>("clearFiles")->setVisible(!pending.isEmpty());
  files->setFixedHeight(qMin(3,pending.size()) * 34 + 4);
  summary->setText(pending.isEmpty() ? "尚未选择文件" : QString("%1 个文件 · %2").arg(pending.size()).arg(sizeText(bytes)));
  send->setEnabled(!pending.isEmpty() && !selectedPeer.isEmpty());
  send->setText(selectedPeer.isEmpty() ? "先选择接收设备" : pending.isEmpty() ? "选择文件后发送到 " + selectedPeer : QString("发送 %1 个文件到 %2").arg(pending.size()).arg(selectedPeer));
}
void TransferPage::sendFiles() {
  if (pending.isEmpty() || selectedPeer.isEmpty()) return;
  QStringList names; qint64 bytes=0;
  for (const auto &f : pending) { names << f.name; bytes+=f.size; }
  const auto title = names.size() == 1 ? names.first() : names.first() + QString(" 等 %1 个文件").arg(names.size());
  const int id = model.add(title,selectedPeer,false,names,bytes);
  pending.clear(); refreshFiles(); refresh(); queue->setCurrentRow(id);
  notice->setText("模拟邀请已创建。展开「演示工具」模拟对端接受，开始进度演示。");
}
void TransferPage::addIncoming() {
  const int id = model.add("sample-document.pdf","iPhone",true,{},65536);
  refresh(); queue->setCurrentRow(id);
}
int TransferPage::selectedJob() const { return queue->currentRow(); }
void TransferPage::refresh() {
  auto *history = findChild<QToolButton *>("showHistory");
  int ended=0, visible=0;
  for (int i=0;i<model.jobs.size();++i) {
    const auto &t=model.jobs[i];
    auto *item=queue->item(i);
    if (!item) {
      item=new QListWidgetItem(queue);
      item->setSizeHint({0,105});
      auto *frame=new QFrame;
      frame->setObjectName("taskCard");
      frame->setAttribute(Qt::WA_TransparentForMouseEvents);
      auto *layout=new QVBoxLayout(frame);
      layout->setContentsMargins(14,10,14,10);
      layout->setSpacing(4);
      layout->addWidget(label({},"taskTitle"));
      layout->addWidget(label({},"taskMeta"));
      layout->addWidget(label({},"taskState"));
      auto *progress=new QProgressBar;
      progress->setObjectName("taskProgress");
      progress->setTextVisible(false);
      layout->addWidget(progress);
      queue->setItemWidget(item,frame);
    }
    auto *frame=queue->itemWidget(item);
    frame->findChild<QLabel *>("taskTitle")->setText(t.file);
    frame->findChild<QLabel *>("taskMeta")->setText((t.incoming?"来自 ":"发送到 ")+t.peer+" · "+sizeText(t.bytes));
    frame->findChild<QLabel *>("taskState")->setText(demo::stateText(t.state));
    auto *progress=frame->findChild<QProgressBar *>("taskProgress");
    progress->setValue(t.progress);
    progress->setVisible(t.state==demo::State::Transferring);
    const bool terminal=!demo::active(t.state);
    if (terminal) ++ended;
    // Failed/rejected/cancelled tasks stay visible for recovery. Completed jobs
    // move into the expandable recent history.
    const bool hide=t.state==demo::State::Complete && !history->isChecked();
    item->setHidden(hide);
    if (!hide) ++visible;
    item->setToolTip(t.files.join('\n'));
  }
  history->setText(QString("最近传输（%1 已结束）").arg(ended));
  history->setVisible(ended>0);
  queue->setVisible(visible>0);
  queue->setFixedHeight(qMin(3,visible)*110+4);
  empty->setVisible(visible==0);
  empty->setText(model.jobs.isEmpty()?"还没有传输任务。发出邀请后，可在这里查看进度。":"当前没有进行中的任务，可展开最近传输。 ");
  int id=selectedJob();
  const bool selected=id>=0 && id<model.jobs.size();
  const demo::Transfer t=selected?model.jobs[id]:demo::Transfer{};
  const bool waiting=selected&&(t.state==demo::State::Offered||t.state==demo::State::Waiting);
  accept->setEnabled(waiting); reject->setEnabled(waiting);
  fail->setEnabled(selected&&demo::active(t.state));
  cancel->setEnabled(selected&&demo::active(t.state));
  cancel->setVisible(cancel->isEnabled());
  retry->setEnabled(selected&&demo::retryable(t.state));
  retry->setVisible(retry->isEnabled());
  retry->setText(t.incoming?"模拟重新收到邀请":"重新发送邀请");
  findChild<QPushButton *>("acceptIncoming")->setVisible(selected&&t.state==demo::State::Offered);
  findChild<QPushButton *>("rejectIncoming")->setVisible(selected&&t.state==demo::State::Offered);
  taskActions->setVisible(selected&&(demo::active(t.state)||demo::retryable(t.state)));
  detail->setVisible(selected);
  if (selected) detail->setText(t.state==demo::State::Complete
    ? "演示完成，未实际保存文件，因此没有打开文件／文件夹操作。"
    : t.state==demo::State::Failed ? "未能建立连接（模拟）。正式传输需检查局域网连通性后重新发出邀请。"
    : (t.incoming?"接收位置：Downloads/WeTypeX（未落盘）":"已选任务："+t.file));
}
