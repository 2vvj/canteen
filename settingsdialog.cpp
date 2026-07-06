#include "settingsdialog.h"
#include "sketchyui.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QPainter>
#include <QPainterPath>
#include <QEvent>
#include <QMouseEvent>
#include <QTimer>

static const QColor C_CREAM2   = QColor("#FDFBF7");
static const QColor C_INK2     = QColor("#2B2B2B");
static const QColor C_INK_L2   = QColor("#4A4540");
static const QColor C_SHADOW2  = QColor("#3A3530");
static const QColor C_HINT     = QColor("#9A9590");
static const QColor C_HINT2    = QColor("#B5AFA8");
static const QColor C_FIELD_BG = QColor("#FAF7F0");

static const char *FIELD_STYLE =
    "QLineEdit, QComboBox {"
    "  border: none;"
    "  border-bottom: 1px solid #C4BDB3;"
    "  padding: 6px 4px;"
    "  background: transparent;"
    "  color: #2B2B2B;"
    "  selection-background-color: #D0DDE8;"
    "}"
    "QLineEdit:focus, QComboBox:focus {"
    "  border-bottom: 1.5px solid #8A8078;"
    "}";

static const char *COMBO_STYLE = FIELD_STYLE;

static QPixmap circleCrop(const QPixmap &src, int size)
{
    if (src.isNull()) return {};
    QPixmap result(size, size);
    result.fill(Qt::transparent);
    QPainter p(&result);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path; path.addEllipse(0, 0, size, size);
    p.setClipPath(path);
    p.drawPixmap(0, 0, size, size, src.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    return result;
}

static QPixmap defaultAvatar(int size)
{
    QPixmap pm(size, size); pm.fill(Qt::transparent);
    QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor("#C4BDB3")); p.setPen(Qt::NoPen);
    p.drawEllipse(0, 0, size, size);
    return pm;
}

SettingsDialog::SettingsDialog(const UserSettings &current, QWidget *parent)
    : QDialog(parent), m_result(current)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(400, 560);

    QVBoxLayout *mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(36, 36, 36, 28);
    mainLay->setSpacing(16);

    QFont secFont; secFont.setPointSize(12);
    secFont.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);
    secFont.setWeight(QFont::Bold);
    QFont bodyFont; bodyFont.setPointSize(12);
    bodyFont.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);
    QFont hintFont; hintFont.setPointSize(10);
    hintFont.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);

    QLabel *titleLbl = new QLabel(QString::fromUtf8("设  置"));
    QFont titleFont; titleFont.setPointSize(16);
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 3.0);
    titleFont.setWeight(QFont::Bold);
    titleLbl->setFont(titleFont);
    titleLbl->setStyleSheet(QString("color: %1;").arg(C_INK2.name()));
    mainLay->addWidget(titleLbl);
    mainLay->addSpacing(4);

    m_closeBtn = new SketchyButton(QString::fromUtf8("×"),
        QColor("#E0D7CC"), C_SHADOW2, this);
    m_closeBtn->setFixedSize(36, 36);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setStyleSheet(
        "font-size: 18px; font-weight: bold; color: #2B2B2B;"
        "font-family: 'Microsoft YaHei';");
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::close);
    m_closeBtn->move(width() - 36 - 36, 30);

    m_avatarPreview = new QLabel;
    m_avatarPreview->setFixedSize(80, 80);
    m_avatarPreview->setCursor(Qt::PointingHandCursor);
    m_avatarPreview->installEventFilter(this);
    updateAvatarPreview();

    QHBoxLayout *avatarRow = new QHBoxLayout;
    avatarRow->addStretch(); avatarRow->addWidget(m_avatarPreview); avatarRow->addStretch();
    mainLay->addLayout(avatarRow);

    QLabel *avatarHint = new QLabel(QString::fromUtf8("点击更换头像"));
    avatarHint->setAlignment(Qt::AlignCenter);
    avatarHint->setFont(hintFont);
    avatarHint->setStyleSheet(QString("color: %1;").arg(C_HINT.name()));
    mainLay->addWidget(avatarHint);
    mainLay->addSpacing(6);

    QLabel *nameLbl = new QLabel(QString::fromUtf8("昵称"));
    nameLbl->setFont(secFont);
    nameLbl->setStyleSheet(QString("color: %1;").arg(C_INK_L2.name()));
    mainLay->addWidget(nameLbl);

    m_nameEdit = new QLineEdit(current.name);
    m_nameEdit->setPlaceholderText(QString::fromUtf8("请输入昵称"));
    m_nameEdit->setFont(bodyFont);
    m_nameEdit->setStyleSheet(FIELD_STYLE);
    mainLay->addWidget(m_nameEdit);

    QHBoxLayout *genderRow = new QHBoxLayout;
    QLabel *genderLbl = new QLabel(QString::fromUtf8("性别"));
    genderLbl->setFont(secFont);
    genderLbl->setStyleSheet(QString("color: %1;").arg(C_INK_L2.name()));
    genderLbl->setFixedWidth(44);
    genderRow->addWidget(genderLbl);
    m_genderCombo = new QComboBox;
    m_genderCombo->addItems({"", QString::fromUtf8("男"), QString::fromUtf8("女")});
    m_genderCombo->setCurrentIndex(current.gender == QString::fromUtf8("女") ? 2 : (current.gender == QString::fromUtf8("男") ? 1 : 0));
    m_genderCombo->setFixedWidth(80);
    m_genderCombo->setFont(bodyFont);
    m_genderCombo->setStyleSheet(COMBO_STYLE);
    genderRow->addWidget(m_genderCombo);
    m_genderHint = new QLabel(QString::fromUtf8(" ⓘ"));
    m_genderHint->setFont(bodyFont);
    m_genderHint->setStyleSheet(QString("color: %1;").arg(C_HINT2.name()));
    m_genderHint->setCursor(Qt::PointingHandCursor);
    m_genderNoteText =
        QString::fromUtf8("我们完全尊重并支持多样化性别认同，\n"
        "但由于目前科学界尚无针对非二元性别的基础代谢率（BMR）计算公式，\n"
        "为了确保您获取的数据具有相对准确的健康参考价值，\n"
        "本程序目前仅提供\"男/女\"两种选项。");
    m_genderHint->installEventFilter(this);
    genderRow->addWidget(m_genderHint);
    genderRow->addStretch();
    mainLay->addLayout(genderRow);
    mainLay->addSpacing(4);

    auto makeRow = [&](const QString &label, const QString &unit, int value, QLineEdit *&edit) {
        QHBoxLayout *row = new QHBoxLayout;
        QLabel *lbl = new QLabel(label);
        lbl->setFont(secFont);
        lbl->setStyleSheet(QString("color: %1;").arg(C_INK_L2.name()));
        lbl->setFixedWidth(44);
        row->addWidget(lbl);
        edit = new QLineEdit(value > 0 ? QString::number(value) : QString());
        edit->setPlaceholderText(QString::fromUtf8("—"));
        edit->setFixedWidth(80);
        edit->setFont(bodyFont);
        edit->setStyleSheet(FIELD_STYLE);
        row->addWidget(edit);
        QLabel *unitLbl = new QLabel(unit);
        unitLbl->setFont(bodyFont);
        unitLbl->setStyleSheet(QString("color: %1;").arg(C_HINT.name()));
        row->addWidget(unitLbl);
        row->addStretch();
        return row;
    };

    mainLay->addLayout(makeRow(QString::fromUtf8("身高"), "cm", current.height, m_heightEdit));
    mainLay->addLayout(makeRow(QString::fromUtf8("体重"), "kg", current.weight, m_weightEdit));
    mainLay->addLayout(makeRow(QString::fromUtf8("年龄"), QString::fromUtf8("岁"), current.age, m_ageEdit));
    mainLay->addSpacing(4);

    QHBoxLayout *bgmRow = new QHBoxLayout;
    QLabel *bgmIcon = new QLabel(QString::fromUtf8("🎵"), this);
    bgmIcon->setStyleSheet("font-size:14px;background:transparent;border:none;");
    bgmRow->addWidget(bgmIcon);
    m_bgmSlider = new QSlider(Qt::Horizontal, this);
    m_bgmSlider->setRange(0, 100);
    m_bgmSlider->setValue(m_result.bgmVolume);
    m_bgmSlider->setFixedSize(120, 24);
    m_bgmSlider->setStyleSheet(
        "QSlider::groove:horizontal {"
        "  height: 6px; background: #D8D0C8; border-radius: 3px;"
        "}"
        "QSlider::handle:horizontal {"
        "  width: 16px; height: 16px; margin: -6px 0;"
        "  background: #E8DDD0; border: 1.5px solid #8A7A6A; border-radius: 9px;"
        "}"
        "QSlider::handle:horizontal:hover { background: #DDD0C0; }"
        "QSlider::sub-page:horizontal {"
        "  background: #8A9A7A; border-radius: 3px;"
        "}");
    connect(m_bgmSlider, &QSlider::valueChanged, this, [this](int val) {
        m_result.bgmVolume = val;
        emit bgmVolumeChanged(val);
    });
    bgmRow->addWidget(m_bgmSlider);
    bgmRow->addStretch();
    mainLay->addLayout(bgmRow);

    QLabel *privacy = new QLabel(QString::fromUtf8(
        "本程序收集的所有个人信息仅用于计算基础代谢，"
        "我们承诺所有数据均保存在您的设备，"
        "开发组不做任何留存，亦不会用于任何商业用途或透露给第三方。"));
    privacy->setWordWrap(true);
    QFont privFont; privFont.setPointSize(9);
    privFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
    privacy->setFont(privFont);
    privacy->setStyleSheet(QString("color: %1;").arg(C_HINT2.name()));
    mainLay->addWidget(privacy);

    QLabel *watermark = new QLabel(QString::fromUtf8("开发组：蛋卷脆"));
    watermark->setAlignment(Qt::AlignLeft);
    QFont wmFont; wmFont.setPointSize(9);
    wmFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
    watermark->setFont(wmFont);
    watermark->setStyleSheet(QString("color: %1;").arg(C_HINT.name()));
    mainLay->addWidget(watermark);

    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    SketchyButton *saveBtn = new SketchyButton(QString::fromUtf8("保存"),
        QColor("#E0D7CC"), C_SHADOW2, this);
    saveBtn->setFixedSize(90, 40);
    connect(saveBtn, &QPushButton::clicked, this, [this]() {
        m_result.name = m_nameEdit->text().isEmpty() ? QString::fromUtf8("用户") : m_nameEdit->text();
        m_result.gender = m_genderCombo->currentText();
        m_result.height = m_heightEdit->text().toInt();
        m_result.weight = m_weightEdit->text().toInt();
        m_result.age    = m_ageEdit->text().toInt();
        accept();
    });
    btnRow->addWidget(saveBtn);
    mainLay->addLayout(btnRow);
}

void SettingsDialog::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF card(12, 12, width() - 24, height() - 24);
    int seed = 57;

    QRectF shadow = card.translated(2.5, 3.5);
    QPainterPath sp = sketchyRect(shadow, seed + 100, 2.8);
    p.setBrush(C_SHADOW2);
    p.setPen(Qt::NoPen);
    p.drawPath(sp);

    QPainterPath cp = sketchyRect(card, seed, 2.8);
    drawInkWash(&p, cp, C_CREAM2, 18);
    drawInkBorder(&p, cp, C_INK2, 3, 0.7);
}

void SettingsDialog::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        QWidget *child = childAt(e->pos());
        if (!child || child == this) {
            m_dragPos = e->globalPosition().toPoint() - frameGeometry().topLeft();
            m_dragging = true;
        }
    }
    QDialog::mousePressEvent(e);
}

void SettingsDialog::mouseMoveEvent(QMouseEvent *e)
{
    if (m_dragging && (e->buttons() & Qt::LeftButton)) {
        QPoint delta = e->globalPosition().toPoint() - frameGeometry().topLeft() - m_dragPos;
        if (delta.manhattanLength() > 4)
            move(e->globalPosition().toPoint() - m_dragPos);
    }
    QDialog::mouseMoveEvent(e);
}

void SettingsDialog::mouseReleaseEvent(QMouseEvent *e)
{
    m_dragging = false;
    QDialog::mouseReleaseEvent(e);
}

void SettingsDialog::showGenderNote()
{
    QLabel *popup = new QLabel(m_genderNoteText, this, Qt::ToolTip);
    popup->setWordWrap(true);
    popup->setFixedWidth(280);

    QFont popFont; popFont.setPointSize(10);
    popFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
    popup->setFont(popFont);
    popup->setStyleSheet(QString(
        "background: %1; border: 1.5px solid #D0C8BC;"
        "padding: 12px 16px; color: %2;").arg(C_CREAM2.name()).arg(C_INK_L2.name()));
    popup->adjustSize();
    m_genderPopup = popup;

    QPoint pos = m_genderHint->mapToGlobal(QPoint(0, m_genderHint->height() + 4));
    m_genderPopup->move(pos);
    m_genderPopup->show();

    QTimer::singleShot(6000, this, [this]() { dismissGenderNote(); });
}

void SettingsDialog::dismissGenderNote()
{
    if (m_genderPopup) { m_genderPopup->deleteLater(); m_genderPopup = nullptr; }
}

void SettingsDialog::updateAvatarPreview()
{
    QPixmap src(m_result.avatarPath);
    QPixmap avatar = src.isNull() ? defaultAvatar(80) : circleCrop(src, 80);
    m_avatarPreview->setPixmap(avatar);
}

void SettingsDialog::onAvatarClicked()
{
    QString path = QFileDialog::getOpenFileName(this, QString::fromUtf8("选择头像"), QString(),
        QString::fromUtf8("Images (*.png *.jpg *.jpeg *.bmp)"));
    if (!path.isEmpty()) { m_result.avatarPath = path; updateAvatarPreview(); }
}

bool SettingsDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_avatarPreview && event->type() == QEvent::MouseButtonPress) {
        onAvatarClicked(); return true;
    }
    if (obj == m_genderHint && event->type() == QEvent::MouseButtonPress) {
        m_genderPopup ? dismissGenderNote() : showGenderNote(); return true;
    }
    if (m_genderPopup && event->type() == QEvent::MouseButtonPress) {
        if (obj != m_genderPopup && obj != m_genderHint) dismissGenderNote();
    }
    return QDialog::eventFilter(obj, event);
}
