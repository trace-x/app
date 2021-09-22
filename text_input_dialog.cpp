#include "text_input_dialog.h"
#include "ui_text_input_dialog.h"

#include <QDialogButtonBox>
#include <QPushButton>

TextInputDialog::TextInputDialog(const QString &title_text, const QString &label_text, const QString &initial_text, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TextInputDialog)
{
    ui->setupUi(this);
    setWindowTitle(title_text);

    ui->label->setText(label_text);
    ui->line_edit->setText(initial_text);
}

TextInputDialog::~TextInputDialog()
{
    delete ui;
}

QString TextInputDialog::text() const
{
    return ui->line_edit->text();
}
