#include "trace_table_view.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QHeaderView>
#include <QHeaderView>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>
#include <QToolButton>
#include <QMessageBox>

#include "trace_table_view.h"
#include "settings.h"

#include "trace_x/trace_x.h"

class ScrollBar : public QScrollBar
{
public:
    ScrollBar(TraceTableView *parent = 0):
        QScrollBar(parent),
        _view(parent)
    {
        QColor color = x_settings().search_highlight_color;

        color.setAlpha(150);

        _pen.setColor(color);
        _pen.setWidth(3);
    }

    void set_markers(const QVector<size_t> &markers)
    {
        _markers = markers;

        update();
    }

protected:
    void paintEvent(QPaintEvent *event)
    {
        QScrollBar::paintEvent(event);

        if(!_markers.empty() && _view->model()->rowCount())
        {
            QStyleOptionSlider opt;

            initStyleOption(&opt);

            QRect sr = this->style()->subControlRect(QStyle::CC_ScrollBar, &opt,
                                                     QStyle::SC_ScrollBarGroove, this);

            QPainter painter;

            painter.begin(this);

            painter.save();

            painter.setPen(_pen);

            qreal dh = qreal(sr.height()) / qreal(_view->model()->rowCount());

            //draw cache map for speedup
            int map[100000] = {};

            for(int i = 0; i < _markers.size(); ++i)
            {
                int y = qFloor(_markers[i] * dh);

                if(!map[y])
                {
                    map[y] = 1;

                    y += sr.top();

                    painter.drawLine(sr.left() + 3, y, sr.right(), y);
                }
            }

            painter.restore();

            painter.end();
        }
    }

private:
    QVector<size_t> _markers;

    TraceTableView *_view;

    QPen _pen;
};

class HeaderView : public QHeaderView
{
public:
    HeaderView(Qt::Orientation orientation, QWidget * parent = 0):
        QHeaderView(orientation, parent)
    {
        setSectionsMovable(true);

        _button = new QToolButton(this);

        //TODO
        _button->setIcon(QIcon(":/icons/blob"));
        _button->setVisible(false);
    }

    void set_selected(bool current)
    {
        _button->setVisible(current);
    }

    bool is_selected() const
    {
        return _button->isVisible();
    }

protected:
    void resizeEvent(QResizeEvent *event)
    {
        QHeaderView::resizeEvent(event);

        QRect geometry(0, 0, 20, this->height());

        geometry.moveTopLeft(QPoint(0, 0));

        _button->setGeometry(geometry);
    }

private:
    QToolButton *_button;
};

TraceTableView::TraceTableView(QWidget *parent):
    TableView(parent),
    _model(0),
    _auto_scroll(false),
    _current_message_index(0),
    _selected_message_id(0),
    _selected_column(0)
{
    X_CALL;

    setAcceptDrops(true);
    setAutoScroll(true);

    setDragDropMode(QAbstractItemView::DragDrop);
    setDragDropOverwriteMode(true);

    setVerticalScrollBar(_scrollbar = new ScrollBar(this));
    setHorizontalHeader(new HeaderView(Qt::Horizontal));

    setSelectionBehavior(QTableView::SelectRows);
    verticalHeader()->setVisible(false);

    setWordWrap(true);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setMinimumSectionSize(15);

    _item_delegate = new TableItemDelegate(this);
    _message_item_delegate = new SearchItemDelegate(this);

    setItemDelegate(_item_delegate);
    setItemDelegateForColumn(TraceTableModel::Message, _message_item_delegate);

    _filter_widget = new TraceFilterWidget(this);
    _filter_widget->set_hideable(true);
    _filter_widget->set_bright_theme();
    _filter_widget->setGeometry(QRect(0, 0, 300, 100));
    _filter_widget->show();
}

void TraceTableView::initialize(TraceController *controller, TraceTableModel *model)
{
    X_CALL;

    set_model(model);

    _controller = controller;

    connect(_controller, &TraceController::model_updated, this, &TraceTableView::update_columns);

    connect(&_controller->trace_model_service(), &TraceModelService::update_search, this, &TraceTableView::update_search);

    connect(model, &TraceTableModel::refiltered, this, &TraceTableView::restore_selected_index);
    connect(_filter_widget, &TraceFilterWidget::filter_changed, this, &TraceTableView::restore_selected_index, Qt::QueuedConnection);

    update_columns();
}

void TraceTableView::set_model(TraceTableModel *model)
{
    X_CALL;

    QTableView::setModel(model);

    _model = model;

    connect(selectionModel(), &QItemSelectionModel::currentChanged, this, &TraceTableView::slot_current_changed);
    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &TraceTableView::slot_selection_changed);

    connect(model, &TraceTableModel::cleaned, this, &TraceTableView::reset_current);
    connect(model, &TraceTableModel::cleaned, this, &TraceTableView::clear_search);

    connect(model, &TraceTableModel::layoutChanged, this, &TraceTableView::update_scroll);
}

void TraceTableView::set_filter(FilterChain filter)
{
    X_CALL;

    _filter_set = filter;

    _model->set_data(_controller->trace_model_service().request_trace_model(filter));
}

void TraceTableView::set_autoscroll(bool enabled)
{
    _auto_scroll = enabled;

    update_scroll();
}

void TraceTableView::setup_font()
{
    X_CALL;

    // update grid size

    if(_model)
    {
        QFontMetrics font_metrics(font());

        for(int i = 0; i < _model->columnCount(); ++i)
        {
            setColumnWidth(i, font_metrics.width(_model->column_size_hint(i)));
        }

        int font_h = QFontMetrics(font()).height() + 5;

        verticalHeader()->setDefaultSectionSize(font_h);
    }
}

void TraceTableView::keyPressEvent(QKeyEvent *event)
{
    X_CALL;

    QKeySequence key(event->modifiers() | event->key());

    if(key == QKeySequence(Qt::CTRL + Qt::Key_Left))
    {
        select_call();

        return;
    }

    if(key == QKeySequence(Qt::CTRL + Qt::Key_Right))
    {
        select_return();

        return;
    }

    if(key == QKeySequence(Qt::CTRL + Qt::Key_Down))
    {
        jump_to_next_call();

        return;
    }

    TableView::keyPressEvent(event);

    if(key == QKeySequence::FindNext)
    {
        find_next();

        return;
    }

    if(key == QKeySequence::FindPrevious)
    {
        find_prev();

        return;
    }

    //    if(event->modifiers() & Qt::AltModifier)
    //    {
    //        setSelectionBehavior(QTableView::SelectItems);
    //    }

//    if(key == QKeySequence(Qt::CTRL + Qt::Key_E))
//    {
//        exclude_selected();

//        return;
//    }
}

void TraceTableView::keyReleaseEvent(QKeyEvent *event)
{
    X_CALL;

    QTableView::keyReleaseEvent(event);

    setSelectionBehavior(QTableView::SelectRows);
}

void TraceTableView::wheelEvent(QWheelEvent *event)
{
    X_CALL;

    if(event->modifiers() & Qt::ControlModifier)
    {
        int numSteps = event->delta() / 120;

        numSteps /= qAbs(numSteps);

        QFont current_font = font();

        current_font.setPixelSize(current_font.pixelSize() + numSteps);

        setFont(current_font);

        return;
    }

    QTableView::wheelEvent(event);
}

void TraceTableView::mousePressEvent(QMouseEvent *event)
{
    emit table_activated();

    if(event->button() == Qt::RightButton)
    {
        setSelectionBehavior(QTableView::SelectItems);
    }

    QMouseEvent levent(event->type(), event->localPos(), event->windowPos(), event->screenPos(), Qt::LeftButton, Qt::LeftButton, event->modifiers());

    QTableView::mousePressEvent(&levent);
}

void TraceTableView::mouseMoveEvent(QMouseEvent *event)
{
    QMouseEvent levent(event->type(), event->localPos(), event->windowPos(), event->screenPos(), Qt::LeftButton, Qt::LeftButton, event->modifiers());

    QTableView::mouseMoveEvent(&levent);
}

void TraceTableView::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
    {
        setSelectionBehavior(QTableView::SelectRows);
    }

    QMouseEvent levent(event->type(), event->localPos(), event->windowPos(), event->screenPos(), Qt::LeftButton, Qt::LeftButton, event->modifiers());

    QTableView::mouseReleaseEvent(&levent);
}

void TraceTableView::resizeEvent(QResizeEvent *event)
{
    TableView::resizeEvent(event);

    QRect geometry = _filter_widget->geometry();

    if(!verticalScrollBar()->isVisible())
    {
        geometry.moveTopRight(QPoint(this->width(), 0));
    }
    else
    {
        geometry.moveTopRight(QPoint(this->width() - this->verticalScrollBar()->width(), 0));
    }

    _filter_widget->setGeometry(geometry);
}

void TraceTableView::dragEnterEvent(QDragEnterEvent *event)
{
    X_CALL;

    if(event->mimeData()->formats().contains("application/trace.items.list"))
    {
        _filter_widget->show_content();

        event->acceptProposedAction();
    }
}

void TraceTableView::dragLeaveEvent(QDragLeaveEvent *event)
{
    X_CALL;

    if(!_filter_widget->rect().contains(_filter_widget->mapFromGlobal(QCursor::pos())))
    {
        QTimer::singleShot(300, _filter_widget, SLOT(hide_content()));
    }
}

void TraceTableView::dropEvent(QDropEvent *event)
{
    X_CALL;

    _filter_widget->current_filter_model()->drop_to(event->mimeData()->data(event->mimeData()->formats().first()));

    event->acceptProposedAction();
}

bool TraceTableView::event(QEvent *event)
{
    if(event->type() == QEvent::FontChange)
    {
        setup_font();
    }

    return QTableView::event(event);
}

void TraceTableView::slot_current_changed(const QModelIndex &current, const QModelIndex &prev)
{
    X_CALL;

    if(prev.row() != current.row())
    {
        const trace_message_t *message = _model->at(current.row());

        _current_message_index = message ? message->index : 0;
        //_selected_index = _current_message_index;

        emit current_changed(message);
    }
}

void TraceTableView::slot_selection_changed(const QItemSelection &, const QItemSelection &)
{
    X_CALL;

    uint64_t time = 0;

    int ranges = 0;

    QItemSelection selection = selectionModel()->selection();

    X_INFO(X_T(selection));

    foreach(QItemSelectionRange range, selection)
    {
        if(range.height() > 1)
        {
            const trace_message_t *top_message =_model->at(range.topLeft().row());
            const trace_message_t *bottom_message = _model->at(range.bottomRight().row());

            uint64_t start = top_message->timestamp + _model->controller().process_at(top_message).time_delta();
            uint64_t end = bottom_message->timestamp + _model->controller().process_at(bottom_message).time_delta();

            if(end > start)
            {
                time += end - start;
            }
            else
            {
                time += start - end;
            }

            ranges++;
        }
    }

    time = qAbs(time);

    if(!ranges)
    {
        time = 0;
    }

    emit selection_duration_changed(time);
}

void TraceTableView::reset_current()
{
    X_CALL;

    //TODO ?

    // setCurrentIndex(QModelIndex());
}

void TraceTableView::restore_selected_index()
{
    X_CALL;

    select_by_index(_selected_message_id, true, true, _selected_column);
}

void TraceTableView::exclude_selected()
{
    X_CALL;

    QItemSelection selection = selectionModel()->selection();

    //Eclude all items from the current column

    foreach(QItemSelectionRange range, selection)
    {
        for(int i = range.top(); i <= range.bottom(); ++i)
        {
            QVariant filter_data = _model->index(i, this->currentIndex().column()).data(FilterDataRole);

            if(filter_data.isValid())
            {
                _filter_widget->current_filter_model()->append_filter(filter_data.value<FilterItem>(), ExcludeOperator);
            }
        }
    }
}

void TraceTableView::update_columns()
{
    X_CALL;

    setColumnHidden(TraceTableModel::Module, _controller->items_by_class(ModuleNameEntity).size() <= 1);
    setColumnHidden(TraceTableModel::Process, _controller->items_by_class(ProcessIdEntity).size() <= 1);
    setColumnHidden(TraceTableModel::Thread, _controller->items_by_class(ThreadIdEntity).size() <= 2);
}

void TraceTableView::update_search()
{
    X_CALL;

    TraceDataModel *data = _model->data_model();

    QVector<size_t> markers;

    data->lock();

    for(size_t i = 0; i < data->size(); ++i)
    {
        if(data->at(i)->flags == SearchHighlighted)
        {
            markers.append(i);
        }
    }

    data->unlock();

    _scrollbar->set_markers(markers);

    this->repaint();
}

void TraceTableView::clear_search()
{
    X_CALL;

    _scrollbar->set_markers(QVector<size_t>());
}

void TraceTableView::update_scroll()
{
    if(_auto_scroll && !this->verticalScrollBar()->isSliderDown())
    {
        scrollToBottom();
    }
}

void TraceTableView::select_by_index(index_t message_index, bool clear_selection, bool set_current, int column)
{
    X_CALL;

    if(!_model)
    {
        return;
    }

    X_INFO("select_by_index: {}", X_T(message_index, clear_selection, set_current, column));

    _selected_message_id = message_index;
    _selected_column = column;

    _item_delegate->set_highlight_top_row(-1);
    _message_item_delegate->set_highlight_top_row(-1);

    TraceDataModel *data_model = _model->data_model();

    //TODO data_model->find_relative_index

    bool strong_equal = false;
    size_t selected_index = 0;
    size_t valid_index = 0;

    data_model->lock();

    X_INFO("data_model->size() = {}", data_model->size());

    if(data_model->size() > 0)
    {
        for(; selected_index < data_model->size(); ++selected_index)
        {
            if(data_model->at(selected_index)->index >= message_index)
            {
                strong_equal = (data_model->at(selected_index)->index == message_index);

                break;
            }
        }

        valid_index = selected_index;

        if(selected_index == data_model->size())
        {
            valid_index = data_model->size() - 1;
        }

        X_INFO("founded index: {} [{}]", valid_index, data_model->at(valid_index)->index);
    }

    data_model->unlock();

    if(clear_selection)
    {
        this->blockSignals(true);

        this->selectionModel()->clearSelection();

        this->blockSignals(false);
    }

    if(strong_equal || !clear_selection)
    {
        if(set_current)
        {
            this->setCurrentIndex(_model->index(valid_index, column));
        }
        else
        {
            this->selectionModel()->select(_model->index(valid_index, column), QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }
    }
    else
    {
        _item_delegate->set_highlight_top_row(selected_index);
        _message_item_delegate->set_highlight_top_row(selected_index);

        this->setCurrentIndex(QModelIndex());
    }

    if(!viewport()->rect().contains(visualRect(_model->index(valid_index, column))))
    {
        this->scrollTo(_model->index(valid_index, column), QAbstractItemView::PositionAtCenter);
    }

    this->viewport()->update();
}

void TraceTableView::select_call()
{
    X_CALL;

    if(selectionModel())
    {
        bool finded = false;

        select_by_index(_controller->get_call_index(_current_message_index, finded), false);
    }
}

void TraceTableView::select_return()
{
    X_CALL;

    if(selectionModel())
    {
        bool finded = false;

        select_by_index(_controller->get_return_index(_current_message_index, finded), false);
    }
}

void TraceTableView::jump_to_next_call()
{
    X_CALL;

    bool finded = false;

    select_by_index(_controller->get_next_call(_current_message_index, finded), true, true);
}

void TraceTableView::jump_to_prev_call()
{
    X_CALL;

    bool finded = false;

    select_by_index(_controller->get_prev_call(_current_message_index, finded), true, true);
}

bool TraceTableView::find_highlighted(size_t start, size_t end, int step)
{
    X_CALL;

    TraceDataModel *data = _model->data_model();

    for(size_t i = start; i != end; i += step)
    {
        if(data->at(i)->flags == SearchHighlighted)
        {
            data->unlock(); //because data is locked

            clearSelection();
            setCurrentIndex(this->model()->index(i, 0));

            if(!viewport()->rect().contains(visualRect(_model->index(i, 0))))
            {
                this->scrollTo(_model->index(i, 0), QAbstractItemView::PositionAtCenter);
            }

            return true;
        }
    }

    return false;
}

void TraceTableView::find_next()
{
    X_CALL;

    size_t start_message = 0;

    if(currentIndex().isValid())
    {
        start_message = currentIndex().row() + 1;
    }

    TraceDataModel *data = _model->data_model();

    data->lock();

    if(!find_highlighted(start_message, data->size(), 1))
    {
        if(!find_highlighted(0, start_message ? start_message - 1 : 0, 1))
        {
            data->unlock();
        }
    }
}

void TraceTableView::find_prev()
{
    X_CALL;

    int start_message = 0;

    if(currentIndex().isValid() && currentIndex().row())
    {
        start_message = currentIndex().row() - 1;
    }

    TraceDataModel *data = _model->data_model();

    data->lock();

    if(!find_highlighted(start_message, 0, -1))
    {
        if(!find_highlighted(data->size() ? data->size() - 1 : 0, start_message ? start_message - 1 : 0, -1))
        {
            data->unlock();
        }
    }
}

void TraceTableView::set_active(bool is_selected)
{
    X_CALL;

    X_INFO(X_T(is_selected));

    static_cast<HeaderView*>(horizontalHeader())->set_selected(is_selected);
}

bool TraceTableView::is_active() const
{
    return static_cast<HeaderView*>(horizontalHeader())->is_selected();
}

TraceTableModel *TraceTableView::model() const
{
    return _model;
}

TraceFilterWidget *TraceTableView::filter_widget() const
{
    return _filter_widget;
}

index_t TraceTableView::current_message_index() const
{
    return _current_message_index;
}

FilterChain TraceTableView::filter_set() const
{
    return _filter_set;
}
