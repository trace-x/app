#include "common_ui_tools.h"

#include <QPainter>
#include <QApplication>
#include <QHeaderView>
#include <QKeyEvent>
#include <QClipboard>
#include <QMimeData>
#include <QHBoxLayout>

#include "trace_data_model.h"
#include "trace_table_model.h"
#include "filter_model.h"

#include "trace_x/trace_x.h"

#include "settings.h"

namespace
{
    void align_decorator_rect(QRect &rect, Qt::Alignment alignment)
    {
        int x = rect.x();
        int y = rect.y();
        int w = rect.height();
        int h = rect.height();

        if ((alignment & Qt::AlignVCenter) == Qt::AlignVCenter)
            y += rect.size().height()/2 - h/2;
        else if ((alignment & Qt::AlignBottom) == Qt::AlignBottom)
            y += rect.size().height() - h;
        if ((alignment & Qt::AlignRight) == Qt::AlignRight)
            x += rect.size().width() - w;
        else if ((alignment & Qt::AlignHCenter) == Qt::AlignHCenter)
            x += rect.size().width()/2 - w/2;

        rect = QRect(x, y, w, h);
    }
}

FancyItemDelegate::FancyItemDelegate(QObject *parent):
    QStyledItemDelegate(parent)
{
}

void FancyItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;

    opt.decorationSize = QSize(option.rect.height() - 4, option.rect.height() - 4);

    initStyleOption(&opt, index);

    QRect decorator_rect = option.widget->style()->subElementRect(QStyle::SE_ItemViewItemDecoration, &opt, option.widget);

    ::align_decorator_rect(decorator_rect, opt.decorationAlignment);

    QStyledItemDelegate::paint(painter, opt, index);

    QVariant decoration = index.data(Qt::DecorationRole);

    if(decoration.type() == QVariant::Color)
    {
        painter->save();

        painter->setPen(QPen(x_settings().main_color, 1));
        painter->setBrush(qvariant_cast<QColor>(decoration));

        painter->drawRect(decorator_rect);

        painter->restore();
    }
}

////////////////////////////////

TableItemDelegate::TableItemDelegate(QObject *parent):
    FancyItemDelegate(parent),
    _highlight_top_row(-1)
{
}

void TableItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    FancyItemDelegate::paint(painter, option, index);

    if(index.row() == _highlight_top_row)
    {
        painter->save();

        painter->setPen(QPen(x_settings().line_highlight_color, 1));

        painter->drawLine(option.rect.topLeft(), option.rect.topRight());

        painter->restore();
    }
    else if(index.row() == _highlight_top_row - 1)
    {
        painter->save();

        painter->setPen(QPen(x_settings().line_highlight_color, 1));

        painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());

        painter->restore();
    }
}

void TableItemDelegate::set_highlight_top_row(int row)
{
    _highlight_top_row = row;
}

////////////////////////////////

SearchItemDelegate::SearchItemDelegate(QObject *parent):
    TableItemDelegate(parent)
{
}

void SearchItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    TableItemDelegate::paint(painter, option, index);

    QVariant highlight = index.data(SearchHighlightDataRole);

    if(highlight.type() == QVariant::Color)
    {
        QColor color = qvariant_cast<QColor>(highlight);

        painter->save();

        painter->setRenderHint(QPainter::Antialiasing);

        QPen ppen(color, option.rect.height() / 2);

        ppen.setCapStyle(Qt::RoundCap);
        painter->setPen(ppen);

        painter->setBrush(color);

        painter->drawPoint(option.rect.right() - option.rect.height() / 2, option.rect.center().y() + 1);

        //painter->drawRect(option.rect.right() - 20, option.rect.top(), 20, option.rect.height());

        /// draw search underlines

        QVector<QPair<int, int>> search_ranges = index.data(SearchIndexesDataRole).value<QVector<QPair<int, int>>>();

        if(!search_ranges.empty())
        {
            int char_width = option.fontMetrics.averageCharWidth();

            painter->setPen(QPen(color, 2));

            int skip = index.data(FunctionLevelDataRole).toInt() * 2;

            int y = option.rect.bottomLeft().y() - 1;

            foreach(const auto &range, search_ranges)
            {
                int from = range.first + skip;
                int to = range.second + skip;

                int x1 = option.rect.bottomLeft().x() + from * char_width + 3; // 3 ?
                int x2 = x1 + (to - from) * char_width - 1;

                painter->drawLine(QPoint(x1, y), QPoint(x2, y));
            }
        }

        painter->restore();
    }
}

////////////////////////////////

TreeItemDelegate::TreeItemDelegate(QObject *parent):
    FancyItemDelegate(parent)
{
}

void TreeItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;

    initStyleOption(&opt, index);

    opt.state &= ~QStyle::State_MouseOver;

    FancyItemDelegate::paint(painter, opt, index);
}

////////////////////////////////

ListModel::ListModel(const QString &header, QObject *parent):
    QAbstractListModel(parent),
    _header(header)
{
}

int ListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(_data.size() + 1);
}

QVariant ListModel::data(const QModelIndex &index, int role) const
{
    if(index.row() == 0)
    {
        if(role == Qt::DisplayRole)
        {
            return _header;
        }
    }

    if(_data.size() > index.row())
    {
        if(role == Qt::DisplayRole)
        {
            return _data[index.row() - 1];
        }

        if(role == FilterDataRole)
        {
            return QVariant::fromValue(FilterItem(_data[index.row() - 1], MessageTextEntity));
        }
    }

    return QVariant();
}

Qt::ItemFlags ListModel::flags(const QModelIndex &index) const
{
    if(index.row() == 0) return Qt::NoItemFlags;

    return QAbstractListModel::flags(index);
}

void ListModel::clear()
{
    _data = QList<QString>();
}

////////////////////////////////

ModelTreeView::ModelTreeView(QWidget *parent):
    BaseTreeView(parent)
{
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    setHeaderHidden(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    header()->setStretchLastSection(false);
    setRootIsDecorated(false);
    setIndentation(1);

   // setItemsExpandable(false);
    setItemDelegate(new TreeItemDelegate(this));
}

void ModelTreeView::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);

    //    header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    //    header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    connect(model, &QAbstractItemModel::layoutChanged, this, &ModelTreeView::update_layout);

    update_layout();
}

void ModelTreeView::keyPressEvent(QKeyEvent *event)
{
    if(model())
    {
        if(event->matches(QKeySequence::Copy))
        {
            QTreeView::keyPressEvent(event);

            QMimeData *data = model()->mimeData(this->selectedIndexes());

            data->setText(qApp->clipboard()->text());

            qApp->clipboard()->setMimeData(data);

            return;
        }
    }

    BaseTreeView::keyPressEvent(event);
}

void ModelTreeView::walk_first_column(const QModelIndex &parent)
{
    if(this->model())
    {
        int row_count = model()->rowCount(parent);

        for(int i = 0; i < row_count; ++i)
        {
            QModelIndex index = this->model()->index(i, 0, parent);

            bool span = !model()->span(index).isNull();

            setFirstColumnSpanned(i, parent, span);

            walk_first_column(index);
        }
    }
}

void ModelTreeView::update_layout()
{
    if(model() && model()->hasChildren())
    {
        header()->setSectionResizeMode(0, QHeaderView::Stretch);

        expandAll();

        walk_first_column(QModelIndex());
    }
}

//////////////////////

MessageListModel::MessageListModel(TraceDataModel *data_model, QObject *parent):
    QAbstractListModel(parent),
    _data_model(data_model)
{
    connect(_data_model, SIGNAL(updated()), SIGNAL(layoutChanged()));
}

int MessageListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(_data_model->safe_size());
}

QVariant MessageListModel::data(const QModelIndex &index, int role) const
{
    _data_model->mutex()->lock();

    if(index.row() < int(_data_model->size()))
    {
        trace_message_t message = *_data_model->at(index.row());

        _data_model->mutex()->unlock();

        if(role == Qt::DisplayRole || role == Qt::ToolTipRole)
        {
            return message.message_text;
        }
        else
        {
            return QVariant();
        }
    }

    _data_model->mutex()->unlock();

    return QVariant();
}

LineEdit::LineEdit(QWidget *parent):
    QLineEdit(parent)
{
}

void LineEdit::set_tip(const QString &text)
{
    _tip = text;
}

void LineEdit::paintEvent(QPaintEvent *event)
{
    QLineEdit::paintEvent(event);

    QPainter painter(this);

    painter.setFont(this->font());
    painter.setPen(Qt::darkGray);
    painter.setRenderHint(QPainter::TextAntialiasing);

    painter.drawText(this->rect().adjusted(0, 0, -6, 0), Qt::AlignRight | Qt::AlignVCenter, _tip);
}

QStringList SIZE_ORDERS = QStringList() << QCoreApplication::tr("Bytes") <<
                                           QCoreApplication::tr("KB") <<
                                           QCoreApplication::tr("MB") <<
                                           QCoreApplication::tr("GB");

QString string_format_bytes(qint64 bytes)
{
    QString retstr;

    if(bytes > 0)
    {
        int order = log10(double(bytes)) / log10((double)1024);

        order = qMin(order, SIZE_ORDERS.size() - 1);

        retstr = QString("%1 %2").arg(bytes / pow(1024.0, order), 0, 'f', 2).arg(SIZE_ORDERS.at(order));
    }
    else
    {
        retstr = "-";
    }

    return retstr;
}

Dialog::Dialog(QWidget *content_widget, QWidget *parent):
    QDialog(parent)
{
    setWindowTitle(content_widget->windowTitle());

    setLayout(new QHBoxLayout());

    layout()->setContentsMargins(QMargins());
    layout()->addWidget(content_widget);
}
