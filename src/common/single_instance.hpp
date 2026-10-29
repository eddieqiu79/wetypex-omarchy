// SPDX-License-Identifier: MIT
#pragma once
#include <QDir>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QStandardPaths>
#include <QWidget>
#include <memory>
namespace wetype {
// Separate settings/transfer instances, one per user's runtime directory.
class SingleInstance {
  QLocalServer server_;
  std::unique_ptr<QLockFile> lock_;

public:
  bool acquire(const QString &name, bool activateOnly = false) {
    const auto root =
        QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (root.isEmpty())
      return false;
    const auto path = root + "/wetypex-omarchy-" + name;
    QLocalSocket client;
    client.connectToServer(path);
    if (client.waitForConnected(300)) {
      client.write("activate\n");
      client.waitForBytesWritten(300);
      return false;
    }
    if (activateOnly)
      return false;
    lock_ = std::make_unique<QLockFile>(path + ".lock");
    if (!lock_->tryLock(0))
      return false;
    QLocalServer::removeServer(path);
    server_.setSocketOptions(QLocalServer::UserAccessOption);
    return server_.listen(path);
  }
  void bind(QWidget &window) {
    QObject::connect(&server_, &QLocalServer::newConnection, &window,
                     [this, &window] {
                       while (auto *client = server_.nextPendingConnection()) {
                         client->close();
                         client->deleteLater();
                       }
                       window.showNormal();
                       window.raise();
                       window.activateWindow();
                     });
  }
};
} // namespace wetype
