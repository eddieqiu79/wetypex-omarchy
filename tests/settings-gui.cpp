// SPDX-License-Identifier: GPL-3.0-only
#define main settingsApplicationMain
#include "../src/settings/main.cpp"
#undef main
#include <QTemporaryDir>
#include <QtTest>

class SettingsTests : public QObject {
  Q_OBJECT
private slots:
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
