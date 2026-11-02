// SPDX-License-Identifier: MIT
#pragma once
#include "../common/qt_helpers.hpp"
#include <QCloseEvent>
#include <QLineEdit>
#include <functional>
#include <QCheckBox>
#include <QDesktopServices>
#include <QDialog>
#include <QFileDialog>
#include <QInputDialog>
#include <QJsonDocument>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

namespace wetype {
// Installed-app onboarding. All mutations require a concrete button action;
// opening the dialog neither acquires runtime assets nor associates a device.
class SetupDialog final : public QDialog {
  QLabel *status_;
  QCheckBox *license_;
  QPushButton *download_;
  QProcess *process_ = nullptr;
  QList<QPushButton *> actions_;
  QString runtimeTool_, serviceTool_;
  std::function<void()> openDevices_;
public:
  SetupDialog(QString runtimeTool, QString serviceTool, std::function<void()> openDevices, QWidget *parent)
      : QDialog(parent), runtimeTool_(std::move(runtimeTool)), serviceTool_(std::move(serviceTool)), openDevices_(std::move(openDevices)) {
    setWindowTitle("开始使用 WeTypeX");
    resize(580, 560);
    auto *layout = new QVBoxLayout(this);
    auto *intro = new QLabel("依次检查运行资源、启用输入法并关联设备。已有账户和关联会继续使用。", this);
    intro->setWordWrap(true); layout->addWidget(intro);
    status_ = new QLabel("尚未检查。完成软件包安装不代表输入法已经启用。", this);
    status_->setWordWrap(true); status_->setTextFormat(Qt::PlainText); layout->addWidget(status_);
    auto button = [this, layout](const QString &text) {
      auto *b = new QPushButton(text, this); layout->addWidget(b); actions_ << b; return b;
    };
    auto *check = button("1. 检查运行资源");
    connect(check, &QPushButton::clicked, this, [this] { run(runtimeTool_, {"--check"}, true); });
    auto *archive = button("使用已有的官方安装包准备资源…");
    connect(archive, &QPushButton::clicked, this, [this] {
      const auto file = QFileDialog::getOpenFileName(this, "选择官方安装包");
      if (!file.isEmpty() && QMessageBox::question(this, "准备运行资源", "将验证所选官方安装包并准备当前用户的运行资源。继续？") == QMessageBox::Yes)
        run(runtimeTool_, {"--archive", file});
    });
    auto *agreement = button("阅读官方软件协议");
    connect(agreement, &QPushButton::clicked, this, [] { QDesktopServices::openUrl(QUrl("https://wetype.weixin.qq.com/web/agreement")); });
    license_ = new QCheckBox("我已阅读并同意适用的官方软件协议", this); layout->addWidget(license_);
    auto *download = button("从官方来源下载并准备资源");
    download_ = download;
    download->setEnabled(false);
    connect(license_, &QCheckBox::toggled, download, &QPushButton::setEnabled);
    connect(download, &QPushButton::clicked, this, [this] { run(runtimeTool_, {"--download", "--accept-upstream-license"}); });
    auto *input = button("2. 打开输入法配置，添加 WeTypeX");
    connect(input, &QPushButton::clicked, this, [this] {
      if (!QProcess::startDetached("fcitx5-configtool", {}))
        status_->setText("无法打开输入法配置工具，请先安装 fcitx5-configtool。");
    });
    auto *service = button("启用独立同步服务…");
    connect(service, &QPushButton::clicked, this, [this] {
      bool ok = false;
      const auto unit = QInputDialog::getText(this, "选择输入法服务", "现有 Fcitx 用户服务名称（会重新启动此服务）：", QLineEdit::Normal, "omarchy-fcitx5.service", &ok);
      if (!ok || unit.isEmpty()) return;
      if (QMessageBox::question(this, "启用同步", "将启用同步服务并重新启动 " + unit + "。请先结束输入和文件传输。继续？") == QMessageBox::Yes)
        run(serviceTool_, {"activate", "--fcitx-service", unit, "--confirm"});
    });
    auto *serviceStatus = button("检查同步服务状态");
    connect(serviceStatus, &QPushButton::clicked, this, [this] { run(serviceTool_, {"status"}, false, true); });
    auto *devices = button("3. 前往跨设备页面关联设备");
    connect(devices, &QPushButton::clicked, this, [this] { accept(); openDevices_(); });
    auto *close = new QPushButton("稍后继续", this); layout->addWidget(close);
    connect(close, &QPushButton::clicked, this, &QDialog::reject);
  }
  ~SetupDialog() override {
    // Disconnect before QWidget destroys labels and the running process.
    for (auto *process : findChildren<QProcess *>(QString(), Qt::FindDirectChildrenOnly))
      QObject::disconnect(process, nullptr, this, nullptr);
  }
  void reject() override {
    if (process_) { status_->setText("操作尚未结束，请等待结果后关闭。"); return; }
    QDialog::reject();
  }
protected:
  void closeEvent(QCloseEvent *event) override {
    if (process_) { event->ignore(); return; }
    QDialog::closeEvent(event);
  }
private:
  void run(const QString &tool, const QStringList &args, bool runtimeCheck = false, bool serviceCheck = false) {
    if (process_) return;
    for (auto *action : actions_) action->setEnabled(false);
    license_->setEnabled(false);
    status_->setText("正在处理，请稍候…");
    process_ = new QProcess(this);
    auto *process = process_;
    onProcessDone(process, this, [=](int code, QProcess::ExitStatus) {
      const auto data = QJsonDocument::fromJson(process->readAllStandardOutput()).object();
      if (runtimeCheck)
        status_->setText(data.value("core_ready").toBool() ? "核心运行资源可用。下一步添加输入法并检查实际输入。" : "运行资源尚未就绪，请选择官方安装包或在同意协议后下载。");
      else if (serviceCheck)
        status_->setText(!code && data.value("sync_active").toBool() ? "同步服务正在运行。设备连接与传输仍需实际检查。" : "未确认同步服务正在运行；可检查安装或启用服务。");
      else
        status_->setText(!code ? "本步操作已完成，请检查状态后继续下一步。" : "操作未完成。请检查资源版本、网络或服务配置；已有账户不会被删除。");
      process_ = nullptr;
      for (auto *action : actions_) action->setEnabled(true);
      // Download remains an explicit license-gated action after every operation.
      download_->setEnabled(license_->isChecked());
      license_->setEnabled(true);
      process->deleteLater();
    });
    process->start(tool, args);
  }
};
} // namespace wetype
