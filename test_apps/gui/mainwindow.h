#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "tracer.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    void new_instance();
    void crash();
    void restart();
    void crash_restart();
    void show_trace();
    void add_tracer(QThread *thread);

private:
    Ui::MainWindow *ui;
};

#endif // MAIN_WINDOW_H
