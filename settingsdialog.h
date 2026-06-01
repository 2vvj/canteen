#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QSlider>
#include <QPushButton>
#include <QPoint>
#include "userdata.h"

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(const UserSettings &current, QWidget *parent = nullptr);
    UserSettings result() const { return m_result; }

signals:
    void bgmVolumeChanged(int vol);

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private slots:
    void onAvatarClicked();

private:
    void updateAvatarPreview();
    void showGenderNote();
    void dismissGenderNote();

    UserSettings m_result;
    QLabel *m_avatarPreview;
    QLabel *m_genderHint;
    QLineEdit *m_nameEdit;
    QString m_genderNoteText;
    QComboBox *m_genderCombo;
    QLineEdit *m_heightEdit;
    QLineEdit *m_weightEdit;
    QLineEdit *m_ageEdit;
    QPushButton *m_closeBtn;
    QSlider *m_bgmSlider;
    QWidget *m_genderPopup = nullptr;
    QPoint m_dragPos;
    bool m_dragging = false;
};

#endif
