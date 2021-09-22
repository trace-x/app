#include "base_item_views.h"

#include <QPainter>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMimeData>
#include <QApplication>
#include <QClipboard>

#include "common_ui_tools.h"
#include "trace_x/trace_x.h"

ItemView::ItemView(QWidget *parent):
    QAbstractItemView(parent)
{
}

void ItemView::set_logo(const QPixmap &pixmap)
{
    _logo_pixmap = pixmap;

    repaint();
}

void ItemView::paintEvent(QPaintEvent *event)
{
    QAbstractItemView::paintEvent(event);

    if(!_logo_pixmap.isNull() && model() && !model()->hasChildren())
    {
        QPainter painter;
        painter.begin(viewport());
        painter.save();

        painter.drawPixmap(viewport()->geometry().center() - _logo_pixmap.rect().center(), _logo_pixmap);

        painter.restore();
        painter.end();
    }
}

TableView::TableView(QWidget *parent):
    QTableView(parent)
{
    connect(this, &QAbstractItemView::doubleClicked, this, &TableView::activated_ex);
}

void TableView::set_logo(const QPixmap &pixmap)
{
    _logo_pixmap = pixmap;

    repaint();
}

void TableView::paintEvent(QPaintEvent *event)
{
    QTableView::paintEvent(event);

    if(!_logo_pixmap.isNull() && model() && !model()->hasChildren())
    {
        QPainter painter;
        painter.begin(viewport());
        painter.save();
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        double scale_factor = 1.0;

        if(_logo_pixmap.height() > viewport()->height())
        {
            scale_factor = viewport()->height() / double(_logo_pixmap.height());
        }

        QRect dest = QRect(QPoint(), _logo_pixmap.rect().size() * scale_factor);

        dest.moveCenter(viewport()->geometry().center());

        painter.drawPixmap(dest, _logo_pixmap);

        painter.restore();
        painter.end();
    }
}

void TableView::keyPressEvent(QKeyEvent *event)
{
    QTableView::keyPressEvent(event);

    if(event->matches(QKeySequence::Copy))
    {
        copy_to_clipboard();
    }
    else if(event->matches(QKeySequence::Find))
    {
        event->accept();

        emit find();
    }
    else if((event->key() == Qt::Key_Enter) || (event->key() == Qt::Key_Return))
    {
        emit activated_ex(currentIndex());
    }
}

void TableView::copy_to_clipboard()
{
    X_CALL;

    QModelIndexList list;

    QItemSelection selection = selectionModel()->selection();

    int total_row_count = 0;

    foreach(QItemSelectionRange range, selection)
    {
        total_row_count += range.height();
    }

    if(total_row_count > 500000)
    {
        // TODO replace message box

        QMessageBox::critical(this, "You do it wrong!", "Copying into system clipboard more than 500k messages it is bad idea");
    }
    else
    {
        // make MimeData for filters
        foreach(QItemSelectionRange range, selection)
        {
            for(int i = range.top(); i <= range.bottom(); ++i)
            {
                //take indexes from one column, which is current

                list << model()->index(i, this->currentIndex().column());
            }
        }

        QMimeData *data = model()->mimeData(list);

        // copy text data
        data->setText(this->selected_text());

        qApp->clipboard()->setMimeData(data);
    }
}

QString TableView::selected_text() const
{
    X_CALL;

    QString result;

    QItemSelection selection = selectionModel()->selection();

    if(selection.count() == 1)
    {
        QItemSelectionRange range = selection.first();

        result = range_text(range.top(), range.bottom(), range.left(), range.right());
    }
    else if(!selection.isEmpty())
    {
        int top = selection.first().top();
        int bottom = selection.first().bottom();
        int left = selection.first().left();
        int right = selection.first().right();

        foreach(QItemSelectionRange range, selection)
        {
            if(range.top() < top)
            {
                top = range.top();
            }

            if(range.bottom() > bottom)
            {
                bottom = range.bottom();
            }

            if(range.left() < left)
            {
                left = range.left();
            }

            if(range.right() > right)
            {
                right = range.right();
            }
        }

        int width = right - left + 1;
        int height = bottom - top + 1;

        char * selection_matrix = new char[width * height];

        memset(selection_matrix, 0, sizeof(char) * width * height);

        foreach(QItemSelectionRange range, selection)
        {
            for(int i = range.top(); i <= range.bottom(); ++i)
            {
                for(int j = range.left(); j <= range.right(); ++j)
                {
                    if(!this->isColumnHidden(j))
                    {
                        selection_matrix[(j - left) + (i - top) * width] = 1;
                    }
                }
            }
        }

        QList<int> last_layout_cols;
        QList<int> layout_rows;

        for(int i = 0; i < height; ++i)
        {
            QList<int> current_layout_cols;

            for(int j = 0; j < width; ++j)
            {
                if(selection_matrix[j + i * width])
                {
                    current_layout_cols.append(j + left);
                }
            }

            if(!current_layout_cols.isEmpty())
            {
                if(current_layout_cols != last_layout_cols)
                {
                    if(!last_layout_cols.isEmpty())
                    {
                        result += layout_text(last_layout_cols, layout_rows) + "\n\n";
                    }

                    last_layout_cols = current_layout_cols;

                    layout_rows.clear();
                }

                layout_rows.append(i + top);
            }
        }

        result += layout_text(last_layout_cols, layout_rows);
    }

    return result;
}

QString TableView::layout_text(const QList<int> &layout_cols, const QList<int> &layout_rows) const
{
    X_CALL;

    QString result;

    if(layout_cols.isEmpty())
    {
        return result;
    }

    QVector<int> widths(layout_cols.size() - 1);

    if(!widths.isEmpty())
    {
        // Align header

        for(int j_index = 0; j_index < layout_cols.size() - 1; ++j_index)
        {
            int j = layout_cols[j_index];

            int strlen = this->model()->headerData(j, Qt::Horizontal, Qt::DisplayRole).toString().length();

            if(strlen > widths[j_index])
            {
                widths[j_index] = strlen;
            }
        }

        // Align table columns (except last column)

        foreach(int i, layout_rows)
        {
            for(int j_index = 0; j_index < layout_cols.size() - 1; ++j_index)
            {
                int j = layout_cols[j_index];

                int strlen = this->model()->index(i, j).data(Qt::DisplayRole).toString().length();

                if(strlen > widths[j_index])
                {
                    widths[j_index] = strlen;
                }
            }
        }
    }

    // Add header text

    for(int j_index = 0; j_index < layout_cols.size(); ++j_index)
    {
        int j = layout_cols[j_index];

        QString str = this->model()->headerData(j, Qt::Horizontal, Qt::DisplayRole).toString();

        result += str;

        if(j_index != layout_cols.size() - 1)
        {
            result += QString(' ').repeated(widths[j_index] - str.length() + 1) + "| ";
        }
    }

    // Add table text

    foreach(int i, layout_rows)
    {
        if(!result.isEmpty()) result += "\n";

        QString line;

        for(int j_index = 0; j_index < layout_cols.size(); ++j_index)
        {
            int j = layout_cols[j_index];

            QString str = this->model()->data(this->model()->index(i, j), Qt::DisplayRole).toString();

            line += str;

            if(j_index != layout_cols.size() - 1)
            {
                line += QString(' ').repeated(widths[j_index] - str.length() + 1) + "| ";
            }
        }

        result += line;
    }

    return result;
}

QString TableView::range_text(int top, int bottom, int left, int right) const
{
    X_CALL;

    QString result;

    QVector<int> widths(right - left + 1);

    if(widths.size() > 1)
    {
        // Align header

        for(int i = top; i <= bottom; ++i)
        {
            for(int j = left; j <= right - 1; ++j)
            {
                if(!this->isColumnHidden(j))
                {
                    int strlen = this->model()->index(i, j).data(Qt::DisplayRole).toString().length();

                    if(strlen > widths[j - left])
                    {
                        widths[j - left] = strlen;
                    }
                }
            }
        }

        // Align table columns (except last column)

        for(int j = left; j <= right - 1; ++j)
        {
            if(!this->isColumnHidden(j))
            {
                int strlen = this->model()->headerData(j, Qt::Horizontal, Qt::DisplayRole).toString().length();

                if(strlen > widths[j - left])
                {
                    widths[j - left] = strlen;
                }
            }
        }
    }

    // Add header text

    for(int j = left; j <= right; ++j)
    {
        if(!this->isColumnHidden(j))
        {
            QString str = this->model()->headerData(j, Qt::Horizontal, Qt::DisplayRole).toString();

            result += str;

            if(j != right)
            {
                result += QString(' ').repeated(widths[j - left] - str.length() + 1) + "| ";
            }
        }
    }

    // Add table text

    for(int i = top; i <= bottom; ++i)
    {
        if(!result.isEmpty()) result += "\n";

        QString line;

        for(int j = left; j <= right; ++j)
        {
            if(!this->isColumnHidden(j))
            {
                QString str = this->model()->data(this->model()->index(i, j), Qt::DisplayRole).toString();

                line += str;

                if(j != right)
                {
                    line += QString(' ').repeated(widths[j - left] - str.length() + 1) + "| ";
                }
            }
        }

        result += line;
    }

    return result;
}

ListView::ListView(QWidget *parent):
    QListView(parent)
{
    setUniformItemSizes(true);
    setAlternatingRowColors(false);

    setItemDelegate(new TreeItemDelegate(this));

    connect(this, &QAbstractItemView::doubleClicked, this, &ListView::activated_ex);
}

void ListView::set_logo(const QPixmap &pixmap)
{
    _logo_pixmap = pixmap;

    repaint();
}

void ListView::paintEvent(QPaintEvent *event)
{
    QListView::paintEvent(event);

    if(!_logo_pixmap.isNull() && model() && !model()->hasChildren())
    {
        QPainter painter;
        painter.begin(viewport());
        painter.save();
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        double scale_factor = 1.0;

        if(_logo_pixmap.height() > viewport()->height())
        {
            scale_factor = viewport()->height() / double(_logo_pixmap.height());
        }

        QRect dest = QRect(QPoint(), _logo_pixmap.rect().size() * scale_factor);

        dest.moveCenter(viewport()->geometry().center());

        painter.drawPixmap(dest, _logo_pixmap);

        painter.restore();
        painter.end();
    }
}

void ListView::keyPressEvent(QKeyEvent *event)
{
    QListView::keyPressEvent(event);

    if((event->key() == Qt::Key_Enter) || (event->key() == Qt::Key_Return))
    {
        emit activated_ex(currentIndex());
    }
}
