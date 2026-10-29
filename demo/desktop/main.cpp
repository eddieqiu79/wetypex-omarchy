// SPDX-License-Identifier: GPL-3.0-only
#include "window.hpp"
#include <QApplication>
#include <QCommandLineParser>
#include <QListWidget>
#include <QTimer>

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  app.setApplicationName("wetypex-desktop-demo");
  app.setDesktopFileName("org.wetypex.Omarchy.Demo");
  QCommandLineParser parser;
  parser.setApplicationDescription("Isolated WeTypeX desktop UX demo. No real settings, network or transfers.");
  parser.addHelpOption();
  parser.addOption({"screenshot", "Save a synthetic UI preview and exit.", "path"});
  parser.addOption({"page", "Open page 1..8 (transfer, input, voice, phrases, appearance, shortcuts, devices, updates).", "number", "1"});
  parser.process(app);
  bool validPage = false;
  const int page = parser.value("page").toInt(&validPage);
  if (!validPage || page < 1 || page > 8) parser.showHelp(2);
  DemoWindow window;
  window.findChild<QListWidget *>("navigation")->setCurrentRow(page - 1);
  window.show();
  if (parser.isSet("screenshot")) {
    QTimer::singleShot(200, &app, [&] {
      const bool ok = window.grab().save(parser.value("screenshot"));
      app.exit(ok ? 0 : 1);
    });
  }
  return app.exec();
}
