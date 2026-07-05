#ifndef AIREPORTDIALOG_H
#define AIREPORTDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QMouseEvent>
#include "calendarwindow.h"
struct ReportEntry {
    QString date;
    QString type;
    QString content;
};

class ReportHistoryDialog : public QDialog {
    Q_OBJECT
public:
    explicit ReportHistoryDialog(QWidget *parent = nullptr);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    QListWidget *m_list;
    QTextEdit *m_viewer;
    QPushButton *m_closeBtn;
    QVector<ReportEntry> m_entries;
    void loadReports();
    void showReport(int index);
};

class AiReportDialog : public QDialog {
    Q_OBJECT
public:
    enum ReportMode { Today, Week };

    explicit AiReportDialog(const QMap<QString, DailyRecord> &records,
                            double bmr = 0,
                            QWidget *parent = nullptr);

    void setApiKey(const QString &key);

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void closeEvent(QCloseEvent *e) override;

private slots:
    void onModeSelected(ReportMode mode);
    void onGenerateReport();
    void onLoadingTick();
    void onHistoryClicked();

private:
    void paintEvent(QPaintEvent *event) override;
    QString buildTodayPrompt() const;
    QString buildWeekPrompt() const;
    QString buildTodayTemplate() const;
    QString buildWeekTemplate() const;
    void callApi(const QString &prompt);
    void setButtonsEnabled(bool enabled);
    void saveReport(const QString &content);
    void analyzeDietQuality(QString &toneHint, QString &dietQuality) const;
    void updateModeButtonStyles();

    QTextEdit *m_reportText;
    QPushButton *m_generateBtn;
    QLabel *m_statusLabel;
    QPushButton *m_todayBtn;
    QPushButton *m_weekBtn;
    QPushButton *m_historyBtn;
    QNetworkAccessManager *m_networkManager;
    QMap<QString, DailyRecord> m_records;
    double m_bmr = 0;
    QString m_apiKey;
    ReportMode m_mode = Today;

    QNetworkReply *m_currentReply = nullptr;
    QByteArray m_sseBuffer;
    QString m_fullText;

    QTimer *m_loadingTimer;
    int m_loadingDots = 0;

    QPoint m_dragPos;
    bool m_dragging = false;
    bool m_closedWhileGenerating = false;
};

#endif
