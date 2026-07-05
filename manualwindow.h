#ifndef MANUALWINDOW_H
#define MANUALWINDOW_H

#include <QDialog>
#include <QPushButton>
#include <QPoint>
#include <QMouseEvent>

class ManualWindow : public QDialog {
    Q_OBJECT
public:
    explicit ManualWindow(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    QPushButton *m_closeBtn;
    QPoint m_dragPos;
    bool m_dragging = false;
};

#endif
