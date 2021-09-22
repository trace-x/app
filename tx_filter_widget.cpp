#include "tx_filter_widget.h"
#include "ui_tx_filter_widget.h"

#include <QCompleter>
#include <QStackedLayout>
#include <QPushButton>
#include <QShortcut>
#include <QKeyEvent>

#include "trace_controller.h"
#include "completer.h"
#include "common_ui_tools.h"
#include "settings.h"

#include "trace_x/trace_x.h"

TxFilterWidget::TxFilterWidget(TransmitterModelService &model_service, QWidget *parent):
    QFrame(parent),
    ui(new Ui::TxFilterWidget),
    _model_service(model_service)
{
    X_CALL;

    ui->setupUi(this);

    ui->filter_widget->set_auto_hide(false);

    //

    restore();

    connect(ui->filter_widget, &TraceFilterWidget::filter_changed, this, &TxFilterWidget::show_changed);

    //

    QStackedLayout *stack_layout = new QStackedLayout;

    stack_layout->setStackingMode(QStackedLayout::StackAll);

    ui->completer_frame->setLayout(stack_layout);

    _model_view = new ModelTreeView;
    _model_view->setObjectName("model_view");

    _model_view->set_logo(QPixmap(":/icons/model_logo"));
    _model_view->setModel(_model_service.item_model());

    ui->filter_widget->filter_view()->set_logo(QPixmap(":/icons/capture_filter_blue"));

    TraceCompleter *completer = new TraceCompleter(x_settings().capture_layout, _model_service.trace_controller(), ui->locator_line_edit,
                                                   this, false, this);

    stack_layout->addWidget(completer->completer_view());
    stack_layout->addWidget(_model_view);

    completer->completer_view()->setVisible(false);

    completer->completer_view()->setObjectName("tx_completer");
    completer->completer_view()->setStyleSheet(qApp->styleSheet());
    completer->set_grip_enabled(false);

    ui->locator_line_edit->setPlaceholderText(tr("Type to locate (%1)").arg("Ctrl + K"));

    //

    connect(_model_view, &QTreeView::doubleClicked, this, &TxFilterWidget::add_items);

    //

    connect(ui->button_box->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &TxFilterWidget::apply_filter);
    connect(ui->button_box->button(QDialogButtonBox::Ok), &QPushButton::clicked, [this] { apply_filter(); hide();});
    connect(ui->button_box->button(QDialogButtonBox::Cancel), &QPushButton::clicked, [this] { restore(); hide();});

    //

    QShortcut *shortcut = new QShortcut(QKeySequence::Save, this);

    connect(shortcut, &QShortcut::activated, this, &TxFilterWidget::apply_filter);
}

TxFilterWidget::~TxFilterWidget()
{
    X_CALL;

    delete ui;
}

void TxFilterWidget::add_selected()
{
    X_CALL;

    current_filter_model()->drop_to(_model_service.item_model()->drag_from(_model_view->selectionModel()->selectedIndexes()), ui->filter_widget->filter_view()->currentIndex());
}

void TxFilterWidget::add_items(const QModelIndex &index)
{
    X_CALL;

    current_filter_model()->drop_to(_model_service.item_model()->drag_from(QModelIndexList() << index), ui->filter_widget->filter_view()->currentIndex());
}

FilterModel *TxFilterWidget::current_filter_model() const
{
    return ui->filter_widget->current_filter_model();
}

void TxFilterWidget::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape)
    {
        hide();
    }

    QFrame::keyPressEvent(event);
}

void TxFilterWidget::apply_filter()
{
    X_CALL;

    _model_service.set_filter(_filter_model, ui->filter_widget->current_filter_index());

    clear_changes();
}

void TxFilterWidget::restore()
{
    _filter_model = *_model_service.filter_model();

    QList<EntityClass> tx_filter_entity_list;

    tx_filter_entity_list << ModuleNameEntity     <<
                             ProcessNameEntity    <<
                             ProcessIdEntity      <<
                             ProcessUserEntity    <<
                             ClassNameEntity      <<
                             SourceNameEntity     <<
                             FunctionNameEntity   <<
                             MessageTypeEntity    <<
                             ThreadIdEntity       <<
                             ContextIdEntity      <<
                             MessageTextEntity;

    ui->filter_widget->initialize(&_filter_model, _model_service.active_filter_index(),
                                  tr("Capture filter"), _model_service.trace_controller(), tx_filter_entity_list);

    ui->filter_widget->update();

    clear_changes();
}

void TxFilterWidget::show_changed()
{
    X_CALL;

    setWindowTitle(tr("Capture filter") + " *");
}

void TxFilterWidget::clear_changes()
{
    X_CALL;

    setWindowTitle(tr("Capture filter"));
}
