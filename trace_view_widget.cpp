#include "trace_view_widget.h"
#include "ui_trace_view_widget.h"

#include <functional>

#include <QShortcut>
#include <QKeyEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QTextBlock>
#include <QLayout>
#include <QHeaderView>
#include <QToolButton>
#include <QApplication>

#include "settings.h"

#include "panel_container.h"
#include "profile_model.h"

#include "image_lib/image.h"

#include "trace_x/trace_x.h"

namespace
{
QIcon state_icon(const QString &on, const QString &off)
{
    QIcon icon;

    icon.addPixmap(on, QIcon::Normal, QIcon::On);
    icon.addPixmap(off, QIcon::Normal, QIcon::Off);

    return icon;
}

QAction *make_toggle_action(const QIcon &icon, const QString &text, const QKeySequence &shortcut, const QObject *receiver, const char* member, QMenu *menu)
{
    QAction *action = new QAction(icon, text, menu);
    action->setShortcut(shortcut);
    action->setCheckable(true);
    action->setChecked(false);

    receiver->connect(action, SIGNAL(toggled(bool)), receiver, member);

    menu->addAction(action);

    return action;
}

QAction *make_action(const QString &text, const QKeySequence &shortcut, QMenu *menu)
{
    QAction *action = new QAction(text, menu);
    action->setShortcut(shortcut);

    menu->addAction(action);

    return action;
}

QAction *make_action(const QString &text, const QKeySequence &shortcut, const std::function<void()> &functor, QMenu *menu, const QIcon &icon = QIcon())
{
    QAction *action = new QAction(text, menu);
    action->setShortcut(shortcut);
    action->setIcon(icon);

    action->connect(action, &QAction::triggered, functor);

    menu->addAction(action);

    return action;
}
}

TraceViewWidget::TraceViewWidget(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::TraceViewWidget),
    _trace_controller(),
    _current_duration(),
    _current_table(),
    _current_message()
{
    X_CALL;

    ui->setupUi(this);

    setAcceptDrops(true);

    _menu_bar = new QMenuBar();

    _menu_bar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    connect(ui->cancel_search_button, &QToolButton::clicked, this, &TraceViewWidget::cancel_search);

    ui->menu_panel->layout()->addWidget(_menu_bar);

    _model_view = new ModelTreeView();
    _model_view->setWindowTitle(tr("Structure"));
    _model_view->setObjectName("_model_view");

    _extra_tree_view= new GridTreeView();
    _extra_tree_view->setWindowTitle(tr("Context"));
    _extra_tree_view->setObjectName("_extra_tree_view");

    _callstack_view = new TableView();
    _callstack_view->verticalHeader()->hide();

    _callstack_view->setWindowTitle(tr("Call Stack"));
    _callstack_view->setObjectName("_callstack_view");
    _callstack_view->setFrameStyle(QFrame::NoFrame);

    _issues_view = new ListView();
    _issues_view->setWindowTitle(tr("Issues"));
    _issues_view->setObjectName("_issues_view");

    //

    _preview_dock = new QStackedWidget();
    _preview_dock->setWindowTitle(tr("Data Preview"));

    _text_preview = new QTextBrowser();
    _text_preview->setFrameStyle(QFrame::NoFrame);
    _text_preview->setReadOnly(true);
    _text_preview->setWordWrapMode(QTextOption::NoWrap);
//    _text_preview->addAction(new QAction("ne kokay", _text_preview));
//    _text_preview->setContextMenuPolicy(Qt::ActionsContextMenu);

    _image_preview = new ImageView();
    _image_preview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    _image_preview_scene = new ImageScene(this);
    _image_preview->setScene(_image_preview_scene);

    _preview_dock->addWidget(_text_preview);
    _preview_dock->addWidget(_image_preview);

    //

    _code_browser = new CodeBrowser();
    _code_browser->setWindowTitle(tr("Code Browser"));

    _panel_manager.add_widget(_extra_tree_view, Qt::RightEdge);

    PanelContainer *preview_panel = _panel_manager.add_widget(_preview_dock, Qt::RightEdge);
    connect(preview_panel, &PanelContainer::double_clicked, this, &TraceViewWidget::open_image_browser);

    _panel_manager.add_widget(_model_view, Qt::RightEdge);
    _panel_manager.add_widget(_callstack_view, Qt::RightEdge);
    _panel_manager.add_widget(_issues_view, Qt::RightEdge);
    _panel_manager.add_widget(_code_browser, Qt::BottomEdge);

    //

    ui->right_panel->layout()->addWidget(_panel_manager.right_container());
    ui->bottom_panel->layout()->addWidget(_panel_manager.bottom_container());

    ui->top_splitter->setStretchFactor(0, 1);
    ui->top_splitter->setStretchFactor(1, 5);

    ui->left_splitter->setStretchFactor(0, 0);
    ui->left_splitter->setStretchFactor(1, 1);
    ui->right_splitter->setStretchFactor(0, 1);
    ui->right_splitter->setStretchFactor(1, 0);

    ui->trace_view_splitter->setSizes(QList<int>() << 100 << 60 << 30);

    //

    _extra_tree_view->setItemDelegate(new TreeItemDelegate(this));

    //

    _callstack_view->horizontalHeader()->hide();
    _callstack_view->horizontalHeader()->setStretchLastSection(true);

    _issues_view->setItemDelegate(new TreeItemDelegate(this));

    //

    connect(_extra_tree_view, &ModelTreeView::find, this, &TraceViewWidget::search_by);
    connect(ui->trace_view->filter_widget()->filter_view(), &ModelTreeView::find, this, &TraceViewWidget::search_by);
    connect(ui->subtrace_view->filter_widget()->filter_view(), &ModelTreeView::find, this, &TraceViewWidget::search_by);
    connect(ui->trace_view, &TraceTableView::find, this, &TraceViewWidget::search_by);
    connect(ui->subtrace_view, &TraceTableView::find, this, &TraceViewWidget::search_by);
    connect(_model_view, &ModelTreeView::find, this, &TraceViewWidget::search_by);

    //

    ui->trace_view->set_logo(QPixmap(":/icons/trace_logo"));

    _issues_view->set_logo(QPixmap(":/icons/issue_logo"));
    _callstack_view->set_logo(QPixmap(":/icons/callstak_logo"));
    _model_view->set_logo(QPixmap(":/icons/model_logo"));

    //

    _current_table = ui->trace_view;

    ui->search_widget->hide();
    ui->search_result_widget->hide();

    ui->trace_view->set_active(true);
    ui->bottom_panel->setVisible(false);
}

TraceViewWidget::~TraceViewWidget()
{
    X_CALL;

    delete ui;
}

void TraceViewWidget::show()
{
    X_CALL;

    X_VALUE("font_family", _extra_tree_view->fontInfo().family());
    X_VALUE("pixel_size", _extra_tree_view->fontInfo().pixelSize());
    X_VALUE("point_size", _extra_tree_view->fontInfo().pointSizeF());

    //

    ui->trace_view->setup_font();
    ui->subtrace_view->setup_font();

    int font_h = QFontMetrics(_callstack_view->font()).height() + 5;

    _callstack_view->verticalHeader()->setDefaultSectionSize(font_h);
}

void TraceViewWidget::initialize(TraceService *trace_service, const QByteArray &state)
{
    X_CALL;

    int index_1 = 0;
    int index_2 = 0;

    if(!state.isEmpty())
    {
        DataStream stream(state);

        stream >> index_1;
        stream >> index_2;
    }

    _trace_controller = &trace_service->trace_controller();

    connect(_trace_controller, &TraceController::cleaned, this, &TraceViewWidget::clear);
    connect(_trace_controller, &TraceController::truncated, this, &TraceViewWidget::clear);

    QList<EntityClass> filter_entity_list;

    filter_entity_list << ModuleNameEntity     <<
                          ProcessNameEntity    <<
                          ProcessIdEntity      <<
                          ProcessUserEntity    <<
                          ClassNameEntity      <<
                          SourceNameEntity     <<
                          FunctionNameEntity   <<
                          LabelNameEntity      <<
                          MessageTypeEntity    <<
                          ThreadIdEntity       <<
                          ContextIdEntity      <<
                          MessageTextEntity;

    ui->trace_view->filter_widget()->initialize(_trace_controller->trace_model_service().trace_filter_models(), index_1,
                                                tr(" Filter"), _trace_controller, filter_entity_list);

    ui->subtrace_view->filter_widget()->initialize(_trace_controller->trace_model_service().subtrace_filter_models(), index_2,
                                                   tr(" SubFilter"), _trace_controller, filter_entity_list);

    ui->trace_view->initialize(_trace_controller, _trace_table_model = make_table_model());
    ui->subtrace_view->initialize(_trace_controller, _subtrace_table_model = make_table_model());

    connect(ui->trace_view, &TraceTableView::activated_ex, this, &TraceViewWidget::filter_item_activated);

    connect(ui->trace_view, &TraceTableView::table_activated, this, &TraceViewWidget::update_activated);
    connect(ui->subtrace_view, &TraceTableView::table_activated, this, &TraceViewWidget::update_activated);

    connect(ui->trace_view, &TraceTableView::current_changed, this, &TraceViewWidget::update_current);
    connect(ui->subtrace_view, &TraceTableView::current_changed, this, &TraceViewWidget::update_current);

    connect(ui->trace_view, &TraceTableView::selection_duration_changed, this, &TraceViewWidget::update_duration);
    connect(ui->subtrace_view, &TraceTableView::selection_duration_changed, this, &TraceViewWidget::update_duration);

    //

    connect(ui->trace_view->filter_widget(), &TraceFilterWidget::filter_changed, this, &TraceViewWidget::update_trace_filter);
    connect(ui->subtrace_view->filter_widget(), &TraceFilterWidget::filter_changed, this, &TraceViewWidget::update_trace_filter);

    //

    _issues_view->setModel(_trace_controller->trace_model_service().issue_model());

    connect(_trace_controller->trace_model_service().issue_model(), &QAbstractItemModel::layoutChanged, this, &TraceViewWidget::update_issue_label);
    connect(&_trace_controller->trace_model_service(), &TraceModelService::update_search, this, &TraceViewWidget::update_preview);

    connect(_issues_view->selectionModel(), &QItemSelectionModel::currentChanged, this, &TraceViewWidget::issue_selected);
    connect(_issues_view, &ListView::activated_ex, this, &TraceViewWidget::issue_activated);

    //

    _call_stack_model = new CallstackModel(&_call_stack, *_trace_controller, this);

    _callstack_view->setModel(_call_stack_model);

    connect(_callstack_view->selectionModel(), &QItemSelectionModel::currentChanged, this, &TraceViewWidget::callstack_current_changed);
    connect(_callstack_view, &TableView::activated_ex, this, &TraceViewWidget::callstack_activated);

    //

    _extra_model = new ExtraMessageModel(*_trace_controller, this);

    _extra_tree_view->setModel(_extra_model);
    connect(_extra_tree_view, &GridTreeView::activated_ex, this, &TraceViewWidget::filter_item_activated);

    //

    _trace_locator = new TraceCompleter(x_settings().trace_completer_layout, _trace_controller, ui->main_locator_line_edit,
                                        this, false, this);

    connect(_trace_locator, &Completer::completer_shows, this, &TraceViewWidget::update_locator_pos);
    connect(_trace_locator, &Completer::entry_accepted, this, &TraceViewWidget::filter_item_activated);
    connect(_trace_locator, &Completer::find, this, &TraceViewWidget::search_by);

    connect(_trace_locator->completer_view(), &BaseTreeView::find, this, &TraceViewWidget::search_by);

    _trace_locator->completer_view()->setObjectName("main_completer");
    _trace_locator->completer_view()->setStyleSheet(qApp->styleSheet());
    _trace_locator->completer_view()->setGeometry(QRect(QPoint(0, 0), QSize(500, 300)));

    ui->main_locator_line_edit->setPlaceholderText(tr("Type to locate (%1)").arg("Ctrl + K"));

    connect(_model_view, &ModelTreeView::activated_ex, this, &TraceViewWidget::filter_item_activated);

    //

    connect(&_trace_controller->tx_model_service(), &TransmitterModelService::filter_changed, this, &TraceViewWidget::update_transmitter_filter);

    update_transmitter_filter();

    //

    connect(&_trace_controller->trace_model(), &TraceDataModel::updated, this, &TraceViewWidget::update_trace_indicators);

    //

    update_trace_filter();

    //

    _search_completer = new TraceCompleter(x_settings().trace_completer_layout, _trace_controller, ui->search_line_edit,
                                           this, true, this);

    _search_completer->set_default_filters(QSet<int>() << MessageTextEntity);
    _search_completer->set_grip_corner(Qt::TopRightCorner);
    _search_completer->completer_view()->setObjectName("search_completer");
    _search_completer->completer_view()->setStyleSheet(qApp->styleSheet());
    _search_completer->completer_view()->setGeometry(QRect(QPoint(0, 0), QSize(500, 200)));

    connect(_search_completer, &Completer::completer_shows, this, &TraceViewWidget::update_search_completer_pos);

    connect(_search_completer, &Completer::find, this, &TraceViewWidget::search_by);
    connect(_search_completer, &Completer::entry_accepted, this, &TraceViewWidget::search_by_index);
    connect(_search_completer, &Completer::text_accepted, this, &TraceViewWidget::search_by);
}

QByteArray TraceViewWidget::save_filter_state() const
{
    X_CALL;

    QByteArray state_array;

    QDataStream stream(&state_array, QIODevice::WriteOnly);

    stream << ui->trace_view->filter_widget()->current_filter_index();
    stream << ui->subtrace_view->filter_widget()->current_filter_index();

    return state_array;
}

void TraceViewWidget::restore_filter_state(const QByteArray &filter_state)
{
    X_CALL;

    int index_1 = 0;
    int index_2 = 0;

    if(!filter_state.isEmpty())
    {
        DataStream stream(filter_state);

        stream >> index_1;
        stream >> index_2;
    }

    ui->trace_view->filter_widget()->set_current_filter(index_1);
    ui->subtrace_view->filter_widget()->set_current_filter(index_2);

    update_trace_filter();
}

QByteArray TraceViewWidget::save_state() const
{
    X_CALL;

    QByteArray state_array;

    QDataStream stream(&state_array, QIODevice::WriteOnly);

    stream << ui->right_splitter->saveState();
    stream << _panel_manager.save_state();
    stream << ui->trace_view_splitter->saveState();
    stream << ui->bottom_panel_button->defaultAction()->isChecked();
    stream << ui->right_panel_button->defaultAction()->isChecked();
    stream << ui->autoscroll_button->defaultAction()->isChecked();
    stream << ui->top_splitter->saveState();
    stream << ui->bottom_splitter->saveState();

    return state_array;
}

void TraceViewWidget::restore_state(const QByteArray &state)
{
    X_CALL;

    bool auto_scroll = false;
    bool bottom_panel_visible = false;
    bool right_panel_visible = true;

    if(!state.isEmpty())
    {
        DataStream stream(state);

        ui->right_splitter->restoreState(stream.read_next_array());

        _panel_manager.restore_state(stream.read_next_array());
        ui->trace_view_splitter->restoreState(stream.read_next_array());

        stream >> bottom_panel_visible;
        stream >> right_panel_visible;
        stream >> auto_scroll;

        ui->top_splitter->restoreState(stream.read_next_array());
        ui->bottom_splitter->restoreState(stream.read_next_array());
    }

    ui->right_panel_button->defaultAction()->setChecked(right_panel_visible);
    ui->bottom_panel_button->defaultAction()->setChecked(bottom_panel_visible);
    ui->autoscroll_button->defaultAction()->setChecked(auto_scroll);
}

void TraceViewWidget::update_current(const trace_message_t *message)
{
    X_CALL;

    if(sender() != _current_table)
    {
        return;
    }

    _current_message = message;

    _extra_model->set_message(message);

    // update callstack

    if(message)
    {
        int current;

        uint64_t duration = 0;

        QList<const trace_message_t *> call_stack = _trace_controller->get_callstack(message, current, duration);

        if(!call_stack.isEmpty() && (current != -1))
        {
            _call_stack_model->set_current_message(call_stack.at(current));
        }

        _callstack_view->setWindowTitle(tr("Callstack  %1  %2 callers  %3  %4 callees").arg(QChar(UnicodeBullet)).arg(current).arg(QChar(UnicodeBullet)).arg(qMax(call_stack.size() - current - 1, 0)));

        _call_stack.set_message_list(call_stack);

        if(current != -1)
        {
            _callstack_view->setCurrentIndex(QModelIndex());
            _callstack_view->selectionModel()->select(_call_stack_model->index(current, 0), QItemSelectionModel::ClearAndSelect);
        }

        //

        X_VALUE("selected_message_index", message->index);

        TraceTableView *master = ui->trace_view;
        TraceTableView *slave = ui->subtrace_view;

        if(sender() != ui->trace_view)
        {
            master = ui->subtrace_view;
            slave = ui->trace_view;
        }

        slave->select_by_index(message->index, true, true, master->currentIndex().column());

        _current_table_index = master->currentIndex();

        _current_duration = duration;
    }
    else
    {
        _call_stack.clear();
    }

    update_preview();
    update_code_browser(message);
}

void TraceViewWidget::update_preview()
{
    X_CALL;

    if(!_current_message)
    {
        _preview_dock->setCurrentWidget(_text_preview);
        _text_preview->setPlainText(QString());

        return;
    }

    if(_current_message->type == trace_x::MESSAGE_IMAGE)
    {
        Image image = get_image(_trace_controller->data_at(_current_message));

        _image_preview_scene->set_image(image);

        _preview_dock->setCurrentWidget(_image_preview);
    }
    else
    {
        _image_preview_scene->set_image(Image());

        _preview_dock->setCurrentWidget(_text_preview);

        QString text;

        if(_current_message->type > trace_x::MESSAGE_RETURN)
        {
            text = _current_message->message_text;
        }
        else
        {
            text = _trace_controller->function_at(_current_message)->toolTip();
        }

        _text_preview->setPlainText(text);

        // draw search highlight`s

        foreach(const auto &range, _current_message->search_indexes)
        {
            QTextCursor cursor(_text_preview->document());

            QTextCharFormat format;

            format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
            format.setUnderlineColor(x_settings().search_highlight_color);
            format.setProperty(QTextCharFormat::LineHeight, 3);

            cursor.setPosition(range.first);
            cursor.setPosition(range.second, QTextCursor::KeepAnchor);
            cursor.setCharFormat(format);
        }
    }
}

//void TraceViewWidget::make_preview()
//{
//}

//void TraceViewWidget::show_preview()
//{

//}

void TraceViewWidget::update_activated()
{
    X_CALL;

    set_current_table(static_cast<TraceTableView*>(sender()));
}

void TraceViewWidget::update_subtrace_model()
{
    X_CALL;

    _model_view->setModel(_trace_controller->trace_model_service().request_index_item_model(FilterChain(ui->trace_view->filter_widget()->current_filter_model(), ui->subtrace_view->is_active() ? ui->subtrace_view->filter_widget()->current_filter_model() : 0)));
}

void TraceViewWidget::callstack_current_changed(const QModelIndex &current, const QModelIndex &prev)
{
    X_CALL;

    if(current.isValid())
    {
        ui->trace_view->select_by_index(_call_stack_model->at(current.row())->index);
    }
}

void TraceViewWidget::callstack_activated(const QModelIndex &index)
{
    X_CALL;

    //TODO ??

    if(!ui->trace_view->selectionModel()->selectedRows().isEmpty())
    {
        ui->trace_view->setCurrentIndex(ui->trace_view->selectionModel()->selectedRows().first());
    }
}

void TraceViewWidget::update_duration(uint64_t time)
{
    if(static_cast<QAbstractItemView*>(sender())->selectionModel()->selectedRows().size() == 1)
    {
        time = _current_duration;
    }

    ui->range_time_label->setText((time > 0) ? QString::number(time / 1000000000.0, 'f', 5) + " sec." : "-");
    ui->range_time_label->setToolTip((time > 0) ? QString::number(time) + " ns." : "-");
}

void TraceViewWidget::update_locator_pos()
{
    _trace_locator->completer_view()->move(_trace_locator->completer_view()->parentWidget()->mapFromGlobal(ui->main_locator_line_edit->mapToGlobal(ui->main_locator_line_edit->geometry().bottomLeft())));
}

void TraceViewWidget::update_search_completer_pos()
{
    QRect geometry = _search_completer->completer_view()->geometry();

    geometry.moveBottomLeft(_search_completer->completer_view()->parentWidget()->mapFromGlobal(ui->search_line_edit->mapToGlobal(QPoint())));

    _search_completer->completer_view()->setGeometry(geometry);
}

void TraceViewWidget::update_trace_indicators()
{
    X_CALL;

    QString text = QString::number(_trace_controller->trace_model().safe_size());

    if(size_t data_size = _trace_controller->data_storage().total_data_size())
    {
        text += QString(" %1 data: %2").arg(QChar(UnicodeBullet)).arg(string_format_bytes(data_size));

        if(size_t swap_size = _trace_controller->data_storage().swap_size())
        {
            text += QString("(swap %1)").arg(string_format_bytes(swap_size));
        }
    }

    ui->captured_label->setText(text);
}

TraceTableModel *TraceViewWidget::make_table_model()
{
    return new TraceTableModel(0,
                               *_trace_controller,
                               QList<TraceTableModel::TraceColumns>() <<
                               TraceTableModel::Number    <<
                               TraceTableModel::Timestamp <<
                               TraceTableModel::Process <<
                               TraceTableModel::Module  <<
                               TraceTableModel::Thread  <<
                               TraceTableModel::Type    <<
                               TraceTableModel::Context <<
                               TraceTableModel::Message ,this);
}

void TraceViewWidget::update_trace_filter()
{
    X_CALL;

    ui->trace_view->set_filter(FilterChain(ui->trace_view->filter_widget()->current_filter_model(), 0));
    ui->subtrace_view->set_filter(FilterChain(ui->trace_view->filter_widget()->current_filter_model(), ui->subtrace_view->filter_widget()->current_filter_model()));

    update_subtrace_model();
}

void TraceViewWidget::set_menu(QMenu *trace_menu, QMenu *help_menu)
{
    X_CALL;

    /// Trace Menu

    _menu_bar->addMenu(trace_menu);

    /// Table Menu

    QMenu *table_menu = new QMenu(tr("&Table"), _menu_bar);

    table_menu->addAction(tr("Filter by Current"), this, SLOT(subfilter_by_current_item()), QKeySequence("Enter"));
    table_menu->addAction(tr("Add Current to Filter") + "     ", this, SLOT(subfilter_add_current_item()), QKeySequence("Alt+Enter"));

    ::make_action(tr("Exclude Current"), QKeySequence(Qt::CTRL + Qt::Key_E), [this] { invoke_on_current_table(&TraceTableView::exclude_selected);}, table_menu);

    table_menu->addSeparator();

    ::make_action(tr("Find\tCtrl+F"), QKeySequence::UnknownKey, [this] { this->search();}, table_menu);

    QAction *find_next = ::make_action(tr("Find Next"), QKeySequence::FindNext, [this] { this->find_next();}, table_menu, QIcon(":/icons/down_arrow"));
    find_next->setIconVisibleInMenu(false);
    ui->find_next_button->setDefaultAction(find_next);

    QAction *find_prev = ::make_action(tr("Find Previous"), QKeySequence::FindPrevious, [this] { this->find_prev();}, table_menu, QIcon(":/icons/up_arrow"));
    find_prev->setIconVisibleInMenu(false);
    ui->find_prev_button->setDefaultAction(find_prev);

    table_menu->addSeparator();

    ::make_action(tr("Select Call"), QKeySequence("Ctrl+Left"), [this] { invoke_on_current_table(&TraceTableView::select_call);}, table_menu);
    ::make_action(tr("Select Return"), QKeySequence("Ctrl+Right"), [this] { invoke_on_current_table(&TraceTableView::select_return);}, table_menu);
    ::make_action(tr("Jump To Next Call"), QKeySequence("Ctrl+Down"), [this] { invoke_on_current_table(&TraceTableView::jump_to_next_call);}, table_menu);
    ::make_action(tr("Jump To Previous Call"), QKeySequence("Ctrl+Up"), [this] { invoke_on_current_table(&TraceTableView::jump_to_prev_call);}, table_menu);

    table_menu->addSeparator();

    table_menu->addAction(tr("Select Next Trace"), this, SLOT(select_next_table()), QKeySequence(Qt::Key_Tab));

    table_menu->addSeparator();

    // Profile action
    table_menu->addAction(QIcon(":/icons/clock"), tr("Profile ..."), this, SLOT(profile_current_table()), QKeySequence(Qt::CTRL + Qt::Key_P));

    // Image Browser action
    table_menu->addAction(QIcon(":/icons/image"), tr("Image Browser ..."), this, SLOT(open_image_browser()), QKeySequence(Qt::CTRL + Qt::Key_I));

    _menu_bar->addMenu(table_menu);

    /// Window Menu

    QMenu *window_menu = new QMenu(tr("&Window"), _menu_bar);

    QAction *action_autoscroll = make_toggle_action(::state_icon(":/icons/auto_scroll_on", ":/icons/auto_scroll_off"), tr("Auto Scroll"),
                                                    QKeySequence(Qt::CTRL + Qt::Key_R),
                                                    ui->trace_view, SLOT(set_autoscroll(bool)), window_menu);

    connect(action_autoscroll, &QAction::toggled, ui->subtrace_view, &TraceTableView::set_autoscroll);

    ui->autoscroll_button->setDefaultAction(action_autoscroll);

    //

    QAction *action_show_right_panel = make_toggle_action(::state_icon(":/icons/right_panel_on", ":/icons/right_panel"), tr("Show Right Panel"),
                                                          QKeySequence(Qt::ALT + Qt::Key_1),
                                                          ui->right_panel, SLOT(setVisible(bool)), window_menu);

    ui->right_panel_button->setDefaultAction(action_show_right_panel);

    connect(_panel_manager.right_container(), &DockWidget::visibility_changed, action_show_right_panel, &QAction::setVisible);
    connect(_panel_manager.right_container(), &DockWidget::visibility_changed, action_show_right_panel, &QAction::setChecked);
    connect(_panel_manager.right_container(), &DockWidget::visibility_changed, ui->right_panel_button, &QToolButton::setVisible);

    //

    QAction *action_show_bottom_panel = make_toggle_action(::state_icon(":/icons/bottom_panel_on", ":/icons/bottom_panel"), tr("Show Bottom Panel"),
                                                           QKeySequence(Qt::ALT + Qt::Key_2),
                                                           ui->bottom_panel, SLOT(setVisible(bool)), window_menu);

    ui->bottom_panel_button->setDefaultAction(action_show_bottom_panel);

    connect(_panel_manager.bottom_container(), &DockWidget::visibility_changed, action_show_bottom_panel, &QAction::setVisible);
    connect(_panel_manager.bottom_container(), &DockWidget::visibility_changed, action_show_bottom_panel, &QAction::setChecked);
    connect(_panel_manager.bottom_container(), &DockWidget::visibility_changed, ui->bottom_panel_button, &QToolButton::setVisible);

    //

    window_menu->addMenu(_panel_manager.panel_menu(_menu_bar));

    _menu_bar->addMenu(window_menu);

    /// Help Menu

    _menu_bar->addMenu(help_menu);
}

void TraceViewWidget::set_common_actions(QAction *capture, QAction *capture_filter, QAction *clear)
{
    X_CALL;

    ui->capture_tool_button->setDefaultAction(capture);
    ui->clear_trace_button->setDefaultAction(clear);

    ui->capture_filter_button->setIcon(capture_filter->icon());

    connect(ui->capture_filter_button, &QToolButton::clicked, capture_filter, &QAction::trigger);
}

void TraceViewWidget::keyPressEvent(QKeyEvent *event)
{
    QFrame::keyPressEvent(event);

    QKeySequence key(event->modifiers() | event->key());

    if(event->key() == Qt::Key_Escape)
    {
        cancel_search();
    }
    else if(key == QKeySequence::Find)
    {
        search();
    }
}

void TraceViewWidget::update_transmitter_filter()
{
    X_CALL;

    ui->capture_filter_button->setText(_trace_controller->capture_filter()->as_string());
}

void TraceViewWidget::update_issue_label()
{
    X_CALL;

    IssuesListModel *model = _trace_controller->trace_model_service().issue_model();

    QString issues_string = tr("Issues ");

    if(int asserts = model->issues_count(trace_x::MESSAGE_ASSERT))
    {
        issues_string += tr(" %1  %2 assert ").arg(QChar(UnicodeBullet)).arg(asserts);
    }

    if(int errors = model->issues_count(trace_x::MESSAGE_ERROR))
    {
        issues_string += tr(" %1  %2 errors ").arg(QChar(UnicodeBullet)).arg(errors);
    }

    if(int warnings = model->issues_count(trace_x::MESSAGE_WARNING))
    {
        issues_string += tr(" %1  %2 warnings ").arg(QChar(UnicodeBullet)).arg(warnings);
    }

    if(int exceptions = model->issues_count(trace_x::MESSAGE_EXCEPTION))
    {
        issues_string += tr(" %1  %2 except ").arg(QChar(UnicodeBullet)).arg(exceptions);
    }

    _issues_view->setWindowTitle(issues_string);
}

void TraceViewWidget::issue_selected(const QModelIndex &index)
{
    X_CALL;

    if(index.isValid())
    {
        index_t message_index = _trace_controller->trace_model_service().issue_model()->data_model()->at(index.row())->index;

        X_VALUE("selected_message_index", message_index);

        ui->trace_view->select_by_index(message_index);
        ui->subtrace_view->select_by_index(message_index);
    }
}

void TraceViewWidget::issue_activated(const QModelIndex &index)
{
    X_CALL;

    ui->trace_view->setCurrentIndex(ui->trace_view->selectionModel()->selectedRows().first());
}

void TraceViewWidget::search()
{
    X_CALL;

    search_by_filter(_current_table->currentIndex().data(FilterDataRole).value<FilterItem>(), false);
}

void TraceViewWidget::search_by()
{
    X_CALL;

    FilterItem search_filter;

    bool find_next = true;

    if(QAbstractItemView *view = qobject_cast<QAbstractItemView*>(sender()))
    {
        search_filter = view->currentIndex().data(FilterDataRole).value<FilterItem>();

        if(qobject_cast<QTableView*>(sender()))
        {
            find_next = false;
        }
    }
    else if(sender() == _trace_locator)
    {
        search_filter = FilterItem(_trace_locator->current_text(), _trace_locator->current_class());
    }
    else if(sender() == _search_completer)
    {
        search_filter = FilterItem(_search_completer->current_text(), _search_completer->current_class());
    }

    search_by_filter(search_filter, find_next);
}

void TraceViewWidget::search_by_index(const QModelIndex &index)
{
    X_CALL;

    search_by_filter(index.data(FilterDataRole).value<FilterItem>());
}

void TraceViewWidget::search_by_filter(const FilterItem &search_filter, bool find_next)
{
    X_CALL;

    X_INFO("search: {}", search_filter.string_id());

    QString string_id = search_filter.string_id();

    _search_completer->set_current_class(EntityClass(search_filter.class_id()), string_id);

    if(!ui->search_widget->isVisible())
    {
        ui->search_widget->show();
        ui->search_result_widget->show();
    }

    ui->search_line_edit->setFocus();
    ui->search_line_edit->setSelection(ui->search_line_edit->text().length() - string_id.length(), string_id.length());

    uint64_t found = _trace_controller->trace_model_service().search(search_filter, _search_completer->default_filters());

    _current_search_filter = search_filter;

    ui->found_label->setText(QString(tr("%1 message found").arg(found)));

    if(find_next)
    {
        QTimer::singleShot(0, _current_table, SLOT(find_next()));
    }
}

void TraceViewWidget::find_next()
{
    X_CALL;

    update_search();

    _current_table->find_next();
}

void TraceViewWidget::find_prev()
{
    X_CALL;

    update_search();

    _current_table->find_prev();
}

void TraceViewWidget::update_search()
{
    X_CALL;

    if(!(_current_search_filter == _search_completer->current_filter()))
    {
        _current_search_filter = _search_completer->current_filter();

        uint64_t found = _trace_controller->trace_model_service().search(_current_search_filter, _search_completer->default_filters());

        ui->found_label->setText(QString(tr("%1 message found").arg(found)));
    }
}

void TraceViewWidget::cancel_search()
{
    X_CALL;

    if(ui->search_widget->isVisible())
    {
        ui->search_widget->hide();
        ui->search_result_widget->hide();

        _trace_controller->trace_model_service().search(FilterItem());

        _current_table->setFocus();
    }
    else
    {
        _current_table->clearSelection();
        _current_table->setCurrentIndex(QModelIndex());
    }
}

void TraceViewWidget::update_code_browser(const trace_message_t *message)
{
    X_CALL;

    if(message)
    {
        QString source_path = _trace_controller->source_at(message)->descriptor().id.toString();

        if(!source_path.isEmpty())
        {
            source_path = _trace_controller->trace_model_service().map_source_file(source_path);

            if(QFile::exists(source_path))
            {
                QFile file(source_path);

                if(file.open(QFile::ReadOnly | QFile::Text))
                {
                    _code_browser->setPlainText(file.readAll());

                    QTextCursor text_cursor(_code_browser->document()->findBlockByLineNumber(message->source_line - 1));

                    _code_browser->setTextCursor(text_cursor);

                    return;
                }
            }
        }
    }

    _code_browser->clear();
}

void TraceViewWidget::subfilter_by_current_item()
{
    X_CALL;

    if(_current_table)
    {
        filter_item_activated(_current_table->currentIndex());
    }
}

void TraceViewWidget::subfilter_add_current_item()
{
    X_CALL;

    if(_current_table)
    {
        QVariant filter_data = _current_table->currentIndex().data(FilterDataRole);

        if(filter_data.isValid())
        {
            ui->subtrace_view->filter_widget()->current_filter_model()->append_filter(filter_data.value<FilterItem>());
        }
    }
}

void TraceViewWidget::dragEnterEvent(QDragEnterEvent *e)
{
    X_CALL;

    if(e->mimeData()->hasUrls() && (e->mimeData()->urls().count() == 1))
    {
        e->acceptProposedAction();
    }
}

void TraceViewWidget::dropEvent(QDropEvent *e)
{
    X_CALL;

    X_INFO("dropped mime formats: {}", e->mimeData()->formats());

    _trace_controller->load_trace(e->mimeData()->urls().first().toLocalFile());
}

void TraceViewWidget::clear()
{
    X_CALL;

    _current_message = nullptr;

    _trace_locator->clear();
    _search_completer->clear();

    _callstack_view->setWindowTitle(tr("Callstack"));

    _text_preview->clear();
    _image_preview_scene->set_image(Image());
    _call_stack_model->set_current_message(0);
    _extra_model->set_message(0);

    update_code_browser(0);

    _call_stack.clear();
}

void TraceViewWidget::reset_current()
{
    X_CALL;

    clear();
}

void TraceViewWidget::filter_item_activated(const QModelIndex &index)
{
    X_CALL;

    QVariant filter_data = index.data(FilterDataRole);

    if(filter_data.isValid())
    {
        //don`t clear, when ALT key is pressed
        if(!qApp->keyboardModifiers().testFlag(Qt::AltModifier))
        {
            ui->subtrace_view->filter_widget()->current_filter_model()->clear();
        }

        ui->subtrace_view->filter_widget()->current_filter_model()->append_filter(filter_data.value<FilterItem>());
    }
}

void TraceViewWidget::invoke_on_current_table(table_method_t method)
{
    X_CALL;

    if(_current_table)
    {
        (_current_table->*method)();
    }
}

void TraceViewWidget::profile_current_table()
{
    X_CALL;

    ProfilerTable *table = new ProfilerTable(_trace_controller, _current_table->model()->data_model(), this);

    connect(table, &ProfilerTable::activated_ex, this, &TraceViewWidget::filter_item_activated);
    connect(table, &ProfilerTable::find, this, &TraceViewWidget::search_by);

    connect(_trace_controller, &TraceController::cleaned, table->model(), &ProfileModel::clear);
    connect(_trace_controller, &TraceController::truncated, table->model(), &ProfileModel::clear);

    Dialog *profiler_dialog = new Dialog(table, this);

    profiler_dialog->setAttribute(Qt::WA_DeleteOnClose);
    profiler_dialog->setGeometry(this->geometry().adjusted(this->width() / 5, this->height() / 4, -this->width() / 5, -this->height() / 4));

    profiler_dialog->show();
}

void TraceViewWidget::open_image_browser()
{
    X_CALL;

    index_t index;

    TraceDataModel *image_model = _trace_controller->trace_model_service().request_image_model(_current_table->filter_set());

    image_model->find_relative_index(_current_table->current_message_index(), index);

    X_INFO("image_model index: {}", index);

    ImageBrowser *image_browser = new ImageBrowser(_trace_controller, this);

    image_browser->set_data_set(image_model);

    if(image_model->size())
    {
        image_browser->set_current_index(index);
    }

    Dialog *image_dialog = new Dialog(image_browser, this);

    image_dialog->setGeometry(this->geometry().adjusted(this->width() / 5, this->height() / 4, -this->width() / 5, -this->height() / 4));
    image_dialog->setAttribute(Qt::WA_DeleteOnClose);

    image_dialog->show();
}

void TraceViewWidget::select_next_table()
{
    X_CALL;

    set_current_table(_current_table == ui->trace_view ? ui->subtrace_view : ui->trace_view);
}

void TraceViewWidget::set_current_table(TraceTableView *table)
{
    X_CALL;

    if(_current_table != table)
    {
        if(_current_table)
        {
            _current_table->set_active(false);
        }

        _current_table = table;

        _current_table->setFocus();
        _current_table->set_active(true);

        update_subtrace_model();
    }
}
