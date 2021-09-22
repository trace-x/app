#ifndef PANEL_CONTAINER_H
#define PANEL_CONTAINER_H

#include <QFrame>
#include <QShortcut>

namespace Ui {
class PanelContainer;
}

class PanelContainer : public QFrame
{
    Q_OBJECT

public:
    explicit PanelContainer(QWidget *content_widget, QWidget *parent = 0);
    ~PanelContainer();

    QByteArray save_state() const;
    void restore_state(const QByteArray &state_data);

signals:
    void visibility_changed(bool visible);
    void dock_changed(bool is_detached);
    void double_clicked();

protected:
    bool event(QEvent *event);
    bool eventFilter(QObject *, QEvent *);

private slots:
    void detach();

private:
    Ui::PanelContainer *ui;

    QWidget *_content_widget;
    QRect _geometry;

    QByteArray _state;
};

#endif // PANEL_CONTAINER_H
