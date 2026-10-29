// SPDX-License-Identifier: GPL-3.0-only
// Exercise the actual application entry point with an isolated XDG environment.
#include <QApplication>
#include <QFile>
#include <QListWidget>
#include <QStackedWidget>
#include <QTimer>
#include <QWidget>
class SnapshotApplication : public QApplication {
public:
  SnapshotApplication(int &argc, char **argv) : QApplication(argc, argv) {
    QTimer::singleShot(500, this, [this] {
      for (auto *w : topLevelWidgets()) {
        if (w->windowTitle() != "WeTypeX")
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
          nav->setCurrentRow(7);
          QTimer::singleShot(50, this, [this, w, path] {
            if (!path.isEmpty() && !w->grab().save(path + ".updates.png")) {
              exit(4);
              return;
            }
            exit(0);
          });
        });
        return;
      }
      exit(1);
    });
  }
};
#define QApplication SnapshotApplication
#include "../src/settings/main.cpp"
