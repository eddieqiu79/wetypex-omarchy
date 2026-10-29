// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QString>
class QWidget;
QWidget *makeSettingsPage(const QString &category, QWidget *parent = nullptr);
