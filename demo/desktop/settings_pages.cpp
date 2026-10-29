// SPDX-License-Identifier: GPL-3.0-only
// Content adapted from the MIT-licensed WeTypeX settings interface. No runtime
// access: these independently implemented controls hold demonstration state only.
#include "settings_pages.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariantMap>
#include <functional>

namespace {
QLabel *label(const QString &value, const char *name = "") {
  auto *result = new QLabel(value);
  result->setObjectName(name);
  result->setTextFormat(Qt::PlainText);
  result->setWordWrap(true);
  return result;
}

class Page final : public QWidget {
public:
  struct Binding {
    QString key;
    std::function<QVariant()> read;
    std::function<void(const QVariant &)> write;
  };
  QVBoxLayout *body;
  QVector<Binding> bindings;
  QVariantMap saved;
  QLabel *status;
  bool ready = false;

  Page(const QString &title, QWidget *parent) : QWidget(parent) {
    body = new QVBoxLayout(this);
    body->setContentsMargins(26, 22, 26, 20);
    body->setSpacing(14);
    body->addWidget(label(title, "heading"));
    body->addWidget(label("沿用现有设置内容 · 当前值为演示预设，修改仅保存在本次窗口内存中", "muted"));
    status = label("演示预设尚未修改", "settingsStatus");
  }
  QVariantMap values() const {
    QVariantMap result;
    for (const auto &binding : bindings) result[binding.key] = binding.read();
    return result;
  }
  void changed() {
    if (!ready) return;
    status->setText(values() == saved ? "与演示已应用值一致" : "有未应用的演示修改");
  }
  void bind(const QString &key, std::function<QVariant()> read,
            std::function<void(const QVariant &)> write) {
    bindings.push_back({key, std::move(read), std::move(write)});
  }
  QVBoxLayout *group(const QString &title) {
    body->addWidget(label(title, "brand"));
    auto *frame = new QFrame;
    frame->setObjectName("card");
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(9);
    body->addWidget(frame);
    return layout;
  }
  void row(QVBoxLayout *group, const QString &title, QWidget *control,
           const QString &description = {}) {
    auto *rowWidget = new QWidget;
    rowWidget->setObjectName("settingRow");
    auto *layout = new QHBoxLayout(rowWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    auto *texts = new QVBoxLayout;
    auto *name = label(title);
    name->setBuddy(control);
    texts->addWidget(name);
    if (!description.isEmpty()) texts->addWidget(label(description, "muted"));
    layout->addLayout(texts, 1);
    control->setAccessibleName(title);
    layout->addWidget(control);
    group->addWidget(rowWidget);
  }
  QCheckBox *toggle(QVBoxLayout *group, const QString &title,
                    const QString &key, bool value,
                    const QString &description = {}) {
    auto *check = new QCheckBox;
    check->setObjectName(key);
    check->setToolTip("源码配置项：" + key + "；仅模拟，不调用引擎");
    check->setChecked(value);
    row(group, title, check, description);
    bind(key, [check] { return check->isChecked(); },
         [check](const QVariant &v) { check->setChecked(v.toBool()); });
    connect(check, &QCheckBox::toggled, this, [this] { changed(); });
    return check;
  }
  QComboBox *choice(QVBoxLayout *group, const QString &title,
                     const QString &key, const QStringList &items,
                     int value = 0, const QString &description = {}) {
    auto *combo = new QComboBox;
    combo->setObjectName(key);
    combo->addItems(items);
    combo->setCurrentIndex(value);
    combo->setToolTip("源码配置项：" + key + "；仅模拟，不写入真实配置");
    row(group, title, combo, description);
    bind(key, [combo] { return combo->currentIndex(); },
         [combo](const QVariant &v) { combo->setCurrentIndex(v.toInt()); });
    connect(combo, &QComboBox::currentIndexChanged, this, [this] { changed(); });
    return combo;
  }
  void finish() {
    saved = values();
    ready = true;
    body->addWidget(status);
    auto *actions = new QHBoxLayout;
    auto *apply = new QPushButton("应用到演示");
    apply->setObjectName("applySettings");
    auto *revert = new QPushButton("撤销未应用修改");
    revert->setObjectName("resetSettings");
    actions->addWidget(apply);
    actions->addWidget(revert);
    actions->addStretch();
    body->addLayout(actions);
    body->addStretch();
    connect(apply, &QPushButton::clicked, this, [this] {
      saved = values();
      status->setText("已应用到演示内存 · 现用输入法未改变");
    });
    connect(revert, &QPushButton::clicked, this, [this] {
      ready = false;
      for (const auto &binding : bindings) binding.write(saved.value(binding.key));
      ready = true;
      status->setText("未应用的演示修改已撤销");
    });
  }
};

void input(Page *p) {
  auto *modes = p->group("输入方式");
  auto *mode = p->choice(modes, "输入方式", "input_mode", {"拼音", "双拼", "五笔"});
  auto *scheme = p->choice(modes, "双拼方案", "double_pinyin_scheme",
                          {"自然码", "搜狗", "微软", "小鹤", "拼音加加", "紫光", "智能 ABC"});
  auto *wubi = p->choice(modes, "五笔方案", "wubi_solution", {"86 五笔", "98 五笔", "新世纪五笔"});
  QVector<QCheckBox *> wubiOptions;
  for (const auto &[title, key] : QList<QPair<QString, QString>>{
         {"五笔拼音混输", "wubi_pinyin"}, {"唯一候选自动上屏", "wubi_unique_commit"},
         {"输入下一码时上屏", "wubi_next_commit"}, {"通配符编码提示", "wubi_wildcard_comment"}})
    wubiOptions << p->toggle(modes, title, key, false);
  auto updateMode = [=] {
    scheme->setEnabled(mode->currentIndex() == 1);
    scheme->parentWidget()->setVisible(mode->currentIndex() == 1);
    wubi->setEnabled(mode->currentIndex() == 2);
    wubi->parentWidget()->setVisible(mode->currentIndex() == 2);
    for (auto *check : wubiOptions) {
      check->setEnabled(mode->currentIndex() == 2);
      check->parentWidget()->setVisible(mode->currentIndex() == 2);
    }
  };
  QObject::connect(mode, &QComboBox::currentIndexChanged, p, updateMode);
  updateMode();
  auto *smart = p->group("智能拼写");
  p->toggle(smart, "智能拼写", "smart_input", true,
            "原有总开关联动后端纠错配置；实际候选与纠错效果仍需引擎验证。");
  smart->addWidget(label("当前源码未提供独立的“纠错强度”或“连拼优化”设置，不虚构这类开关。", "muted"));
  auto *fuzzy = p->group("模糊拼音");
  fuzzy->addWidget(label("四项常用预设已勾选；这不是读取到的现用配置。", "muted"));
  const QList<QPair<QString, QString>> pairs = {
      {"z / zh", "fuzzy_z_zh"}, {"c / ch", "fuzzy_c_ch"},
      {"s / sh", "fuzzy_s_sh"}, {"in / ing", "fuzzy_in_ing"},
      {"n / l", "fuzzy_nl"}, {"r / l", "fuzzy_rl"}, {"h / f", "fuzzy_hf"},
      {"g / k", "fuzzy_gk"}, {"an / ang", "fuzzy_an_ang"},
      {"ian / iang", "fuzzy_ian_iang"}, {"uan / uang", "fuzzy_uan_uang"},
      {"hui / fei", "fuzzy_hui_fei"}, {"en / eng", "fuzzy_en_eng"},
      {"on / ong", "fuzzy_on_ong"}, {"huang / wang", "fuzzy_huang_wang"},
      {"un / ong", "fuzzy_un_ong"}, {"un / iong", "fuzzy_un_iong"},
      {"an / ai", "fuzzy_an_ai"}, {"eng / ong", "fuzzy_eng_ong"}};
  for (int i = 0; i < pairs.size(); ++i)
    p->toggle(fuzzy, pairs[i].first, pairs[i].second, i < 4);
  auto *symbols = p->group("语言与标点");
  p->choice(symbols, "默认输入语言", "default_language", {"中文", "英文"});
  p->toggle(symbols, "中文下将 /? 替换为 、", "slash_punctuation", true);
  p->toggle(symbols, "符号自动转换", "symbol_auto_change", true, "例如数字之间的部分符号使用英文标点。");
  p->toggle(symbols, "符号自动补全", "symbol_auto_pair", true, "补全成对符号的右半部分。");
  auto *emoji = p->group("表情和颜文字推荐");
  p->toggle(emoji, "微信表情（微信客户端）", "wechat_emoji", true);
  p->toggle(emoji, "Emoji", "normal_emoji", true);
  p->toggle(emoji, "颜文字", "kaomoji", true);
  p->toggle(emoji, "大表情", "large_emoji", true);
  p->toggle(emoji, "符号表情", "symbol_emoji", true);
  auto *network = p->group("联网行为");
  p->toggle(network, "单机模式", "standalone", false,
            "原设置用于停用联网功能；此演示本身始终离线，不改变真实同步服务。");
}

void voice(Page *p) {
  auto *capture = p->group("语音输入");
  p->choice(capture, "麦克风", "voice_microphone", {"自动检测（演示）"}, 0,
            "不枚举设备，不录音；正式接入后才显示真实麦克风。");
  p->choice(capture, "标点设置", "voice_punctuation",
            {"智能标点", "添加完整标点", "句末不加句号", "空格替换标点"});
  p->toggle(capture, "语音智能整理", "voice_smart_polish", true);
  auto *keys = p->group("操作方式");
  p->toggle(keys, "启动语音输入", "voice_launch_shortcut", true,
            "原页面显示 Ctrl + Super + Shift 组合；实际按键处理需独立核对。");
  p->toggle(keys, "按住说话", "voice_hold_shortcut", true,
            "原页面显示 Ctrl + Super 组合；本页不注册或触发快捷键。");
  p->body->addWidget(label("语音识别、方言与实时转写在本 Demo 中均未接入，不能以开关可调判定功能可用。", "muted"));
}

void phrases(Page *p) {
  auto *group = p->group("常用语");
  group->addWidget(label("使用合成示例编辑常用语和触发码；不会读取或修改你的词库。", "muted"));
  auto *list = new QListWidget;
  list->setObjectName("hotwords");
  list->setMinimumHeight(160);
  auto setRows = [list](const QVariant &data) {
    list->clear();
    for (const auto &v : data.toList()) {
      const auto row = v.toMap();
      auto *item = new QListWidgetItem(row.value("words").toString() + "   ·   " + row.value("key").toString(), list);
      item->setData(Qt::UserRole, row);
    }
    if (list->count()) list->setCurrentRow(0);
  };
  setRows(QVariantList{QVariantMap{{"words", "这是一条演示常用语"}, {"key", "ys"}}});
  p->bind("hotwords", [list]() -> QVariant {
    QVariantList rows;
    for (int i = 0; i < list->count(); ++i) rows << list->item(i)->data(Qt::UserRole);
    return rows;
  }, setRows);
  group->addWidget(list);
  auto *actions = new QHBoxLayout;
  auto *add = new QPushButton("添加");
  auto *edit = new QPushButton("编辑");
  auto *remove = new QPushButton("删除");
  add->setObjectName("addPhrase"); edit->setObjectName("editPhrase"); remove->setObjectName("deletePhrase");
  actions->addWidget(add); actions->addWidget(edit); actions->addWidget(remove); actions->addStretch();
  group->addLayout(actions);
  auto editEntry = [p, list](bool create) {
    auto *item = create ? nullptr : list->currentItem();
    if (!create && !item) return;
    const auto old = item ? item->data(Qt::UserRole).toMap() : QVariantMap{};
    QDialog dialog(p);
    dialog.setWindowTitle(create ? "添加演示常用语" : "编辑演示常用语");
    auto *layout = new QFormLayout(&dialog);
    auto *words = new QLineEdit(old.value("words").toString());
    auto *key = new QLineEdit(old.value("key").toString());
    words->setObjectName("phraseWords"); key->setObjectName("phraseKey");
    layout->addRow("常用语", words); layout->addRow("触发码", key);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    layout->addRow(buttons);
    auto validate = [=] { buttons->button(QDialogButtonBox::Save)->setEnabled(!words->text().trimmed().isEmpty()); };
    QObject::connect(words, &QLineEdit::textChanged, &dialog, validate); validate();
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    if (!item) item = new QListWidgetItem(list);
    item->setText(words->text() + "   ·   " + key->text());
    item->setData(Qt::UserRole, QVariantMap{{"words", words->text()}, {"key", key->text()}});
    list->setCurrentItem(item);
    p->changed();
  };
  QObject::connect(add, &QPushButton::clicked, p, [editEntry] { editEntry(true); });
  QObject::connect(edit, &QPushButton::clicked, p, [editEntry] { editEntry(false); });
  QObject::connect(list, &QListWidget::itemDoubleClicked, p, [editEntry] { editEntry(false); });
  QObject::connect(remove, &QPushButton::clicked, p, [p, list] {
    if (list->currentRow() >= 0) delete list->takeItem(list->currentRow());
    p->changed();
  });
  auto selection = [=] { edit->setEnabled(list->currentItem()); remove->setEnabled(list->currentItem()); };
  QObject::connect(list, &QListWidget::currentRowChanged, p, selection); selection();
  auto *clipboard = p->group("本地剪贴板入口");
  p->toggle(clipboard, "剪贴板", "clipboard_enabled", false,
            "原设置控制输入法内的剪贴板入口，与跨设备同步开关分开。");
}

void appearance(Page *p) {
  auto *previewGroup = p->group("候选框预览");
  auto *preview = label("1输入　2输入法　3拼音　4中文　5候选");
  preview->setObjectName("candidatePreview");
  preview->setMinimumHeight(85);
  preview->setAlignment(Qt::AlignCenter);
  previewGroup->addWidget(preview);
  auto *options = p->group("显示设置");
  auto *size = new QSlider(Qt::Horizontal);
  size->setObjectName("candidate_size");
  size->setRange(10, 18); size->setValue(13);
  p->row(options, "候选字大小（10–18）", size);
  p->bind("candidate_size", [size] { return size->value(); }, [size](const QVariant &v) { size->setValue(v.toInt()); });
  QObject::connect(size, &QSlider::valueChanged, p, [p] { p->changed(); });
  auto *theme = p->choice(options, "候选主题", "theme_mode", {"跟随系统", "浅色", "深色"});
  p->choice(options, "拼音显示位置", "preedit_position", {"输入框内", "候选框内"}, 0,
            "原实现会在输入框不支持内嵌拼音时回退到候选框。");
  auto *style = p->choice(options, "候选序号样式", "candidate_label_style", {"微信风格", "Fcitx 默认"});
  auto *linuxOptions = p->group("Linux 候选框设置");
  auto *count = new QSpinBox;
  count->setObjectName("page_size");
  count->setRange(3, 9); count->setValue(5);
  p->row(linuxOptions, "每页候选数量", count, "源自 Fcitx 插件配置，范围为 3–9。");
  p->bind("page_size", [count] { return count->value(); }, [count](const QVariant &v) { count->setValue(v.toInt()); });
  QObject::connect(count, &QSpinBox::valueChanged, p, [p] { p->changed(); });
  auto *vertical = p->toggle(linuxOptions, "纵向排列候选", "vertical", false,
            "源自 Fcitx 插件配置；本 Demo 不修改真实候选框。");
  auto update = [=] {
    const QStringList words = {"输入", "输入法", "拼音", "中文", "候选", "文字", "设置", "词语", "演示"};
    QStringList candidates;
    for (int i = 0; i < count->value(); ++i)
      candidates << QString::number(i + 1) + (style->currentIndex() == 1 ? ". " : "") + words[i];
    preview->setText(candidates.join(vertical->isChecked() ? "\n" : "　"));
    const bool dark = theme->currentIndex() != 1;
    preview->setStyleSheet(QString("background:%1;color:%2;font-size:%3px;border-radius:8px;padding:12px;")
      .arg(dark ? "#282827" : "#e1f3e8", dark ? "#effaf3" : "#244737").arg(size->value()));
  };
  QObject::connect(size, &QSlider::valueChanged, p, update);
  QObject::connect(theme, &QComboBox::currentIndexChanged, p, update);
  QObject::connect(count, &QSpinBox::valueChanged, p, update);
  QObject::connect(vertical, &QCheckBox::toggled, p, update);
  QObject::connect(style, &QComboBox::currentIndexChanged, p, update); update();
  p->body->addWidget(label("这里只调整候选框示意，不修改 Fcitx 主题。跟随系统在 Demo 中使用当前深色示意，不探测系统主题。", "muted"));
}

void shortcuts(Page *p) {
  auto *switches = p->group("中英文切换");
  p->toggle(switches, "使用 Shift", "shift_switch", true);
  p->toggle(switches, "使用 Ctrl", "ctrl_switch", false);
  switches->addWidget(label("Fcitx 全局切换键由桌面配置决定；本页不假定 Ctrl + Space 已绑定。", "muted"));
  auto *quick = p->group("快捷使用");
  p->toggle(quick, "AI 助手（=）", "ai_assistant", true, "原有入口；演示不调用联网服务。");
  p->toggle(quick, "V 模式（V）", "v_mode", true, "原有快捷功能栏入口；演示不启动功能栏。");
  auto *voice = p->group("语音输入");
  voice->addWidget(label("启动语音输入、按住说话的开关集中在“语音输入”页，避免重复配置。本页不注册按键。", "muted"));
  auto *states = p->group("输入状态切换");
  p->toggle(states, "全半角输入切换", "half_full_switch", false, "Shift + Space（依据插件按键处理）");
  p->toggle(states, "中文下中英标点切换", "punctuation_switch", true, "原页面显示 Ctrl + 。");
  p->toggle(states, "简繁体输入切换", "traditional_switch", false, "Ctrl + Shift + F");
  auto *pages = p->group("候选翻页");
  p->toggle(pages, "减号 / 等号", "page_minus_equal", true);
  p->toggle(pages, "左右中括号", "page_brackets", true);
  p->toggle(pages, "逗号 / 句号", "page_comma_period", false);
  p->toggle(pages, "Shift + Tab / Tab", "page_shift_tab", false);
  auto *selection = p->group("候选词选择");
  p->toggle(selection, "分号 / 引号选择第 2 / 3 位候选词", "select_semicolon_quote", false);
  p->toggle(selection, "左 / 右 Ctrl 选择第 2 / 3 位候选词", "select_ctrl", false);
}
}

QWidget *makeSettingsPage(const QString &category, QWidget *parent) {
  const QMap<QString, QString> titles{{"input", "输入设置"}, {"voice", "语音输入"},
    {"phrases", "常用语和剪贴板"}, {"appearance", "外观"}, {"shortcuts", "快捷键"}};
  auto *page = new Page(titles.value(category, "设置"), parent);
  page->setObjectName("settings_" + category);
  if (category == "input") input(page);
  else if (category == "voice") voice(page);
  else if (category == "phrases") phrases(page);
  else if (category == "appearance") appearance(page);
  else if (category == "shortcuts") shortcuts(page);
  page->finish();
  page->status->setObjectName(category + "_status");
  page->findChild<QPushButton *>("applySettings")->setObjectName(category + "_apply");
  page->findChild<QPushButton *>("resetSettings")->setObjectName(category + "_reset");
  return page;
}
