// SPDX-License-Identifier: GPL-3.0-only
// Exercise the actual application entry point with an isolated XDG environment.
#include <QApplication>
#include <QFile>
#include <QListWidget>
#include <QLabel>
#include <QStackedWidget>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QWidget>
class SnapshotApplication : public QApplication {
public:
  static inline bool exercised = false;
  SnapshotApplication(int &argc, char **argv) : QApplication(argc, argv) {
    QTimer::singleShot(500, this, [this] {
      for (auto *w : topLevelWidgets()) {
        if (w->windowTitle() != "WeTypeX" && w->windowTitle() != "WeTypeX · 开发构建")
          continue;
        w->resize(960, 720);
        auto *nav = w->findChild<QListWidget *>("navigation");
        if (!nav || nav->count() != 8 ||
            (w->windowFlags() & Qt::FramelessWindowHint)) {
          exit(2);
          return;
        }
        QTimer::singleShot(100, this, [this, w, nav] {
          const auto path = qEnvironmentVariable("WETYPE_TEST_GUI_SCREENSHOT");
          if (!path.isEmpty() && !w->grab().save(path)) {
            exit(3);
            return;
          }
          // Exercise all real pages at common tiled widths. Content wider
          // than the viewport must remain reachable, never silently clipped.
          for (int width : {480, 600, 720, 960, 1280}) {
            for (int height : {320, 540}) {
              w->resize(width, height);
              for (int page = 0; page < nav->count(); ++page) {
                nav->setCurrentRow(page);
                nav->scrollToItem(nav->item(page));
                processEvents();
                if (w->width() != width || w->height() != height) { exit(5); return; }
                if (!nav->viewport()->rect().contains(nav->visualItemRect(nav->item(page)).center())) {
                  exit(9); return;
                }
                auto *brand = w->findChild<QLabel *>("brand");
                if (!brand || brand->width() < brand->fontMetrics().horizontalAdvance(brand->text())) {
                  exit(8); return;
                }
                for (auto *scroll : w->findChildren<QScrollArea *>("pageScroll")) {
                  if (!scroll->isVisible()) continue;
                  if (scroll->widget()->width() > scroll->viewport()->width() &&
                      (scroll->horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOff ||
                       scroll->horizontalScrollBar()->maximum() <= 0)) {
                    exit(6); return;
                  }
                }
              }
            }
            if (!path.isEmpty()) {
              nav->setCurrentRow(4);
              processEvents();
              if (!w->grab().save(path + QString(".%1.png").arg(width))) {
                exit(3); return;
              }
            }
          }
          w->resize(960, 720);
          nav->setCurrentRow(7);
          QTimer::singleShot(50, this, [this, w, path] {
            if (!path.isEmpty() && !w->grab().save(path + ".updates.png")) {
              exit(4);
              return;
            }
            exercised = true;
            exit(0);
          });
        });
        return;
      }
      exit(1);
    });
  }
};
// Single-instance IPC has its own test. This harness must still open the
// actual UI on hosts whose test sandbox cannot create Unix-domain sockets.
#define WETYPE_SETTINGS_WINDOW_TEST
#define QApplication SnapshotApplication
#define main settingsSnapshotMain
#include "../src/settings/main.cpp"
#undef main
#undef QApplication
int main(int argc, char **argv) {
  const int result = settingsSnapshotMain(argc, argv);
  return !result && !SnapshotApplication::exercised ? 7 : result;
}
