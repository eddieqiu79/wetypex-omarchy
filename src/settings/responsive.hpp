// SPDX-License-Identifier: MIT
#pragma once
#include <QBoxLayout>
#include <QEvent>
#include <QResizeEvent>
#include <QWidget>

namespace original_ui {
// Preserve complete controls while allowing groups to wrap at any width.
class FlowLayout final : public QLayout {
  QList<QLayoutItem *> items_;
  int arrange(const QRect &rect, bool apply) const {
    const auto margins = contentsMargins();
    const auto area = rect.marginsRemoved(margins);
    int x = area.x(), y = area.y(), lineHeight = 0;
    for (auto *item : items_) {
      const auto size = item->sizeHint().expandedTo(item->minimumSize());
      if (x > area.x() && x + size.width() > area.x() + area.width()) {
        x = area.x(); y += lineHeight + spacing(); lineHeight = 0;
      }
      if (apply) item->setGeometry(QRect(QPoint(x, y), size));
      x += size.width() + spacing();
      lineHeight = qMax(lineHeight, size.height());
    }
    return y - rect.y() + lineHeight + margins.bottom();
  }
public:
  explicit FlowLayout(QWidget *parent) : QLayout(parent) {
    setContentsMargins(0, 0, 0, 0);
    setSpacing(6);
  }
  ~FlowLayout() override { while (auto *item = takeAt(0)) delete item; }
  void addItem(QLayoutItem *item) override { items_.append(item); invalidate(); }
  int count() const override { return items_.size(); }
  QLayoutItem *itemAt(int index) const override { return items_.value(index); }
  QLayoutItem *takeAt(int index) override {
    return index >= 0 && index < items_.size() ? items_.takeAt(index) : nullptr;
  }
  Qt::Orientations expandingDirections() const override { return {}; }
  bool hasHeightForWidth() const override { return true; }
  int heightForWidth(int width) const override { return arrange(QRect(0, 0, width, 0), false); }
  QSize minimumSize() const override {
    QSize result;
    for (auto *item : items_) result = result.expandedTo(item->minimumSize());
    const auto m = contentsMargins();
    return result + QSize(m.left() + m.right(), m.top() + m.bottom());
  }
  QSize sizeHint() const override {
    QSize result(0, 0);
    for (auto *item : items_) {
      const auto size = item->sizeHint();
      if (result.width()) result.rwidth() += spacing();
      result.rwidth() += size.width();
      result.setHeight(qMax(result.height(), size.height()));
    }
    const auto m = contentsMargins();
    return result + QSize(m.left() + m.right(), m.top() + m.bottom());
  }
  void setGeometry(const QRect &rect) override {
    QLayout::setGeometry(rect);
    arrange(rect, true);
  }
};
// Choose a direction from current content hints, not monitor models or widths.
// LayoutRequest also covers text/font/style changes without a window resize.
class AdaptiveRow : public QWidget {
  int baseMinimumHeight_ = -1;
  int preferredWidth() const {
    auto *box = qobject_cast<QBoxLayout *>(layout());
    if (!box) return QWidget::sizeHint().width();
    const auto margins = box->contentsMargins();
    int preferred = margins.left() + margins.right();
    for (int i = 0; i < box->count(); ++i) {
      preferred += box->itemAt(i)->sizeHint().width();
      if (i) preferred += qMax(0, box->spacing());
    }
    return preferred;
  }
  void reflow() {
    auto *box = qobject_cast<QBoxLayout *>(layout());
    if (!box || box->count() < 2) return;
    if (baseMinimumHeight_ < 0) baseMinimumHeight_ = minimumHeight();
    const auto direction = width() < preferredWidth() ? QBoxLayout::TopToBottom
                                              : QBoxLayout::LeftToRight;
    if (box->direction() != direction) {
      box->setDirection(direction);
      updateGeometry();
    }
    // The parent may be a scrollable card with a bounded size policy. Publish
    // the wrapped height explicitly rather than letting it keep the old row
    // height after a direction or font change.
    const int needed = qMax(baseMinimumHeight_, box->totalHeightForWidth(width()));
    if (needed != minimumHeight()) setMinimumHeight(needed);
  }
public:
  QSize sizeHint() const override {
    auto hint = QWidget::sizeHint();
    // Keep the width preference independent of the current arrangement, so
    // parents do not alternate between layouts as this row wraps.
    hint.setWidth(preferredWidth());
    return hint;
  }
protected:
  void resizeEvent(QResizeEvent *event) override {
    QWidget::resizeEvent(event);
    reflow();
  }
  bool event(QEvent *event) override {
    const bool result = QWidget::event(event);
    if (event->type() == QEvent::LayoutRequest || event->type() == QEvent::FontChange ||
        event->type() == QEvent::StyleChange) reflow();
    return result;
  }
};
} // namespace original_ui
