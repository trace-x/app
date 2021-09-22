#include "filter_item_editor.h"
#include "ui_filter_item_editor.h"

#include <QStackedLayout>
#include <QShortcut>
#include <QPushButton>

#include "completer.h"

#include "trace_x/trace_x.h"

FilterItemEditor::FilterItemEditor(TraceController &controller, const QList<EntityClass> &entity_list,
                                   int initial_class, FilterOperator filter_sign, QWidget *parent):
    QDialog(parent),
    ui(new Ui::FilterItemEditor),
    _controller(controller)
{
    X_CALL;

    initialize(initial_class, entity_list, filter_sign);

    setWindowTitle(tr("New Filter Item"));
}

FilterItemEditor::FilterItemEditor(TraceController &controller, const QList<EntityClass> &entity_list,
                                   const FilterItem &edited_item, FilterOperator filter_sign, QWidget *parent):
    QDialog(parent),
    ui(new Ui::FilterItemEditor),
    _controller(controller)
{
    X_CALL;

    initialize(edited_item.class_id(), entity_list, filter_sign, edited_item.string_id());

    setWindowTitle(tr("Edit Filter Item"));
}

FilterItemEditor::~FilterItemEditor()
{
    delete ui;
}

FilterItem FilterItemEditor::filter() const
{
    if(_accepted_filter.is_valid())
    {
        return _accepted_filter;
    }

    return FilterItem(_completer->current_text(), _completer->current_class());
}

FilterOperator FilterItemEditor::filter_sign() const
{
    return FilterOperator(ui->filter_sign_button->isChecked());
}

void FilterItemEditor::initialize(int initial_class, const QList<EntityClass> &entity_list, FilterOperator filter_sign, const QString &pre_id)
{
    X_CALL;

    ui->setupUi(this);

    connect(ui->filter_sign_button, &QToolButton::toggled, [this] { ui->filter_sign_button->setText(ui->filter_sign_button->isChecked() ? tr("Include") : tr("Exclude"));});

    ui->filter_sign_button->setChecked(bool(filter_sign));

    //

    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(shortcut, &QShortcut::activated, this, &FilterItemEditor::reject);

    //

    _completer = new TraceCompleter(entity_list, &_controller, ui->locator_line_edit, this, false, this);

    _completer->completer_view()->setObjectName("editor_completer");

    connect(_completer, &TraceCompleter::text_accepted, this, &FilterItemEditor::text_accepted);
    connect(_completer, &TraceCompleter::entry_accepted, this, &FilterItemEditor::entry_accepted);

    QStackedLayout *stack_layout = new QStackedLayout;

    stack_layout->setStackingMode(QStackedLayout::StackAll);

    ui->completer_frame->setLayout(stack_layout);

    _completer->set_grip_enabled(false);

    ui->locator_line_edit->setPlaceholderText(tr("Type to locate"));

    stack_layout->addWidget(_completer->completer_view());

    _completer->set_always_visible(true);

    ui->locator_line_edit->setFocus();

    if(initial_class != -1)
    {
        _completer->set_current_class(EntityClass(initial_class), pre_id);

        // hack for avoid auto-selection
        QTimer::singleShot(0, this, SLOT(clear_selection()));
    }
}

void FilterItemEditor::entry_accepted(const QModelIndex &index)
{
    X_CALL;

    _accepted_filter = index.data(FilterDataRole).value<FilterItem>();

    accept();
}

void FilterItemEditor::text_accepted()
{
    X_CALL;

    if(!_completer->current_text().isEmpty())
    {
        accept();
    }
}

void FilterItemEditor::clear_selection()
{
    ui->locator_line_edit->deselect();
}
