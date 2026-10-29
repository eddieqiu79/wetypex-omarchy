// SPDX-License-Identifier: GPL-3.0-only
#define WETYPE_GUI_TEST
#include "../src/transfer/main.cpp"
#include <QTemporaryDir>
#include <QtTest>

class TransferTests : public QObject {
  Q_OBJECT
private slots:
  void selectionRequiresLocalFilesAndExplicitSend() {
    QTemporaryDir temp;
    QFile f(temp.filePath("sample.md"));
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("synthetic\n");
    f.close();
    TransferWindow w;
    w.addFiles(
        {f.fileName(), f.fileName(), temp.path(), temp.filePath("missing")});
    QCOMPARE(w.files_->count(), 1);
    QVERIFY(!w.send_->isEnabled());
    w.peers_->addItem("Synthetic Mac", "123");
    w.peers_->setCurrentIndex(1);
    QVERIFY(w.send_->isEnabled());
    QVERIFY(!w.active_);
    QCOMPARE(w.session_.state(), QProcess::NotRunning);
    QVERIFY(!(w.windowFlags() & Qt::FramelessWindowHint));
  }
  void downloadCompletionRequiresActualFileAndSize() {
    QTemporaryDir temp;
    TransferWindow w;
    w.downloads_ = temp.path();
    const auto path = temp.filePath("received.md");
    w.recordTask({{"event", "download_done"},
                  {"path", path},
                  {"total", 5},
                  {"status", 0}});
    QVERIFY(w.tasks_->topLevelItem(0)->text(2).contains("未核实"));
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("hello");
    f.close();
    w.recordTask({{"event", "download_start"}, {"path", path}});
    w.recordTask({{"event", "download_done"},
                  {"path", path},
                  {"total", 5},
                  {"status", 0}});
    QVERIFY(w.tasks_->topLevelItem(1)->text(2).startsWith("已落盘"));
    w.recordTask({{"event", "download_progress"},
                  {"path", path},
                  {"total", 5},
                  {"transferred", 1}});
    QVERIFY(w.tasks_->topLevelItem(1)->text(2).startsWith("已落盘"));
  }
  void stopIsTerminalAndRestoresUnsentSelection() {
    QTemporaryDir temp;
    QFile f(temp.filePath("queued.pdf"));
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("synthetic");
    f.close();
    TransferWindow w;
    w.active_ = true;
    w.connected_ = true;
    w.currentUpload_ = f.fileName();
    w.outgoing_.enqueue(f.fileName());
    w.recordTask({{"event", "upload_start"}, {"path", f.fileName()}});
    const auto generation = w.generation_;
    w.stopSession("test");
    QVERIFY(!w.active_);
    QVERIFY(!w.connected_);
    QVERIFY(w.generation_ > generation);
    QVERIFY(w.outgoing_.isEmpty());
    QVERIFY(w.currentUpload_.isEmpty());
    QCOMPARE(w.files_->count(), 1);
    w.recordTask(
        {{"event", "upload_done"}, {"path", f.fileName()}, {"status", 0}});
    QVERIFY(w.tasks_->topLevelItem(0)->text(2).contains("中断"));
  }
  void backendEventsSerializeFilesAndFailureUnblocksQueue() {
    QTemporaryDir temp;
    QFile helper(temp.filePath("backend"));
    QVERIFY(helper.open(QIODevice::WriteOnly));
    helper.write(
        "#!/usr/bin/env python3\nimport "
        "sys,json\nprint(json.dumps({'event':'connected'}),flush=True)\nfor "
        "line in sys.stdin:\n c=json.loads(line)\n if c['action']=='quit': "
        "break\n if c['action']=='upload':\n  "
        "print(json.dumps({'event':'upload_done','path':c['path'],'status':1 "
        "if c['path'].endswith('a.md') else 0}),flush=True)\n");
    helper.close();
    helper.setPermissions(QFile::ReadOwner | QFile::WriteOwner |
                          QFile::ExeOwner);
    qputenv("WETYPE_ENGINE_HOST", helper.fileName().toUtf8());
    qputenv("WETYPE_DATA_ROOT", temp.filePath("data").toUtf8());
    qputenv("WETYPE_TRANSFER_DOWNLOAD_DIR",
            temp.filePath("downloads").toUtf8());
    TransferWindow w;
    for (const auto &name : {"a.md", "b.md"}) {
      QFile f(temp.filePath(name));
      QVERIFY(f.open(QIODevice::WriteOnly));
      f.write("synthetic");
      f.close();
      w.addFiles({f.fileName()});
    }
    w.peers_->addItem("Synthetic iPhone", "456");
    w.peers_->setCurrentIndex(1);
    w.begin(false, true);
    QTRY_COMPARE(w.tasks_->topLevelItemCount(), 2);
    QTRY_VERIFY(w.currentUpload_.isEmpty());
    QVERIFY(w.tasks_->topLevelItem(0)->text(2).startsWith("失败"));
    QVERIFY(w.tasks_->topLevelItem(1)->text(2).contains("报告完成"));
    w.stopSession("test complete");
    QTRY_COMPARE(w.session_.state(), QProcess::NotRunning);
    qunsetenv("WETYPE_ENGINE_HOST");
    qunsetenv("WETYPE_DATA_ROOT");
    qunsetenv("WETYPE_TRANSFER_DOWNLOAD_DIR");
  }
  void stalledTransferExpiresWithoutReportingSuccess() {
    TransferWindow w;
    w.active_ = true;
    w.connected_ = true;
    w.recordTask(
        {{"event", "download_start"}, {"path", "/synthetic/stalled.pdf"}});
    w.activity_.start(1);
    QTRY_VERIFY(!w.active_);
    QVERIFY(w.tasks_->topLevelItem(0)->text(2).contains("中断"));
    QVERIFY(w.status_->text().contains("没有进展"));
  }
  void screenshotSyntheticWindow() {
    const auto path = qEnvironmentVariable("WETYPE_TEST_GUI_SCREENSHOT");
    if (path.isEmpty())
      QSKIP("Optional synthetic review screenshot");
    wetype::applyDesktopStyle(
        *qobject_cast<QApplication *>(QCoreApplication::instance()));
    qputenv("WETYPE_TRANSFER_DOWNLOAD_DIR", "/tmp/WeTypeX-test-downloads");
    TransferWindow w;
    w.peers_->clear();
    w.peers_->addItem("Mac · synthetic test device", "123");
    w.recordTask({{"event", "upload_done"},
                  {"path", "/synthetic/example.md"},
                  {"status", 0}});
    w.show();
    QTest::qWait(100);
    QVERIFY(w.grab().save(path));
  }
};
QTEST_MAIN(TransferTests)
#include "transfer-gui.moc"
