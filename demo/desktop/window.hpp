// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "model.hpp"
#include <QMainWindow>
class QListWidget;
class QStackedWidget;
class TransferPage;
class QLabel;
class QComboBox;
class QPushButton;
class QCheckBox;

class DemoWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit DemoWindow(bool animate = true);
  demo::Model model;
  void refresh();
private:
  QListWidget *navigation;
  QStackedWidget *pages;
  TransferPage *transferPage;
  void pickFile();
  void sendFile();
};
