// SPDX-License-Identifier: GPL-3.0-only
#define WETYPE_GUI_TEST
#include "../src/transfer/main.cpp"
#include <QTemporaryDir>
#include <QtTest>

class TransferTests : public QObject {
  Q_OBJECT
private slots:
  void deviceListLoadsFromInvitationPollAndKeepsSelection() {
    TransferWindow w;
    const QJsonArray devices{QJsonObject{{"uin", 1}, {"name", "Linux"}},
                            QJsonObject{{"uin", 42}, {"name", "Synthetic Mac"}},
                            QJsonObject{{"uin", 43}, {"name", "Synthetic Phone"}}};
    w.updatePeerList({{"self_uin", 1}, {"devices", devices}});
    QCOMPARE(w.peers_->count(), 2);
    QCOMPARE(w.peers_->currentText(), QString("Synthetic Mac"));
    QVERIFY(w.refresh_->isEnabled());
    QVERIFY(w.connect_->isEnabled());
    w.peers_->setCurrentIndex(1);
    w.updatePeerList({{"self_uin", 1}, {"devices", devices}});
    QCOMPARE(w.peers_->currentData().toString(), QString("43"));
  }
  void canceledPeerEndsWaitImmediately() {
    for (int state : {1, 2, 4}) {
      TransferWindow w;
      w.active_ = true;
      w.incoming_ = true;
      w.boundUin_ = "42";
      w.poll_.start();
      w.handlePeer({{"ok", true}, {"dispatch_buf", "c3ludGhldGlj"},
                    {"peer", QJsonObject{{"uin", 42}, {"state", state}}}});
      QVERIFY(!w.active_);
      QVERIFY(!w.poll_.isActive());
      QVERIFY(w.refresh_->isEnabled());
      QVERIFY(w.lastDispatch_.isEmpty());
    }
  }
  void incomingInviteJoinsOriginalSession() {
    QTemporaryDir temp;
    const auto log = temp.filePath("commands");
    QFile account(temp.filePath("account"));
    QVERIFY(account.open(QIODevice::WriteOnly));
    account.write(("#!/usr/bin/env python3\nimport sys,json\n"
        "with open(" + QString("'%1'").arg(log) + ", 'a') as f: f.write(sys.argv[1]+'\\n')\n"
        "print(json.dumps({'ok':True,'peer':{'uin':42,'public_cer':'c3ludGhldGlj'}}))\n").toUtf8());
    account.close();
    account.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    QFile backend(temp.filePath("backend"));
    QVERIFY(backend.open(QIODevice::WriteOnly));
    backend.write("#!/usr/bin/env python3\nimport sys,json\n"
        "print(json.dumps({'event':'ready','transport':'wxp2p','ca_cert':'synthetic','server_name':'test'}),flush=True)\n"
        "for line in sys.stdin:\n"
        " c=json.loads(line)\n"
        " if c['action']=='quit': break\n"
        " if c['action']=='server': print(json.dumps({'event':'connected'}),flush=True)\n");
    backend.close();
    backend.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    qputenv("WETYPE_ACCOUNT_TOOL", account.fileName().toUtf8());
    qputenv("WETYPE_ENGINE_HOST", backend.fileName().toUtf8());
    qputenv("WETYPE_DATA_ROOT", temp.filePath("data").toUtf8());
    qputenv("WETYPE_TRANSFER_DOWNLOAD_DIR", temp.filePath("downloads").toUtf8());
    {
      TransferWindow w;
      QVERIFY(!w.active_);
      w.acceptInvitation({{"transfer_code", "synthetic-invitation"}, {"sender_uin", 42}});
      QTRY_VERIFY(w.connected_);
      QCOMPARE(w.transferCode_, QString("synthetic-invitation"));
      QVERIFY(w.incoming_);
      QCOMPARE(w.peers_->currentData().toString(), QString("42"));
      QVERIFY(w.outgoing_.isEmpty());
      QFile commands(log);
      QVERIFY(commands.open(QIODevice::ReadOnly));
      QCOMPARE(commands.readAll(), QByteArray("p2p-update\n"));
      w.stopSession("test complete");
      QTRY_COMPARE(w.session_.state(), QProcess::NotRunning);
    }
    qunsetenv("WETYPE_ACCOUNT_TOOL");
    qunsetenv("WETYPE_ENGINE_HOST");
    qunsetenv("WETYPE_DATA_ROOT");
    qunsetenv("WETYPE_TRANSFER_DOWNLOAD_DIR");
  }
  void incomingPeerMustMatchInvitation() {
    TransferWindow w;
    w.active_ = true;
    w.incoming_ = true;
    w.boundUin_ = "42";
    w.handlePeer({{"ok", true}, {"peer", QJsonObject{}}});
    QVERIFY(w.active_); // Wait for identity, do not consume partial response.
    w.handlePeer({{"ok", true}, {"peer", QJsonObject{{"uin", 43}}}});
    QVERIFY(!w.active_);
    QVERIFY(w.status_->text().contains("身份"));
  }
  void lateDispatchDoesNotSwitchDirectConnection() {
    TransferWindow w;
    w.active_ = true;
    w.incoming_ = true;
    w.boundUin_ = "42";
    w.directStarted_ = true;
    w.handlePeer({{"ok", true}, {"dispatch_buf", "c3ludGhldGlj"},
                  {"peer", QJsonObject{{"uin", 42}, {"public_cer", "c3ludGhldGlj"}}}});
    QVERIFY(w.active_);
    QVERIFY(w.lastDispatch_.isEmpty());
    QVERIFY(!w.wxpStarted_);
  }
  void declinedInviteDoesNotStartBackendOrRepeatPrompt() {
    QTemporaryDir temp;
    QFile account(temp.filePath("account"));
    QVERIFY(account.open(QIODevice::WriteOnly));
    account.write("#!/usr/bin/env python3\nimport json\nprint(json.dumps({'ok':True,'self_uin':1,'devices':[{'uin':1,'name':'Linux'},{'uin':42,'name':'Synthetic Mac'}],'pending_invite':{'transfer_code':'synthetic','sender_uin':42,'device_name':'Synthetic Mac'}}))\n");
    account.close();
    account.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    qputenv("WETYPE_ACCOUNT_TOOL", account.fileName().toUtf8());
    {
      TransferWindow w;
      w.pollInvitations();
      QTRY_VERIFY(w.invitationPrompt_);
      QCOMPARE(w.peers_->currentText(), QString("Synthetic Mac"));
      QCOMPARE(w.peers_->count(), 1);
      QVERIFY(!w.active_);
      QCOMPARE(w.session_.state(), QProcess::NotRunning);
      auto *prompt = w.findChild<QMessageBox *>();
      QVERIFY(prompt);
      prompt->done(QMessageBox::No);
      QVERIFY(!w.invitationPrompt_);
      QVERIFY(!w.active_);
      w.pollInvitations();
      QTRY_VERIFY(!w.accountBusy_);
      QVERIFY(!w.invitationPrompt_);
      QVERIFY(!w.active_);
    }
    qunsetenv("WETYPE_ACCOUNT_TOOL");
  }
  void phoneCredentialsCanArriveAfterDispatch() {
    QTemporaryDir temp;
    qputenv("WETYPE_DATA_ROOT", temp.path().toUtf8());
    const auto state = temp.filePath("state/transfer");
    QVERIFY(QDir().mkpath(state));
    TransferWindow w;
    w.active_ = true;
    w.preloadedWxp2p_ = true;
    // No network or real certificates: just keep the synthetic backend alive
    // until stopSession sends quit.
    w.session_.start("python3", {"-u", "-c",
        "import sys,json\nfor line in sys.stdin:\n"
        " if json.loads(line).get('action') == 'quit': break\n"});
    QVERIFY(w.session_.waitForStarted());
    const QByteArray dispatch("synthetic-dispatch");
    w.startWxp2p(dispatch, {});
    QVERIFY(w.status_->text().contains("等待手机返回"));
    QVERIFY(w.lastDispatch_.isEmpty());
    QVERIFY(!w.wxpStarted_);
    QVERIFY(!QFile::exists(state + "/wxp2p-dispatch.bin"));
    w.startWxp2p(dispatch, {{"public_cer", "invalid!base64"}});
    QVERIFY(w.status_->text().contains("格式无效"));
    QVERIFY(w.lastDispatch_.isEmpty());
    const QByteArray certificate("synthetic-certificate-not-for-TLS");
    const QJsonObject peer{{"public_cer", QString::fromLatin1(certificate.toBase64())}};
    // Force a real local write failure, then recover using the same dispatch.
    const auto caPath = state + "/peer-ca.pem";
    QVERIFY(QDir().mkdir(caPath));
    w.startWxp2p(dispatch, peer);
    QVERIFY(w.status_->text().contains("本地保存失败"));
    QVERIFY(w.lastDispatch_.isEmpty());
    QVERIFY(!w.wxpStarted_);
    QVERIFY(QDir().rmdir(caPath));
    w.startWxp2p(dispatch, peer);
    QCOMPARE(w.lastDispatch_, dispatch);
    QVERIFY(w.wxpStarted_);
    QFile ca(caPath);
    QVERIFY(ca.open(QIODevice::ReadOnly));
    QCOMPARE(ca.readAll(), certificate);
    QVERIFY(!(ca.permissions() & (QFileDevice::ReadGroup | QFileDevice::WriteGroup |
                                  QFileDevice::ReadOther | QFileDevice::WriteOther)));
    const auto status = w.status_->text();
    w.startWxp2p(dispatch, {});
    QCOMPARE(w.status_->text(), status); // Already launched: ignore duplicates.
    w.stopSession("test complete");
    QTRY_COMPARE(w.session_.state(), QProcess::NotRunning);
    qunsetenv("WETYPE_DATA_ROOT");
  }
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
