// SPDX-License-Identifier: GPL-3.0-only
#include "window.hpp"
#include "transfer_page.hpp"
#include <QApplication>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QFile>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QListWidget>
#include <QPushButton>
#include <QTreeWidget>
#include <QtTest>

using demo::State;

class DesktopDemoTests : public QObject {
  Q_OBJECT
private slots:
  void waitingRequiresExplicitAcceptance() {
    demo::Model model;
    const int out = model.add("notes.md", "Demo Mac");
    const int in = model.add("sample.pdf", "Demo iPhone", true);
    for (int i = 0; i < 30; ++i) model.tick();
    QCOMPARE(model.jobs[out].state, State::Waiting);
    QCOMPARE(model.jobs[in].state, State::Offered);
    QCOMPARE(model.jobs[out].progress, 0);
    QCOMPARE(model.jobs[in].progress, 0);
    QVERIFY(model.accept(out));
    for (int i = 0; i < 10; ++i) model.tick();
    QCOMPARE(model.jobs[out].state, State::Complete);
    QCOMPARE(model.jobs[out].progress, 100);
    QCOMPARE(model.jobs[in].state, State::Offered);
    QVERIFY(!model.cancel(out));
    QVERIFY(!model.retry(out));
    QVERIFY(demo::stateText(State::Complete).contains("未实际传输"));
  }

  void cancellationIsTerminalUntilRetry() {
    demo::Model model;
    const int id = model.add("notes.md", "Demo Mac");
    QVERIFY(model.accept(id));
    model.tick();
    model.tick();
    QVERIFY(model.jobs[id].progress > 0);
    QVERIFY(model.cancel(id));
    const int progress = model.jobs[id].progress;
    for (int i = 0; i < 10; ++i) model.tick();
    QCOMPARE(model.jobs[id].state, State::Cancelled);
    QCOMPARE(model.jobs[id].progress, progress);
    QVERIFY(!model.accept(id));
    QVERIFY(!model.fail(id));
    QVERIFY(model.retry(id));
    QCOMPARE(model.jobs[id].state, State::Waiting);
    QCOMPARE(model.jobs[id].progress, 0);
    model.tick();
    QCOMPARE(model.jobs[id].state, State::Waiting);
  }

  void incomingAcceptRejectAndFailure() {
    demo::Model model;
    const int id = model.add("sample.pdf", "Demo iPhone", true);
    QVERIFY(model.reject(id));
    model.tick();
    QCOMPARE(model.jobs[id].state, State::Rejected);
    QVERIFY(!model.accept(id));
    QVERIFY(model.retry(id));
    QCOMPARE(model.jobs[id].state, State::Offered);
    QCOMPARE(model.jobs[id].progress, 0);
    QVERIFY(model.accept(id));
    model.tick();
    model.tick();
    QVERIFY(model.fail(id));
    const int progress = model.jobs[id].progress;
    model.tick();
    QCOMPARE(model.jobs[id].state, State::Failed);
    QCOMPARE(model.jobs[id].progress, progress);
    QVERIFY(model.retry(id));
    QCOMPARE(model.jobs[id].state, State::Offered);
    QCOMPARE(model.jobs[id].progress, 0);
  }

  void jobsAreIndependentAndInvalidActionsDoNothing() {
    demo::Model model;
    QCOMPARE(model.add("   ", "Demo Mac"), -1);
    QCOMPARE(model.add("notes.md", ""), -1);
    QVERIFY(model.jobs.isEmpty());
    const int first = model.add("notes.md", "Demo Mac");
    const int second = model.add("sample.pdf", "Demo iPhone");
    for (int id : {-1, 2, 999}) {
      QVERIFY(!model.accept(id));
      QVERIFY(!model.reject(id));
      QVERIFY(!model.cancel(id));
      QVERIFY(!model.fail(id));
      QVERIFY(!model.retry(id));
    }
    QVERIFY(model.accept(first));
    QVERIFY(model.accept(second));
    model.tick();
    QVERIFY(model.cancel(first));
    for (int i = 0; i < 10; ++i) model.tick();
    QCOMPARE(model.jobs[first].state, State::Cancelled);
    QCOMPARE(model.jobs[first].progress, 0);
    QCOMPARE(model.jobs[second].state, State::Complete);
    QCOMPARE(model.jobs[second].progress, 100);
  }

  void windowUsesNormalResizableSemanticsAndKeyboardNavigation() {
    DemoWindow window(false);
    QVERIFY(!(window.windowFlags() & Qt::FramelessWindowHint));
    QVERIFY(window.minimumWidth() < window.maximumWidth());
    QVERIFY(window.minimumHeight() < window.maximumHeight());
    auto *navigation = window.findChild<QListWidget *>("navigation");
    QVERIFY(navigation);
    QCOMPARE(navigation->count(), 8);
    window.show();
    window.activateWindow();
    QTRY_VERIFY(window.isActiveWindow());
    // Start with a different page so Ctrl+1 is checked as an actual action.
    navigation->setCurrentRow(7);
    for (int page = 0; page < 8; ++page) {
      QTest::keyClick(&window, Qt::Key(Qt::Key_1 + page), Qt::ControlModifier);
      QTRY_COMPARE(navigation->currentRow(), page);
    }
  }

  void queueActionsTrackTheSelectedJob() {
    DemoWindow window(false);
    auto *send = window.findChild<QPushButton *>("sendButton");
    auto *peer = window.findChild<QPushButton *>("peerMac");
    auto *sample = window.findChild<QPushButton *>("sampleFiles");
    auto *queue = window.findChild<QListWidget *>("transferQueue");
    auto *accept = window.findChild<QPushButton *>("acceptButton");
    auto *reject = window.findChild<QPushButton *>("rejectButton");
    auto *cancel = window.findChild<QPushButton *>("cancelButton");
    auto *retry = window.findChild<QPushButton *>("retryButton");
    auto *fail = window.findChild<QPushButton *>("failButton");
    auto *incoming = window.findChild<QPushButton *>("incomingButton");
    QVERIFY(send && peer && queue && accept && reject && cancel && retry &&
            fail && incoming);
    QVERIFY(sample);
    QVERIFY(!send->isEnabled());
    peer->click();
    sample->click();
    const int initial = window.model.jobs.size();
    QVERIFY(send->isEnabled());
    send->click();
    QCOMPARE(window.model.jobs.size(), initial + 1);
    QCOMPARE(queue->count(), initial + 1);
    QVERIFY(queue->currentItem());
    QCOMPARE(window.model.jobs.last().state, State::Waiting);
    QVERIFY(accept->isEnabled());
    QVERIFY(reject->isEnabled());
    QVERIFY(cancel->isEnabled());
    QVERIFY(!retry->isEnabled());
    cancel->click();
    QCOMPARE(window.model.jobs.last().state, State::Cancelled);
    QVERIFY(!accept->isEnabled());
    QVERIFY(!reject->isEnabled());
    QVERIFY(!cancel->isEnabled());
    QVERIFY(!fail->isEnabled());
    QVERIFY(retry->isEnabled());
    retry->click();
    QCOMPARE(window.model.jobs.last().state, State::Waiting);
    QVERIFY(accept->isEnabled());
    accept->click();
    window.model.tick();
    window.refresh();
    QVERIFY(!accept->isEnabled());
    QVERIFY(!reject->isEnabled());
    QVERIFY(cancel->isEnabled());
    QVERIFY(fail->isEnabled());
    const int outgoing = window.model.jobs.size() - 1;
    incoming->click();
    QCOMPARE(window.model.jobs.size(), initial + 2);
    QVERIFY(window.model.jobs.last().incoming);
    QCOMPARE(window.model.jobs.last().state, State::Offered);
    QVERIFY(accept->isEnabled());
    QVERIFY(reject->isEnabled());
    reject->click();
    QCOMPARE(window.model.jobs.last().state, State::Rejected);
    QCOMPARE(window.model.jobs[outgoing].state, State::Transferring);
    QVERIFY(retry->isEnabled());
    for (int i = 0; i < 10; ++i) window.model.tick();
    window.refresh();
    QCOMPARE(window.model.jobs[outgoing].state, State::Complete);
    QCOMPARE(window.model.jobs.last().state, State::Rejected);
  }

  void settingsApplyResetAndModeDependencies() {
    DemoWindow window(false);
    auto *mode = window.findChild<QComboBox *>("input_mode");
    auto *scheme = window.findChild<QComboBox *>("double_pinyin_scheme");
    auto *wubi = window.findChild<QComboBox *>("wubi_solution");
    auto *smart = window.findChild<QCheckBox *>("smart_input");
    auto *apply = window.findChild<QPushButton *>("input_apply");
    auto *reset = window.findChild<QPushButton *>("input_reset");
    QVERIFY(mode && scheme && wubi && smart && apply && reset);
    QVERIFY(!scheme->isEnabled());
    QVERIFY(!wubi->isEnabled());
    mode->setCurrentIndex(1);
    QVERIFY(scheme->isEnabled());
    QVERIFY(!wubi->isEnabled());
    smart->setChecked(false);
    apply->click();
    mode->setCurrentIndex(2);
    smart->setChecked(true);
    QVERIFY(wubi->isEnabled());
    reset->click();
    QCOMPARE(mode->currentIndex(), 1);
    QVERIFY(!smart->isChecked());
    QVERIFY(scheme->isEnabled());
    QVERIFY(!wubi->isEnabled());
    const auto fuzzy = window.findChildren<QCheckBox *>(QRegularExpression("^fuzzy_"));
    QCOMPARE(fuzzy.size(), 19);
    for (const QString &key : {"fuzzy_z_zh", "fuzzy_c_ch", "fuzzy_s_sh", "fuzzy_in_ing"}) {
      auto *check = window.findChild<QCheckBox *>(key);
      QVERIFY(check && check->isChecked());
    }
    // A new demo session has its own presets, not saved live-engine state.
    DemoWindow next(false);
    QVERIFY(next.findChild<QCheckBox *>("smart_input")->isChecked());
  }

  void fileSelectionAndDropRequireExplicitSend() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString first=directory.filePath("notes.md"), second=directory.filePath("guide.pdf");
    for (const auto &path : {first,second}) {
      QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly));
      QCOMPARE(file.write("synthetic test bytes"),20);
    }
    DemoWindow window(false);
    auto *page=window.findChild<TransferPage *>();
    auto *files=window.findChild<QListWidget *>("pendingFiles");
    auto *send=window.findChild<QPushButton *>("sendButton");
    QVERIFY(page && files && send);
    page->addPaths({first,first,directory.path()});
    QCOMPARE(files->count(),1);
    QVERIFY(window.model.jobs.isEmpty());
    QVERIFY(!send->isEnabled());
    QMimeData mime;
    mime.setUrls({QUrl::fromLocalFile(second),QUrl("https://example.invalid/private.pdf")});
    QDragEnterEvent enter(QPoint(20,20),Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(page,&enter);
    QVERIFY(enter.isAccepted());
    QDropEvent drop(QPointF(20,20),Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(page,&drop);
    QVERIFY(drop.isAccepted());
    QCOMPARE(files->count(),2);
    QVERIFY(window.model.jobs.isEmpty());
    window.findChild<QPushButton *>("peerPhone")->click();
    QVERIFY(send->isEnabled());
    send->click();
    QCOMPARE(window.model.jobs.size(),1);
    QCOMPARE(window.model.jobs[0].files,QStringList({"notes.md","guide.pdf"}));
    QCOMPARE(window.model.jobs[0].bytes,40);
    QCOMPARE(window.model.jobs[0].peer,QString("iPhone"));
    QCOMPARE(window.model.jobs[0].state,State::Waiting);
    QCOMPARE(files->count(),0);
    QVERIFY(!send->isEnabled());
  }

  void candidatePreviewAndPhraseRevert() {
    DemoWindow window(false);
    auto *count = window.findChild<QSpinBox *>("page_size");
    auto *vertical = window.findChild<QCheckBox *>("vertical");
    auto *preview = window.findChild<QLabel *>("candidatePreview");
    QVERIFY(count && vertical && preview);
    count->setValue(3);
    vertical->setChecked(true);
    QCOMPARE(preview->text().split('\n').size(), 3);
    QVERIFY(!preview->text().contains("4"));
    auto *phrases = window.findChild<QListWidget *>("hotwords");
    QVERIFY(phrases && phrases->count() == 1);
    const auto original = phrases->item(0)->text();
    window.findChild<QPushButton *>("deletePhrase")->click();
    QCOMPARE(phrases->count(), 0);
    window.findChild<QPushButton *>("phrases_reset")->click();
    QCOMPARE(phrases->count(), 1);
    QCOMPARE(phrases->item(0)->text(), original);
  }
};

QTEST_MAIN(DesktopDemoTests)
#include "tests.moc"
