// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "model.hpp"
#include <QWidget>
#include <QStringList>
class QListWidget;
class QLabel;
class QPushButton;
class QVBoxLayout;
class QDragEnterEvent;
class QDropEvent;

class TransferPage : public QWidget {
  Q_OBJECT
public:
  explicit TransferPage(demo::Model &model, QWidget *parent = nullptr);
  void pickFiles();
  void sendFiles();
  void refresh();
  void addPaths(const QStringList &paths);
protected:
  void dragEnterEvent(QDragEnterEvent *) override;
  void dropEvent(QDropEvent *) override;
private:
  struct File { QString name, identity; qint64 size; };
  demo::Model &model;
  QVector<File> pending;
  QString selectedPeer;
  QListWidget *files, *queue;
  QLabel *summary, *notice, *empty, *detail;
  QPushButton *send, *accept, *reject, *cancel, *retry, *fail;
  QWidget *taskActions;
  int selectedJob() const;
  void refreshFiles();
  void addIncoming();
};
