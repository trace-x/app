#ifndef TEXT_INPUT_DIALOG_H
#define TEXT_INPUT_DIALOG_H

#include <QDialog>

namespace Ui {
class TextInputDialog;
}

class TextInputDialog : public QDialog
{
    Q_OBJECT

public:
    TextInputDialog(const QString &title_text, const QString &label_text, const QString &initial_text, QWidget *parent = 0);
    ~TextInputDialog();

    QString text() const;

private:
    Ui::TextInputDialog *ui;
};

#endif // TEXT_INPUT_DIALOG_H
