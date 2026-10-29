// SPDX-License-Identifier: GPL-3.0-only
#include "window.hpp"
#include "settings_pages.hpp"
#include "transfer_page.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QShortcut>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {
QLabel *text(const QString &value, const char *name = "") {
  auto *label = new QLabel(value);
  label->setWordWrap(true);
  label->setTextFormat(Qt::PlainText);
  label->setObjectName(name);
  return label;
}
QVBoxLayout *pageLayout(QWidget *page, const QString &title,
                        const QString &subtitle) {
  auto *layout = new QVBoxLayout(page);
  layout->setContentsMargins(26, 22, 26, 20);
  layout->setSpacing(14);
  layout->addWidget(text(title, "heading"));
  layout->addWidget(text(subtitle, "muted"));
  return layout;
}
QPushButton *button(const QString &title, const char *name) {
  auto *result = new QPushButton(title);
  result->setObjectName(name);
  result->setCursor(Qt::PointingHandCursor);
  return result;
}
QVBoxLayout *card(QVBoxLayout *parent) {
  auto *frame = new QFrame;
  frame->setObjectName("card");
  auto *layout = new QVBoxLayout(frame);
  layout->setContentsMargins(18, 15, 18, 15);
  layout->setSpacing(12);
  parent->addWidget(frame);
  return layout;
}
void optionRow(QVBoxLayout *parent, const QString &title, QCheckBox *control) {
  auto *row = new QHBoxLayout;
  auto *label = text(title);
  label->setBuddy(control);
  control->setText({});
  control->setAccessibleName(title);
  row->addWidget(label, 1);
  row->addWidget(control);
  parent->addLayout(row);
}
void addScrollable(QStackedWidget *stack, QWidget *page) {
  auto *scroll = new QScrollArea;
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setWidget(page);
  stack->addWidget(scroll);
}
}

DemoWindow::DemoWindow(bool animate) {
  setWindowTitle(QStringLiteral("WeTypeX · 桌面交互 Demo"));
  resize(1020, 740);
  setMinimumSize(640, 460);
  // Ordinary decorated window: no fixed size, forced floating, positioning or
  // always-on-top flag. The compositor owns placement and window management.
  setStyleSheet(QStringLiteral(R"(
    QMainWindow, QWidget { background: #282827; color: #eeeeec; font-size: 14px; }
    QLabel { background: transparent; }
    QLabel#heading { font-size: 23px; font-weight: 600; }
    QLabel#brand { font-size: 18px; font-weight: 600; }
    QLabel#muted { color: #b0b0ac; }
    QLabel#badge { color: #24C891; font-size: 12px; }
    QWidget#sidebar { background: #242423; }
    QFrame#card { background: #323231; border: 1px solid #444443; border-radius: 10px; }
    QFrame#card QLabel, QFrame#card QCheckBox { background: transparent; }
    QWidget#settingRow { background: transparent; }
    QListWidget#navigation { background: transparent; border: 0; padding: 0; }
    QListWidget#navigation::item { padding: 12px 10px; margin-bottom: 5px; border-radius: 7px; }
    QListWidget#navigation::item:selected { background: #24C891; color: #282827; }
    QListWidget#navigation::item:hover:!selected { background: #3b3b39; }
    QPushButton, QComboBox { background: #323231; border: 1px solid #555552;
      border-radius: 6px; padding: 7px 12px; min-height: 22px; }
    QPushButton:hover { background: #383e39; border-color: #24C891; }
    QPushButton:disabled { color: #92928e; border-color: #444443; }
    QPushButton#sendButton { background: #24C891; border-color: #24C891; color: #282827; font-weight: 600; }
    QPushButton:focus, QComboBox:focus, QTreeWidget:focus, QCheckBox:focus,
    QListWidget:focus, QToolButton:focus { outline: 0; border: 2px solid #24C891; }
    QTreeWidget { background: #323231; alternate-background-color: #363634;
      border: 1px solid #444443; border-radius: 8px; }
    QTreeWidget::item { padding: 9px 3px; }
    QTreeWidget::item:selected { background: #244a3b; color: #e5f8f0; }
    QHeaderView::section { background: #242423; color: #c3c3bf;
      border: 0; padding: 9px 5px; }
    QCheckBox { spacing: 10px; padding: 7px; background: transparent; }
    QCheckBox::indicator { width: 18px; height: 18px; }
    QCheckBox::indicator:checked { background: #24C891; border: 1px solid #24C891; border-radius: 4px; }
    QCheckBox::indicator:unchecked { background: #323231; border: 1px solid #777773; border-radius: 4px; }
    QToolButton { background: transparent; color: #24C891; border: 1px solid transparent; padding: 6px; }
    QStatusBar { background: #242423; color: #b4b4af; }
    QLineEdit, QSpinBox, QListWidget#hotwords { background: #2d2d2b; color: #eeeeec; border: 1px solid #555552; border-radius: 5px; padding: 5px; }
    QComboBox QAbstractItemView { background: #323231; color: #eeeeec; selection-background-color: #244a3b; selection-color: #eeeeec; }
    QScrollArea { border: 0; }
    QScrollBar:vertical { background: #282827; width: 10px; margin: 0; }
    QScrollBar::handle:vertical { background: #666662; border-radius: 5px; min-height: 30px; }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
    QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
    QSlider::groove:horizontal { background: #555552; height: 5px; border-radius: 2px; }
    QSlider::handle:horizontal { background: #24C891; width: 15px; margin: -5px 0; border-radius: 7px; }
    QToolTip { background: #363634; color: #eeeeec; border: 1px solid #666662; padding: 5px; }
  )"));
  auto *root = new QWidget;
  auto *shell = new QHBoxLayout(root);
  shell->setContentsMargins(0, 0, 0, 0);
  shell->setSpacing(0);
  auto *rail = new QWidget;
  rail->setObjectName("sidebar");
  rail->setFixedWidth(184);
  auto *railLayout = new QVBoxLayout(rail);
  railLayout->setContentsMargins(12, 24, 12, 12);
  railLayout->addWidget(text("WeTypeX", "brand"));
  railLayout->addWidget(text("桌面适配 · 交互演示", "badge"));
  navigation = new QListWidget;
  navigation->setObjectName("navigation");
  navigation->addItems({"文件传输", "输入", "语音输入", "常用语和剪贴板", "外观", "快捷键", "跨设备", "版本与更新"});
  navigation->setAccessibleName("页面导航");
  railLayout->addWidget(navigation, 1);
  railLayout->addWidget(text("Ctrl + 1…8  切换页面\nCtrl + O  选择文件\nCtrl + Enter  发送\nEsc  关闭演示", "muted"));
  shell->addWidget(rail);
  pages = new QStackedWidget;
  shell->addWidget(pages, 1);
  setCentralWidget(root);
  statusBar()->showMessage("隔离演示 · 不读取账户或剪贴板，不发送文件，不修改设置");

  transferPage = new TransferPage(model);
  addScrollable(pages, transferPage);

  for (const QString &category : {"input", "voice", "phrases", "appearance", "shortcuts"})
    addScrollable(pages, makeSettingsPage(category));

  auto *devicePage = new QWidget;
  auto *devices = pageLayout(devicePage, "跨设备", "对应原版的剪贴板、个人词库和常用语同步；这里只修改演示内存。");
  auto *syncCard = card(devices);
  QVector<QCheckBox *> syncControls;
  const QStringList names = {"跨设备复制粘贴（文字、图片）", "个人词库同步", "常用语同步"};
  const QStringList keys = {"device_clipboard_sync", "device_dictionary_sync", "device_phrase_sync"};
  for (int i = 0; i < names.size(); ++i) {
    auto *check = new QCheckBox;
    check->setChecked(true);
    check->setObjectName(keys[i]);
    check->setProperty("saved", true);
    optionRow(syncCard, names[i], check);
    syncControls.push_back(check);
  }
  auto *syncState = text("演示预设 · 尚未读取真实配置", "device_status");
  syncCard->addWidget(syncState);
  auto *applySync = button("应用到演示", "device_apply");
  auto *resetSync = button("撤销未应用修改", "device_reset");
  auto *syncButtons = new QHBoxLayout;
  syncButtons->addWidget(applySync);
  syncButtons->addWidget(resetSync);
  syncButtons->addStretch();
  syncCard->addLayout(syncButtons);
  for (auto *check : syncControls) connect(check, &QCheckBox::toggled, this, [syncState] {
    syncState->setText("有演示修改，应用仅保存到内存");
  });
  connect(applySync, &QPushButton::clicked, this, [syncControls, syncState] {
    for (auto *check : syncControls) check->setProperty("saved", check->isChecked());
    syncState->setText("已应用到演示 · 未请求真实同步");
  });
  connect(resetSync, &QPushButton::clicked, this, [syncControls, syncState] {
    for (auto *check : syncControls) check->setChecked(check->property("saved").toBool());
    syncState->setText("已撤销未应用的演示修改");
  });
  auto *requestSync = button("立即请求同步（未接入）", "requestSync");
  requestSync->setEnabled(false);
  requestSync->setToolTip("原实现支持 sync-now；演示不连接官方服务");
  devices->addWidget(requestSync);
  devices->addWidget(text("原实现是一个文字／图片剪贴板同步开关；本机剪贴板历史在「常用语和剪贴板」中设置。", "muted"));
  for (const QString &device : {"Mac", "iPhone"}) {
    auto *deviceCard = card(devices);
    deviceCard->addWidget(text(device + " · 合成设备", "brand"));
    deviceCard->addWidget(text("配对：仅模拟    最近通信：未检查    局域网连接：未检查", "muted"));
  }
  auto *associate = button("关联设备 / 查看匹配码（未接入）", "associateDevice");
  associate->setEnabled(false);
  devices->addWidget(associate);
  auto *transferEntry = button("打开文件传输演示", "transferEntry");
  connect(transferEntry, &QPushButton::clicked, this, [this] { navigation->setCurrentRow(0); });
  devices->addWidget(transferEntry);
  devices->addWidget(text("iPhone 可能需要展开官方微信输入法键盘；入口可见不代表系统全局剪贴板已写入。Mac／iPhone 继续使用已有官方客户端。", "muted"));
  devices->addStretch();
  addScrollable(pages, devicePage);

  auto *updatePage = new QWidget;
  auto *updates = pageLayout(updatePage, "版本与更新", "更新到经过验证的 Omarchy 衍生版，保留输入和互通适配。");
  updates->addWidget(text("当前界面：独立开发 Demo\n现用程序版本：未读取\n可用新版本：未查询", "brand"));
  updates->addWidget(text("用户更新来源\neddieqiu79/wetypex-omarchy", "badge"));
  updates->addWidget(text("原始 WeTypeX → 合并与回归 → 衍生版版本发布 → 审核 Arch 包并升级", "muted"));
  updates->addWidget(text("目前使用固定源码构建、检查后通过 pacman 升级；尚无正式稳定的自动更新通道。官方引擎更新、输入法重启和设备配对各自独立。", "muted"));
  auto *checkUpdate = button("检查更新 · 尚未接入", "checkUpdate");
  checkUpdate->setEnabled(false);
  updates->addWidget(checkUpdate);
  updates->addWidget(text("升级前应检查活动传输、保留旧包，并明确何时重启。构建通过不等于真实设备互通验收通过。", "muted"));
  updates->addStretch();
  addScrollable(pages, updatePage);

  connect(navigation, &QListWidget::currentRowChanged, pages, &QStackedWidget::setCurrentIndex);
  navigation->setCurrentRow(0);
  for (int i = 0; i < pages->count(); ++i) {
    auto *key = new QShortcut(QKeySequence(QString("Ctrl+%1").arg(i + 1)), this);
    connect(key, &QShortcut::activated, this, [this, i] { navigation->setCurrentRow(i); });
  }
  auto *openKey = new QShortcut(QKeySequence("Ctrl+O"), this);
  connect(openKey, &QShortcut::activated, this, [this] { navigation->setCurrentRow(0); pickFile(); });
  auto *sendKey = new QShortcut(QKeySequence("Ctrl+Return"), this);
  connect(sendKey, &QShortcut::activated, this, [this] {
    if (navigation->currentRow() == 0) sendFile();
  });
  auto *closeKey = new QShortcut(QKeySequence("Esc"), this);
  connect(closeKey, &QShortcut::activated, this, &QWidget::close);
  if (animate) {
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this] { model.tick(); refresh(); });
    timer->start(650);
  }
  refresh();
}

void DemoWindow::pickFile() { transferPage->pickFiles(); }
void DemoWindow::sendFile() { transferPage->sendFiles(); }
void DemoWindow::refresh() { transferPage->refresh(); }
