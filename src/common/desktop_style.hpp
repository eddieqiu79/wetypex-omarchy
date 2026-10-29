// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QPalette>
#include <QString>
namespace wetype {
inline void applyDesktopStyle(QApplication &app) {
  app.setStyle("Fusion");
  QPalette p;
  p.setColor(QPalette::Window, QColor("#282827"));
  p.setColor(QPalette::WindowText, QColor("#f0f0ed"));
  p.setColor(QPalette::Base, QColor("#242423"));
  p.setColor(QPalette::AlternateBase, QColor("#333332"));
  p.setColor(QPalette::Text, QColor("#f0f0ed"));
  p.setColor(QPalette::Button, QColor("#373736"));
  p.setColor(QPalette::ButtonText, QColor("#f0f0ed"));
  p.setColor(QPalette::Highlight, QColor("#24C891"));
  p.setColor(QPalette::HighlightedText, QColor("#10271e"));
  p.setColor(QPalette::Disabled, QPalette::Text, QColor("#92928e"));
  p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#92928e"));
  p.setColor(QPalette::ToolTipBase, QColor("#333332"));
  p.setColor(QPalette::ToolTipText, QColor("#f0f0ed"));
  app.setPalette(p);
  app.setStyleSheet(R"(
    QWidget { font-size:14px; }
    QWidget#windowSurface,QWidget#contentPage,QScrollArea#pageScroll { background:#282827; }
    QWidget#sidebar { background:#222221; }
    QLabel { background:transparent; }
    QLabel#brand,QLabel#pageHeading,QLabel#title { font-size:21px; font-weight:600; }
    QLabel#rowSubtitle,QLabel#hint { color:#b8b8b3; }
    QLabel#keyCap { background:#3a3a38; padding:2px 6px; border-radius:4px; }
    QFrame#card { background:#333332; border:1px solid #50504c; border-radius:9px; }
    QFrame#separator { background:#484845; border:0; }
    QListWidget#navigation { background:transparent; border:0; }
    QListWidget#navigation::item { min-height:42px; padding:2px 10px; border-radius:6px; }
    QListWidget#navigation::item:selected { background:#24C891; color:#10271e; }
    QPushButton,QToolButton,QComboBox { min-height:28px; padding:3px 12px; border:1px solid #5a5a55; border-radius:5px; background:#373736; }
    QPushButton:hover,QToolButton:hover { border-color:#24C891; }
    QPushButton:focus,QComboBox:focus,QLineEdit:focus,QPlainTextEdit:focus,QListWidget:focus { border:2px solid #24C891; }
    QPushButton#greenButton { background:#24C891; color:#10271e; border-color:#24C891; }
    QPushButton:disabled { color:#92928e; background:#333332; border-color:#484845; }
    QLineEdit,QPlainTextEdit,QListWidget,QTreeWidget { background:#242423; border:1px solid #50504c; border-radius:5px; padding:4px; }
    QRadioButton,QCheckBox { spacing:8px; }
    QTabBar::tab { padding:7px 14px; background:#333332; }
    QTabBar::tab:selected { color:#24C891; border-bottom:2px solid #24C891; }
    QProgressBar { background:#383836; border:0; min-height:10px; text-align:center; }
    QProgressBar::chunk { background:#24C891; }
    QScrollBar:vertical { width:12px; background:#282827; }
    QScrollBar::handle:vertical { background:#62625c; min-height:30px; border-radius:5px; }
  )");
}
} // namespace wetype
