// SPDX-License-Identifier: GPL-3.0-only
#define main settingsApplicationMain
#include "../src/settings/main.cpp"
#undef main
#include <QTemporaryDir>
#include <QtTest>

class SettingsTests : public QObject {
  Q_OBJECT
private slots:
  void shortcutLayoutPreservesLongContent() {
    QWidget window;
    QFont font = window.font();
    font.setPointSize(18);
    window.setFont(font);
    auto *layout = new QVBoxLayout(&window);
    auto *row = shortcutRow("需要完整显示的较长快捷键说明文字", false,
                            {"Ctrl", "Shift", "PageDown"},
                            "窄窗口下说明换行，按键仍然完整显示。");
    layout->addWidget(row);
    window.resize(400, 400);
    window.show();
    QCoreApplication::processEvents();
    auto *box = qobject_cast<QBoxLayout *>(row->layout());
    QVERIFY(box);
    QCOMPARE(box->direction(), QBoxLayout::TopToBottom);
    for (auto *key : row->findChildren<QLabel *>("keyCap")) {
      QVERIFY(key->width() >= key->fontMetrics().horizontalAdvance(key->text()));
      QVERIFY(row->rect().contains(QRect(key->mapTo(row, QPoint()), key->size())));
    }
    auto *title = row->findChild<QLabel *>("rowTitle");
    QVERIFY(title && title->wordWrap());
    QVERIFY(title->height() >= title->heightForWidth(title->width()));
    window.resize(1200, 400);
    QCoreApplication::processEvents();
    QCOMPARE(box->direction(), QBoxLayout::LeftToRight);
  }
  void destroyPanelWithRunningProcess_data() {
    QTest::addColumn<bool>("setup");
    QTest::newRow("update-status") << false;
    QTest::newRow("setup-runtime-check") << true;
  }
  void destroyPanelWithRunningProcess() {
    QFETCH(bool, setup);
    QTemporaryDir temp;
    QFile file(temp.filePath("slow-helper"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("#!/bin/sh\nexec /usr/bin/sleep 30\n");
    file.close();
    QVERIFY(file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
    QWidget *panel;
    if (setup)
      panel = new wetype::SetupDialog(file.fileName(), file.fileName(), [] {}, nullptr);
    else
      panel = new wetype::UpdatePanel(file.fileName());
    panel->show();
    if (setup) {
      for (auto *button : panel->findChildren<QPushButton *>())
        if (button->text() == "1. 检查运行资源") button->click();
    }
    auto *process = panel->findChild<QProcess *>();
    QVERIFY(process);
    QTRY_COMPARE(process->state(), QProcess::Running);
    // Destruction, unlike an ordinary close request, must also be safe during
    // application shutdown. QProcess's destructor emits finished synchronously.
    QSignalSpy ended(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished));
    delete panel;
    QCOMPARE(ended.count(), 1);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
  }
  void updateDownloadAndAuthorizationRecovery() {
    QTemporaryDir temp;
    const auto oldConfig = qgetenv("XDG_CONFIG_HOME");
    qputenv("XDG_CONFIG_HOME", temp.path().toUtf8());
    const auto helper = temp.filePath("helper");
    QFile file(helper);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(R"SH(#!/bin/sh
case "$1" in
status) echo '{"status":"local-status","installed_version":"1:0.1.0rc4-1"}' ;;
check) echo '{"status":"available","version":"0.2.0","tag":"v0.2.0"}' ;;
download) echo '{"stage":"ready","tag":"v0.2.0","message":"已验证"}' ;;
install) echo '{"stage":"failed","tag":"v0.2.0","error_code":"authorization-cancelled","retryable":true,"message":"授权取消，下载已保留"}'; exit 1 ;;
resume) echo '{"stage":"installed","message":"已安装，待生效"}' ;;
esac
)SH");
    file.close();
    QVERIFY(file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
    wetype::UpdatePanel panel(helper);
    panel.show();
    auto *check = panel.findChild<QPushButton *>("checkUpdate");
    auto *prepare = panel.findChild<QPushButton *>("prepareUpdate");
    auto *install = panel.findChild<QPushButton *>("installUpdate");
    auto *resume = panel.findChild<QPushButton *>("resumeUpdate");
    auto *status = panel.findChild<QLabel *>("updateStatus");
    QTRY_VERIFY(check->isEnabled());
    QVERIFY(!install->isEnabled());
    check->click();
    QTRY_VERIFY(prepare->isEnabled());
    QVERIFY(!install->isEnabled());
    prepare->click();
    QTRY_VERIFY(install->isEnabled());
    auto confirm = [] {
      QTimer::singleShot(0, [] {
        for (auto *w : QApplication::topLevelWidgets())
          if (auto *box = qobject_cast<QMessageBox *>(w)) box->button(QMessageBox::Yes)->click();
      });
    };
    confirm(); install->click();
    QTRY_VERIFY(resume->isVisible() && resume->isEnabled());
    QVERIFY(status->text().contains("授权取消"));
    confirm(); resume->click();
    QTRY_VERIFY(status->text().contains("待生效"));
    QTRY_VERIFY(check->isEnabled());
    QVERIFY(!resume->isVisible());
    QVERIFY(!install->isEnabled());
    panel.close();
    qputenv("XDG_CONFIG_HOME", oldConfig);
  }
  void onboardingDoesNotStartWorkOnOpen() {
    wetype::SetupDialog dialog("/nonexistent-runtime", "/nonexistent-service", [] {}, nullptr);
    dialog.show();
    QCoreApplication::processEvents();
    QVERIFY(dialog.findChildren<QProcess *>().isEmpty());
    auto *license = dialog.findChild<QCheckBox *>();
    QVERIFY(license && !license->isChecked());
    for (auto *button : dialog.findChildren<QPushButton *>())
      if (button->text().contains("从官方来源")) QVERIFY(!button->isEnabled());
    dialog.close();
  }
  void singleInstanceRejectsParallelWindow() {
    QTemporaryDir temp;
    const auto before = qgetenv("XDG_RUNTIME_DIR");
    qputenv("XDG_RUNTIME_DIR", temp.path().toUtf8());
    wetype::SingleInstance first, second;
    QVERIFY(first.acquire("synthetic-settings-test"));
    QWidget window;
    first.bind(window);
    QVERIFY(!second.acquire("synthetic-settings-test"));
    QCoreApplication::processEvents();
    qputenv("XDG_RUNTIME_DIR", before);
  }
  void nativeResizableWindow() {
    Window w;
    QVERIFY(!(w.windowFlags() & Qt::FramelessWindowHint));
    QVERIFY(w.maximumWidth() > w.minimumWidth());
    QVERIFY(!w.testAttribute(Qt::WA_TranslucentBackground));
  }
  void preserveUnknownKeysAndRejectDamagedSettings() {
    QTemporaryDir temp;
    qputenv("XDG_CONFIG_HOME", temp.path().toUtf8());
    QDir().mkpath(configDir());
    const auto path = configDir() + "/wetypex.json";
    QVERIFY(writeFile(path, "{\"unknown_future_setting\":42}"));
    QVERIFY(writeSetting("fuzzy_z_zh", true));
    QCOMPARE(readSettings().value("unknown_future_setting").toInt(), 42);
    QVERIFY(readSettings().value("fuzzy_z_zh").toBool());
    const QByteArray damaged("{bad data");
    QVERIFY(writeFile(path, damaged));
    QTimer::singleShot(50, [] {
      for (auto *w : QApplication::topLevelWidgets())
        if (auto *box = qobject_cast<QMessageBox *>(w))
          box->accept();
    });
    QVERIFY(!writeSetting("standalone", false));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), damaged);
    qunsetenv("XDG_CONFIG_HOME");
  }
  void asyncEngineFailureDoesNotBlockEventLoop() {
    QTemporaryDir temp;
    qputenv("XDG_DATA_HOME", temp.path().toUtf8());
    qputenv("WETYPE_BACKEND_LAUNCHER", "/nonexistent-synthetic-backend");
    QObject owner;
    bool done = false;
    QString result;
    hotwordAsync(&owner, "hotword_list", {},
                 [&](const QJsonObject &, const QString &error) {
                   done = true;
                   result = error;
                 });
    bool ticked = false;
    QTimer::singleShot(0, [&] { ticked = true; });
    QTRY_VERIFY(ticked);
    QTRY_VERIFY_WITH_TIMEOUT(done, 5000);
    QVERIFY(!result.isEmpty());
    qunsetenv("XDG_DATA_HOME");
    qunsetenv("WETYPE_BACKEND_LAUNCHER");
  }
};
QTEST_MAIN(SettingsTests)
#include "settings-gui.moc"
