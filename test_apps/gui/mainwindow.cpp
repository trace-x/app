#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QProcess>
#include <QDebug>

#include "trace_x/trace_x.h"

#include "test_apps/lib0/lib0.h"

MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowTitle(QString("%1[%2]").arg(QApplication::applicationName()).arg(QApplication::applicationPid()));

    connect(ui->action_new_instance, &QAction::triggered, this, &MainWindow::new_instance);
    connect(ui->action_crash, &QAction::triggered, this, &MainWindow::crash);
    connect(ui->action_restart, &QAction::triggered, this, &MainWindow::restart);
    connect(ui->action_crash_restart, &QAction::triggered, this, &MainWindow::crash_restart);
    connect(ui->action_show_trace, &QAction::triggered, this, &MainWindow::show_trace);

    add_tracer(new QThread());
    add_tracer(new QThread());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::new_instance()
{
    QProcess::startDetached(QApplication::applicationFilePath());
}

void MainWindow::crash()
{
    int *dead_ptr = (int *)(0xDEAD);

    delete dead_ptr;
}

void MainWindow::restart()
{
    new_instance();

    qApp->exit();
}

void MainWindow::crash_restart()
{
    new_instance();
    crash();
}

void MainWindow::show_trace()
{
    trace_x::show_trace_receiver();
}

void MainWindow::add_tracer(QThread *thread)
{
    Tracer *tracer = new Tracer(thread);

    ui->splitter->addWidget(tracer);

    tracer->init();
}
