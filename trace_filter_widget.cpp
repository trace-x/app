#include "trace_filter_widget.h"
#include "ui_trace_filter_widget.h"

#include "trace_x/trace_x.h"

#include "common_ui_tools.h"

#include "filter_item_editor.h"

#include <QComboBox>
#include <QToolButton>

namespace
{

// Returns the last visible item in the tree view or invalid model index if not found any.
QModelIndex last_visible_item(QTreeView *view, const QModelIndex &index = QModelIndex())
{
    QAbstractItemModel *model = view->model();

    int row_count = model->rowCount(index);

    if(row_count > 0)
    {
        QModelIndex last_index = model->index(row_count - 1, 0, index);

        if(model->hasChildren(last_index) && view->isExpanded(last_index))
        {
            return last_visible_item(view, last_index);
        }
        else
        {
            return last_index;
        }
    }
    else
    {
        return QModelIndex();
    }
}

}

TraceFilterWidget::TraceFilterWidget(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::TraceFilterWidget),
    _filter_model(0),
    _auto_hide(true),
    _is_hidden(false),
    _hideable(false)
{
    X_CALL;

    ui->setupUi(this);

    ui->filter_view->set_logo(QPixmap(":/icons/filter"));

    this->setMouseTracking(true);
    this->installEventFilter(this);

    connect(this, &TraceFilterWidget::_remove_row_later, this, &TraceFilterWidget::remove_row, Qt::QueuedConnection);

    ui->filter_view->setAcceptDrops(true);
    ui->filter_view->setDragDropMode(QAbstractItemView::DragDrop);
    ui->filter_view->setItemDelegate(new TreeItemDelegate(this));
    ui->filter_view->setHeaderHidden(true);
    ui->filter_view->setDragDropOverwriteMode(true);
    ui->filter_view->expandAll();

    connect(ui->filter_combo_box, SIGNAL(currentIndexChanged(int)), SLOT(switch_filter(int)));
    connect(ui->remove_tool_button, &QAbstractButton::clicked, ui->filter_view, &FilterTreeView::remove_current);
    connect(ui->add_model_button, &QAbstractButton::clicked, this, &TraceFilterWidget::add_model);
    connect(ui->remove_model_button, &QAbstractButton::clicked, this, &TraceFilterWidget::remove_current_model);
    connect(ui->filter_view, &QTreeView::doubleClicked, this, &TraceFilterWidget::edit_item);
    connect(ui->filter_view, &FilterTreeView::before_context_menu, this, &TraceFilterWidget::disbale_auto_hide);

    ui->filter_combo_box->view()->setItemDelegate(new FancyItemDelegate(ui->filter_combo_box));

    _hide_timer.setInterval(1000);
    _hide_timer.setSingleShot(true);

    connect(&_hide_timer, &QTimer::timeout, this, &TraceFilterWidget::hide_content);

    QTimer::singleShot(0, this, SLOT(hide_content()));
}

TraceFilterWidget::~TraceFilterWidget()
{
    X_CALL;

    delete ui;
}

void TraceFilterWidget::set_bright_theme()
{
    X_CALL;

    ui->add_model_button->setIcon(QIcon(":/icons/add_page_dark"));
    ui->remove_tool_button->setIcon(QIcon(":/icons/minus_dark"));
    ui->remove_model_button->setIcon(QIcon(":/icons/cross_dark"));
    ui->filter_combo_box->setProperty("dark_theme", false);

    ui->tools_panel->setProperty("dark_theme", false);
    ui->tools_panel->setProperty("bright_theme", true);
}

void TraceFilterWidget::set_hideable(bool enabled)
{
    _hideable = enabled;
}

void TraceFilterWidget::set_auto_hide(bool enabled)
{
    X_CALL;

    _auto_hide = enabled;
}

void TraceFilterWidget::disbale_auto_hide()
{
    set_auto_hide(false);
}

void TraceFilterWidget::initialize(FilterListModel *filter_model, int active_filter, const QString &title,
                                   TraceController *controller, const QList<EntityClass> &class_list)
{
    X_CALL;

    if(_filter_model)
    {
        _filter_model->disconnect(this);
    }

    _filter_model = filter_model;

    ui->filter_combo_box->setModel(_filter_model->names_model());

    connect(_filter_model->names_model(), &QStandardItemModel::layoutChanged, this, &TraceFilterWidget::update_list);

    ui->filter_combo_box->setCurrentIndex(active_filter);

    _controller = controller;
    _class_list = class_list;

    update_list();
}

void TraceFilterWidget::set_current_filter(int index)
{
    X_CALL;

    ui->filter_combo_box->blockSignals(true);
    this->blockSignals(true);

    ui->filter_combo_box->setCurrentIndex(index);

    switch_filter(index);

    ui->filter_combo_box->blockSignals(false);
    this->blockSignals(false);
}

void TraceFilterWidget::switch_filter(int index)
{
    X_CALL;

    if(current_filter_model())
    {
        current_filter_model()->disconnect(this);
        this->disconnect(current_filter_model());
    }

    FilterModel *new_model = &_filter_model->models()[index];

    connect(new_model, &QAbstractItemModel::layoutChanged, ui->filter_view, &QTreeView::expandAll);
    connect(new_model, &QAbstractItemModel::layoutChanged, this, &TraceFilterWidget::update_filter);
    connect(new_model, &QAbstractItemModel::layoutChanged, this, &TraceFilterWidget::filter_changed);

    ui->filter_view->set_model(new_model);
    ui->filter_view->expandAll();

    update_filter();

    emit filter_changed();
}

void TraceFilterWidget::add_model()
{
    X_CALL;

    int model_index = _filter_model->add_model();

    X_VALUE(model_index);

    ui->filter_combo_box->setCurrentIndex(model_index);
}

void TraceFilterWidget::remove_current_model()
{
    X_CALL;

    if(ui->filter_combo_box->count() > 1)
    {
        int index = ui->filter_combo_box->currentIndex();

        _filter_model->remove_model(index);

        if(index > 0)
        {
            index--;
        }

        ui->filter_combo_box->setCurrentIndex(index);
    }
    else
    {
        current_filter_model()->clear();
    }
}

void TraceFilterWidget::add_item()
{
    X_CALL;

    BaseFilterItem *current_item = static_cast<BaseFilterItem*>(current_filter_model()->itemFromIndex(ui->filter_view->currentIndex()));

    int current_class = -1;
    FilterOperator filter_sign = IncludeOperator;
    FilterGroup *group = 0;

    if(current_item)
    {
        group = static_cast<FilterGroup*>(current_item);

        if(current_item->type() == FilterItemType)
        {
            group = static_cast<FilterGroup*>(current_item->parent());
        }

        filter_sign = group->operator_type();

        current_class = current_item->class_id();
    }

    FilterItemEditor add_dialog(*_controller, _class_list, current_class, filter_sign, this);

    if(_hideable)
    {
        set_auto_hide(false);
    }

    if(add_dialog.exec() == QDialog::Accepted)
    {
        if(current_class == -1)
        {
            current_filter_model()->append_filter(add_dialog.filter(), add_dialog.filter_sign());
        }
        else
        {
            group->append_filter(add_dialog.filter(), add_dialog.filter_sign());
        }

        current_filter_model()->update_index_model();
    }

    if(_hideable)
    {
        set_auto_hide(true);
        _hide_timer.start();
    }
}

void TraceFilterWidget::edit_item()
{
    X_CALL;

    BaseFilterItem *current_item = static_cast<BaseFilterItem*>(current_filter_model()->itemFromIndex(ui->filter_view->currentIndex()));

    if(current_item && (current_item->type() == FilterItemType))
    {
        FilterItem *editing_filter = static_cast<FilterItem*>(current_item);
        FilterGroup *group = editing_filter->group_item();

        FilterItemEditor edit_dialog(*_controller, _class_list, *editing_filter, group->operator_type(), this);

        if(_hideable)
        {
            set_auto_hide(false);
        }

        if(edit_dialog.exec() == QDialog::Accepted)
        {
            FilterItem edited_filter = edit_dialog.filter();

            if(editing_filter->class_id() == edited_filter.class_id())
            {
                emit _remove_row_later(editing_filter->index().row(), editing_filter->index().parent());

                group->append_filter(edited_filter, edit_dialog.filter_sign(), editing_filter->index().row());
            }
            else if(editing_filter->group_item()->rowCount() == 1)
            {
                FilterGroup new_group(edit_dialog.filter_sign(), edited_filter.class_id(), QList<FilterItem>() << edit_dialog.filter(), _controller->filter_class_name(edited_filter.class_id()));

                if(group->group_item())
                {
                    group->group_item()->append_group(new_group);
                }
                else
                {
                    group->model()->append_group(new_group);
                }

                emit _remove_row_later(group->index().row(), group->index().parent());
            }

            current_filter_model()->update_index_model();
        }

        if(_hideable)
        {
            set_auto_hide(true);
            _hide_timer.start();
        }
    }
    else
    {
        add_item();
    }
}

void TraceFilterWidget::remove_row(int row, const QModelIndex &parent)
{
    X_CALL;

    emit ui->filter_view->model()->layoutAboutToBeChanged();

    ui->filter_view->model()->removeRow(row, parent);

    emit ui->filter_view->model()->layoutChanged();
}

void TraceFilterWidget::update_list()
{
    X_CALL;

    ui->remove_model_button->setVisible(ui->filter_combo_box->count() > 1);
    ui->filter_combo_box->setProperty("single_item", ui->filter_combo_box->count() <= 1);
    ui->filter_combo_box->setStyleSheet(qApp->styleSheet());
}

void TraceFilterWidget::update_filter()
{
    X_CALL;

    ui->remove_tool_button->setVisible(current_filter_model()->rowCount());

    show_content(true);
}

void TraceFilterWidget::show_content(bool hide_after_show)
{
    X_CALL;

    if(_hideable)
    {
        if(_is_hidden && hide_after_show)
        {
            _hide_timer.start();
        }

        int height = 0;

        if(ui->filter_view->model()->hasChildren())
        {
            height = ui->filter_view->visualRect(last_visible_item(ui->filter_view)).bottom() + 30;
        }
        else
        {
            height = 50;
        }

        ui->filter_view->setVisible(height);

        QRect geom = this->geometry();

        geom.setHeight(height + ui->tools_panel->height());

        setGeometry(geom);

        _is_hidden = false;
    }
}

void TraceFilterWidget::hide_content()
{
    X_CALL;

    if(_auto_hide)
    {
        QRect geom = this->geometry();

        geom.setHeight(ui->tools_panel->height());

        setGeometry(geom);

        _is_hidden = true;
    }
}

bool TraceFilterWidget::eventFilter(QObject *object, QEvent *event)
{
    if((event->type() == QEvent::ContextMenu)/* ||  (event->type() == QEvent::Leave)*/ && _hideable)
    {
        set_auto_hide(true);
    }

    if(!_auto_hide)
    {
        return false;
    }

    if(event->type() == QEvent::Enter)
    {
        ui->filter_view->show();

        show_content();

        _hide_timer.stop();

        //set_auto_hide(false);
    }
    else if(event->type() == QEvent::Leave)
    {
        _hide_timer.start();
    }

    return false;
}

FilterModel *TraceFilterWidget::current_filter_model() const
{
    return &_filter_model->models()[ui->filter_combo_box->currentIndex()];
}

FilterTreeView *TraceFilterWidget::filter_view() const
{
    return ui->filter_view;
}

QFrame *TraceFilterWidget::tool_panel()
{
    return ui->tools_panel;
}

int TraceFilterWidget::current_filter_index() const
{
    return ui->filter_combo_box->currentIndex();
}
