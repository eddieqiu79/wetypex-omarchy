// SPDX-License-Identifier: MIT
#pragma once
#include "../common/qt_helpers.hpp"
#include <QMap>
#include <QCloseEvent>
#include <QComboBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

namespace wetype {
// Shared CLI performs discovery, verification and installation. This widget
// consumes its event protocol; it never parses package-manager console output.
class UpdatePanel final : public QWidget {
  QString tool_, tag_, resumeTag_, stage_;
  QProcess *process_ = nullptr;
  QByteArray pending_;
  bool installing_ = false;
  bool initialized_ = false;
  bool freshCache_ = false;
  QComboBox *channel_;
  QLabel *status_;
  QProgressBar *progress_;
  QPushButton *check_, *prepare_, *install_, *cancel_, *resume_;
  QPlainTextEdit *details_;
public:
  explicit UpdatePanel(const QString &tool, QWidget *parent = nullptr) : QWidget(parent), tool_(tool) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    channel_ = new QComboBox(this);
    channel_->setObjectName("updateChannel");
    channel_->addItem("稳定版", "stable");
    channel_->addItem("预览版（主动加入）", "preview");
    QSettings prefs("wetypex-omarchy", "updates");
    channel_->setCurrentIndex(prefs.value("channel").toString() == "preview" ? 1 : 0);
    layout->addWidget(channel_);
    status_ = new QLabel("检查更新后，可在这里下载、验证并安装。", this);
    status_->setObjectName("updateStatus");
    status_->setWordWrap(true);
    status_->setTextFormat(Qt::PlainText);
    layout->addWidget(status_);
    progress_ = new QProgressBar(this);
    progress_->setRange(0, 0);
    progress_->setTextVisible(false);
    progress_->hide();
    layout->addWidget(progress_);
    check_ = new QPushButton("检查更新", this);
    prepare_ = new QPushButton("下载并验证", this);
    install_ = new QPushButton("安装更新", this);
    resume_ = new QPushButton("继续上次安装", this);
    cancel_ = new QPushButton("取消准备", this);
    check_->setObjectName("checkUpdate");
    prepare_->setObjectName("prepareUpdate");
    install_->setObjectName("installUpdate");
    resume_->setObjectName("resumeUpdate");
    for (auto *button : {check_, prepare_, install_, resume_, cancel_}) layout->addWidget(button);
    prepare_->setEnabled(false);
    install_->setEnabled(false);
    resume_->hide();
    cancel_->hide();
    auto *showDetails = new QPushButton("查看详情", this);
    layout->addWidget(showDetails);
    details_ = new QPlainTextEdit(this);
    details_->setReadOnly(true);
    details_->setMaximumHeight(150);
    details_->hide();
    layout->addWidget(details_);
    connect(showDetails, &QPushButton::clicked, this, [this] { details_->setVisible(!details_->isVisible()); });
    connect(channel_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] {
      tag_.clear(); stage_.clear(); prepare_->setEnabled(false); install_->setEnabled(false);
      QSettings("wetypex-omarchy", "updates").setValue("channel", channel_->currentData());
      status_->setText("已切换频道，请检查更新。已有下载仍会保留。");
    });
    connect(check_, &QPushButton::clicked, this, [this] { start("check"); });
    connect(prepare_, &QPushButton::clicked, this, [this] { start("download"); });
    connect(install_, &QPushButton::clicked, this, [this] { authorize("install"); });
    connect(resume_, &QPushButton::clicked, this, [this] { authorize("resume"); });
    connect(cancel_, &QPushButton::clicked, this, [this] {
      if (process_ && !installing_) { cancel_->setEnabled(false); process_->terminate(); }
    });
  }
  ~UpdatePanel() override {
    // QProcess can emit finished during child destruction, after our fields
    // and sibling widgets have already been destroyed.
    for (auto *process : findChildren<QProcess *>(QString(), Qt::FindDirectChildrenOnly))
      QObject::disconnect(process, nullptr, this, nullptr);
  }
  void authorize(const QString &action) {
    if (QMessageBox::question(this, "安装更新", "将安装 " + (action == "resume" ? resumeTag_ : tag_) + " 的已验证软件包。输入法和传输服务不会自动重启。\n继续后会请求系统授权。") == QMessageBox::Yes)
      start(action);
  }
  void applyEvent(const QJsonObject &event) {
    const auto state = event.value("stage").toString(event.value("status").toString());
    if (event.value("bytes_total").toDouble() > 0 && state == "downloading") {
      progress_->setRange(0, 100);
      progress_->setValue(int(100.0 * event.value("bytes_received").toDouble() / event.value("bytes_total").toDouble()));
    } else if (state != "downloading") {
      progress_->setRange(0, 0);
    }
    if (event.contains("message")) status_->setText(event.value("message").toString());
    details_->appendPlainText(QString::fromUtf8(QJsonDocument(event).toJson(QJsonDocument::Compact)));
    if (state == "local-status") {
      const auto installed = event.value("installed_version").toString();
      status_->setText(installed.isEmpty() ? "尚未检测到系统安装。" : "已安装软件包：" + installed + "。可检查所选频道的更新。");
      const auto operation = event.value("operation").toObject();
      const auto previous = operation.value("stage").toString();
      resumeTag_ = operation.value("tag").toString();
      resume_->setVisible(!operation.value("tag").toString().isEmpty() && previous != "installed");
      return;
    }
    stage_ = state;
    if (event.contains("tag")) tag_ = event.value("tag").toString();
    const QMap<QString, QString> messages{
      {"no-release", "所选频道尚无发布版本。预览版本需要主动切换频道。"},
      {"available", "发现新版本 " + event.value("version").toString() + "，可以下载并验证。"},
      {"current", "已安装的软件包与当前发布版本一致。"},
      {"installed-newer", "已安装版本比此频道的新，不会自动降级。"},
      {"runtime-review-required", "新版本需要不同的运行资源，请先查看迁移说明。"},
      {"release-not-immutable", "发布尚未完成，不可用于安装。"},
      {"installed-version-unknown", "无法识别安装版本，请查看安装来源。"}};
    if (messages.contains(state)) status_->setText(messages.value(state));
    if (state == "ready") { freshCache_ = false; prepare_->setText("下载并验证"); install_->setEnabled(true); }
    if (state == "failed") {
      const auto error = event.value("error_code").toString();
      if (event.contains("tag")) resumeTag_ = event.value("tag").toString();
      freshCache_ = error == "package-integrity" || error == "cache-incomplete";
      prepare_->setText(freshCache_ ? "重新下载（保留原缓存）" : "下载并验证");
      resume_->setVisible(!freshCache_ && !resumeTag_.isEmpty() && event.value("retryable").toBool());
    }
    if (state == "installed") { install_->setEnabled(false); resume_->hide(); }
  }
protected:
  void showEvent(QShowEvent *event) override {
    QWidget::showEvent(event);
    if (!initialized_) {
      initialized_ = true;
      window()->installEventFilter(this);
      start("status");  // Local metadata only; no network request on page open.
    }
  }
  bool eventFilter(QObject *, QEvent *event) override {
    if (event->type() == QEvent::Close && process_) {
      static_cast<QCloseEvent *>(event)->ignore();
      status_->setText(installing_ ? "安装操作尚未结束，请等待结果后关闭窗口。" : "请先取消准备或等待操作结束。");
      return true;
    }
    return false;
  }
private:
  void start(const QString &action) {
    if (process_) return;
    installing_ = action == "install" || action == "resume";
    pending_.clear(); stage_.clear();
    process_ = new QProcess(this);
    auto *process = process_;
    QStringList arguments{action, "--json", "--channel", channel_->currentData().toString()};
    if (!tag_.isEmpty() && action != "check" && action != "status" && action != "resume") arguments << "--tag" << tag_;
    if (action == "resume" && !resumeTag_.isEmpty()) arguments << "--tag" << resumeTag_;
    if (freshCache_ && action == "download") arguments << "--fresh-cache";
    if (installing_) arguments << "--confirm" << "--authorization" << "desktop";
    check_->setEnabled(false); prepare_->setEnabled(false); install_->setEnabled(false);
    channel_->setEnabled(false); resume_->setEnabled(false);
    cancel_->setEnabled(true); cancel_->setVisible(action == "download" || action == "check");
    progress_->show();
    status_->setText(action == "status" ? "正在读取安装状态…" : "正在处理，请稍候…");
    connect(process, &QProcess::readyReadStandardOutput, this, [this, process] {
      pending_ += process->readAllStandardOutput();
      int newline;
      while ((newline = pending_.indexOf('\n')) >= 0) {
        const auto line = pending_.left(newline); pending_.remove(0, newline + 1);
        const auto object = QJsonDocument::fromJson(line).object();
        if (!object.isEmpty()) applyEvent(object);
      }
    });
    // FailedToStart does not emit finished. One completion handler for both.
    onProcessDone(process, this, [this, process](int code, QProcess::ExitStatus) {
      if (!pending_.trimmed().isEmpty()) applyEvent(QJsonDocument::fromJson(pending_).object());
      if (code && stage_ != "failed") status_->setText("操作未完成。请检查更新工具是否已安装，再重试；已有下载会保留。");
      process_ = nullptr; installing_ = false;
      check_->setEnabled(true); channel_->setEnabled(true); resume_->setEnabled(true);
      prepare_->setEnabled(stage_ == "available" || freshCache_);
      install_->setEnabled(stage_ == "ready");
      cancel_->hide(); progress_->hide(); process->deleteLater();
    });
    process->start(tool_, arguments);
  }
};
} // namespace wetype
