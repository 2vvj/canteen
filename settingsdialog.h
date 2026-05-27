#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include "userdata.h"

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(const UserSettings &current, QWidget *parent = nullptr);
    UserSettings result() const { return m_result; }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

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
    QWidget *m_genderPopup = nullptr;
};

#endif
