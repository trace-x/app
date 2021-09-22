#include <QtWidgets>

#include "code_browser.h"
#include "cpp_highlighter.h"
#include "settings.h"

CodeBrowser::CodeBrowser(QWidget *parent) : QPlainTextEdit(parent)
{
    _line_number_area = new LineNumberArea(this);

    setReadOnly(true);

    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(update_line_number_area_width(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(update_line_number_area(QRect,int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlight_current_line()));

    update_line_number_area_width(0);
    highlight_current_line();

    setCenterOnScroll(true);
    setLineWrapMode(QPlainTextEdit::NoWrap);

    new CppHighlighter(this->document());
}

int CodeBrowser::line_number_area_width()
{
    int digits = 1;
    int max = qMax(1, blockCount());

    while (max >= 10)
    {
        max /= 10;
        ++digits;
    }

    int space = 8 + fontMetrics().width(QLatin1Char('9')) * digits;

    return space;
}

void CodeBrowser::update_line_number_area_width(int /* newBlockCount */)
{
    setViewportMargins(line_number_area_width(), 0, 0, 0);
}

void CodeBrowser::update_line_number_area(const QRect &rect, int dy)
{
    if(dy)
    {
        _line_number_area->scroll(0, dy);
    }
    else
    {
        _line_number_area->update(0, rect.y(), _line_number_area->width(), rect.height());
    }

    if(rect.contains(viewport()->rect()))
    {
        update_line_number_area_width(0);
    }
}

void CodeBrowser::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    _line_number_area->setGeometry(QRect(cr.left(), cr.top(), line_number_area_width(), cr.height()));
}

void CodeBrowser::paintEvent(QPaintEvent *event)
{
    if(this->document()->isEmpty())
    {
        QPainter painter;
        painter.begin(viewport());
        painter.save();

        painter.drawText(viewport()->geometry(), Qt::AlignCenter, tr("Source code is not available"));

        painter.restore();
        painter.end();
    }
    else
    {
        QPlainTextEdit::paintEvent(event);
    }
}

void CodeBrowser::highlight_current_line()
{
    QList<QTextEdit::ExtraSelection> extra_selections;

    QTextEdit::ExtraSelection selection;

    QColor line_color = x_settings().line_highlight_color;

    selection.format.setBackground(line_color);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();

    extra_selections.append(selection);

    setExtraSelections(extra_selections);
}

void CodeBrowser::line_number_area_paint(QPaintEvent *event)
{
    if(document()->isEmpty())
    {
        return;
    }

    QPainter painter(_line_number_area);

    QTextBlock block = firstVisibleBlock();
    int block_number = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + int(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(block_number + 1);
            painter.setPen(Qt::white);
            painter.setFont(this->font());

            painter.drawText(0, top, _line_number_area->width(), fontMetrics().height(),
                             Qt::AlignCenter, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + int(blockBoundingRect(block).height());

        ++block_number;
    }
}

LineNumberArea::LineNumberArea(CodeBrowser *editor) : QFrame(editor)
{
    _code_editor = editor;
}

QSize LineNumberArea::sizeHint() const
{
    return QSize(_code_editor->line_number_area_width(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);

    _code_editor->line_number_area_paint(event);
}
