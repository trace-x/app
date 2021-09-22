#ifndef TRACEBUTTON_H
#define TRACEBUTTON_H

#include <QFrame>

namespace Ui {
class TraceButton;
}

class TraceButton : public QFrame
{
    Q_OBJECT

public:
    explicit TraceButton(const QString text, bool checkable = false, QWidget *parent = 0);
    ~TraceButton();

    /*volatile*/ int flag;

signals:
    void triggered(bool checked);

public slots:
    void set_elapsed(quint64 elapsed);
    void clear();

private slots:
    void switch_flag(bool check);

private:
    Ui::TraceButton *ui;
};

#endif // TRACEBUTTON_H
