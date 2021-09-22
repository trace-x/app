#include "about_dialog.h"
#include "ui_about_dialog.h"

#ifdef WITH_BUILD_INFO
  #include "build_info.h"
#endif

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    QString version = "1.0";
    ui->app_label->setText(QString("Trace-X %1").arg(version));

#ifdef WITH_BUILD_INFO
    QString build_time = BUILD_TIME;
    QString revision = BUILD_REVISION;

    ui->info_label->setText(QString("\nBuilt on %1\nRevision %2").arg(build_time).arg(revision));
#endif
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
