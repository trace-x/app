#include "tracebutton.h"
#include "ui_tracebutton.h"

TraceButton::TraceButton(const QString text, bool checkable, QWidget *parent) :
    QFrame(parent),
    ui(new Ui::TraceButton)
{
    ui->setupUi(this);

    ui->pushButton->setCheckable(checkable);
    ui->pushButton->setText(text);

    connect(ui->pushButton, &QPushButton::clicked, this, &TraceButton::switch_flag);
    connect(ui->pushButton, &QPushButton::clicked, this, &TraceButton::triggered);

    clear();
}

TraceButton::~TraceButton()
{
    delete ui;
}

void TraceButton::set_elapsed(quint64 elapsed)
{
    ui->label->setText(QString::number(elapsed));
}

void TraceButton::clear()
{
    ui->label->setText("-");
    flag = 1;
}

void TraceButton::switch_flag(bool check)
{
    if(!check)
    {
        flag = 0;
    }
}
