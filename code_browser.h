#ifndef CODE_BROWSER_H
#define CODE_BROWSER_H

#include <QPlainTextEdit>
#include <QObject>

QT_BEGIN_NAMESPACE
class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;
QT_END_NAMESPACE

class LineNumberArea;

class CodeBrowser : public QPlainTextEdit
{
    Q_OBJECT

public:
    CodeBrowser(QWidget *parent = 0);

    void line_number_area_paint(QPaintEvent *event);
    int line_number_area_width();

protected:
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

private slots:
    void update_line_number_area_width(int newBlockCount);
    void highlight_current_line();
    void update_line_number_area(const QRect &, int);

private:
    QFrame *_line_number_area;
};

class LineNumberArea : public QFrame
{
public:
    LineNumberArea(CodeBrowser *editor);

    QSize sizeHint() const Q_DECL_OVERRIDE;

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

private:
    CodeBrowser *_code_editor;
};

#endif
