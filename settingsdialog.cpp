#include "settingsdialog.h"
#include "sketchyui.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QPainter>
#include <QPainterPath>
#include <QEvent>
#include <QMouseEvent>
#include <QToolTip>
#include <QTimer>

static QPixmap circleCrop(const QPixmap &src, int size)
{
    if (src.isNull()) return {};
    QPixmap result(size, size);
    result.fill(Qt::transparent);
    QPainter p(&result);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addEllipse(0, 0, size, size);
    p.setClipPath(path);
    p.drawPixmap(0, 0, size, size, src.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    p.end();
    return result;
}

static QPixmap defaultAvatar(int size)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor("#C4BDB3"));
    p.setPen(Qt::NoPen);
    p.drawEllipse(0, 0, size, size);
    p.end();
    return pm;
}

SettingsDialog::SettingsDialog(const UserSettings &current, QWidget *parent)
    : QDialog(parent), m_result(current)
{
    setWindowTitle("设置");
    setFixedSize(360, 500);
    setStyleSheet(QString("QDialog { background: %1; }").arg(QColor("#FDFBF7").name()));

    QVBoxLayout *mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(32, 28, 32, 24);
    mainLay->setSpacing(18);

    // ── Avatar section ──
    m_avatarPreview = new QLabel;
    m_avatarPreview->setFixedSize(88, 88);
    m_avatarPreview->setCursor(Qt::PointingHandCursor);
    m_avatarPreview->installEventFilter(this);
    updateAvatarPreview();

    QHBoxLayout *avatarRow = new QHBoxLayout;
    avatarRow->addStretch();
    avatarRow->addWidget(m_avatarPreview);
    avatarRow->addStretch();
    mainLay->addLayout(avatarRow);

    QLabel *avatarHint = new QLabel("点击更换头像");
    avatarHint->setAlignment(Qt::AlignCenter);
    avatarHint->setStyleSheet("color: #9A9590; font-size: 11px;");
    mainLay->addWidget(avatarHint);

    mainLay->addSpacing(4);

    // ── Name section ──
    QLabel *nameLabel = new QLabel("昵称");
    nameLabel->setStyleSheet("color: #4A4540; font-size: 12px; font-weight: bold;");
    mainLay->addWidget(nameLabel);

    m_nameEdit = new QLineEdit(current.name);
    m_nameEdit->setPlaceholderText("请输入昵称");
    m_nameEdit->setStyleSheet(
        "QLineEdit {"
        "  border: 1px solid #C4BDB3;"
        "  border-radius: 6px;"
        "  padding: 8px 12px;"
        "  font-size: 14px;"
        "  background: #FAF7F0;"
        "  color: #2B2B2B;"
        "}"
        "QLineEdit:focus { border-color: #8A8078; }"
    );
    mainLay->addWidget(m_nameEdit);

    // ── Gender ──
    QHBoxLayout *genderRow = new QHBoxLayout;
    QLabel *genderLbl = new QLabel("性别");
    genderLbl->setStyleSheet("color: #4A4540; font-size: 12px; font-weight: bold;");
    genderLbl->setFixedWidth(40);
    genderRow->addWidget(genderLbl);
    m_genderCombo = new QComboBox;
    m_genderCombo->addItems({"", "男", "女"});
    m_genderCombo->setCurrentIndex(current.gender == "女" ? 2 : (current.gender == "男" ? 1 : 0));
    m_genderCombo->setFixedWidth(80);
    m_genderCombo->setStyleSheet(
        "QComboBox {"
        "  border: 1px solid #C4BDB3;"
        "  border-radius: 6px;"
        "  padding: 6px 10px;"
        "  font-size: 13px;"
        "  background: #FAF7F0;"
        "  color: #2B2B2B;"
        "}"
        "QComboBox:focus { border-color: #8A8078; }"
        "QComboBox::drop-down { border: none; width: 20px; }"
    );
    genderRow->addWidget(m_genderCombo);
    m_genderHint = new QLabel(" ⓘ");
    m_genderHint->setStyleSheet("color: #B5AFA8; font-size: 13px;");
    m_genderHint->setCursor(Qt::PointingHandCursor);
    m_genderNoteText =
        "我们完全尊重并支持多样化性别认同，\n"
        "但由于目前科学界尚无针对非二元性别的基础代谢率（BMR）计算公式，\n"
        "为了确保您获取的数据具有相对准确的健康参考价值，\n"
        "本程序目前仅提供\"男/女\"两种选项。";
    m_genderHint->installEventFilter(this);
    genderRow->addWidget(m_genderHint);
    genderRow->addStretch();
    mainLay->addLayout(genderRow);

    mainLay->addSpacing(6);

    // ── Body metrics ──
    auto makeMetricRow = [&](const QString &label, const QString &unit, int value, QLineEdit *&edit) {
        QHBoxLayout *row = new QHBoxLayout;
        QLabel *lbl = new QLabel(label);
        lbl->setStyleSheet("color: #4A4540; font-size: 12px; font-weight: bold;");
        lbl->setFixedWidth(40);
        row->addWidget(lbl);
        edit = new QLineEdit(value > 0 ? QString::number(value) : QString());
        edit->setPlaceholderText("—");
        edit->setFixedWidth(80);
        edit->setStyleSheet(
            "QLineEdit {"
            "  border: 1px solid #C4BDB3;"
            "  border-radius: 6px;"
            "  padding: 6px 10px;"
            "  font-size: 13px;"
            "  background: #FAF7F0;"
            "  color: #2B2B2B;"
            "}"
            "QLineEdit:focus { border-color: #8A8078; }"
        );
        row->addWidget(edit);
        QLabel *unitLbl = new QLabel(unit);
        unitLbl->setStyleSheet("color: #9A9590; font-size: 12px;");
        row->addWidget(unitLbl);
        row->addStretch();
        return row;
    };

    mainLay->addLayout(makeMetricRow("身高", "cm", current.height, m_heightEdit));
    mainLay->addLayout(makeMetricRow("体重", "kg", current.weight, m_weightEdit));
    mainLay->addLayout(makeMetricRow("年龄", "岁",  current.age,    m_ageEdit));

    mainLay->addSpacing(8);

    // ── Privacy disclaimer ──
    QLabel *privacy = new QLabel(
        "本程序收集的所有个人信息仅用于计算基础代谢，"
        "我们承诺所有数据均保存在您的设备，"
        "开发组不做任何留存，亦不会用于任何商业用途或透露给第三方。");
    privacy->setWordWrap(true);
    privacy->setStyleSheet("color: #B5AFA8; font-size: 9px; line-height: 1.4;");
    mainLay->addWidget(privacy);

    // ── Buttons ──
    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    SketchyButton *cancelBtn = new SketchyButton("取消", QColor("#E0D7CC"), QColor("#3A3530"));
    cancelBtn->setFixedSize(80, 36);
    SketchyButton *saveBtn = new SketchyButton("保存", QColor("#D0E3EF"), QColor("#3A3530"));
    saveBtn->setFixedSize(80, 36);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, this, [this]() {
        m_result.name = m_nameEdit->text().isEmpty() ? "用户" : m_nameEdit->text();
        m_result.gender = m_genderCombo->currentText();
        m_result.height = m_heightEdit->text().toInt();
        m_result.weight = m_weightEdit->text().toInt();
        m_result.age    = m_ageEdit->text().toInt();
        accept();
    });
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(saveBtn);
    mainLay->addLayout(btnRow);
}

void SettingsDialog::showGenderNote()
{
    QLabel *popup = new QLabel(m_genderNoteText, this, Qt::ToolTip);
    popup->setStyleSheet(
        "background: #FDFBF7; border: 1px solid #D0C8BC; border-radius: 6px;"
        "padding: 10px 14px; color: #4A4540; font-size: 12px;");
    popup->setWordWrap(true);
    popup->setFixedWidth(300);
    popup->adjustSize();
    m_genderPopup = popup;

    QPoint pos = m_genderHint->mapToGlobal(QPoint(0, m_genderHint->height() + 4));
    m_genderPopup->move(pos);
    m_genderPopup->show();

    QTimer::singleShot(6000, this, [this]() { dismissGenderNote(); });
}

void SettingsDialog::dismissGenderNote()
{
    if (m_genderPopup) {
        m_genderPopup->deleteLater();
        m_genderPopup = nullptr;
    }
}

void SettingsDialog::updateAvatarPreview()
{
    QPixmap src(m_result.avatarPath);
    QPixmap avatar = src.isNull() ? defaultAvatar(88) : circleCrop(src, 88);
    m_avatarPreview->setPixmap(avatar);
}

void SettingsDialog::onAvatarClicked()
{
    QString path = QFileDialog::getOpenFileName(this, "选择头像", QString(),
        "Images (*.png *.jpg *.jpeg *.bmp)");
    if (!path.isEmpty()) {
        m_result.avatarPath = path;
        updateAvatarPreview();
    }
}

bool SettingsDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_avatarPreview && event->type() == QEvent::MouseButtonPress) {
        onAvatarClicked();
        return true;
    }
    if (obj == m_genderHint && event->type() == QEvent::MouseButtonPress) {
        if (m_genderPopup)
            dismissGenderNote();
        else
            showGenderNote();
        return true;
    }
    if (m_genderPopup && event->type() == QEvent::MouseButtonPress) {
        if (obj != m_genderPopup && obj != m_genderHint)
            dismissGenderNote();
    }
    return QDialog::eventFilter(obj, event);
}
