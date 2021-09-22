#include "panel_container.h"
#include "ui_panel_container.h"

#include <QLayout>
#include <QLabel>
#include <QToolButton>
#include <QSplitter>
#include <QEvent>
#include <QStackedLayout>
#include <QTimer>
#include <QKeyEvent>

#include "settings.h"
#include "trace_x/trace_x.h"

PanelContainer::PanelContainer(QWidget *content_widget, QWidget *parent) :
    QFrame(parent),
    ui(new Ui::PanelContainer),
    _content_widget(content_widget)
{
    X_CALL;

    X_INFO("content_widget title: ", content_widget->windowTitle());

    ui->setupUi(this);

    _content_widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);

    ui->title_label->setText(content_widget->windowTitle());
    ui->content_layout->addWidget(content_widget);

    content_widget->installEventFilter(this);

    ui->header_widget->installEventFilter(this);

    QStackedLayout *stack_layout = new QStackedLayout();
    stack_layout->setContentsMargins(QMargins());

    stack_layout->setStackingMode(QStackedLayout::StackAll);

    ui->header_widget->setLayout(stack_layout);

    stack_layout->addWidget(ui->tool_bar);
    stack_layout->addWidget(ui->title_label);

    ui->tool_bar->hide();

    connect(ui->detach_button, &QToolButton::clicked, this, &PanelContainer::detach);
}

PanelContainer::~PanelContainer()
{
    X_CALL;

    delete ui;
}

QByteArray PanelContainer::save_state() const
{
    X_CALL;

    QByteArray state_array;

    QDataStream stream(&state_array, QIODevice::WriteOnly);

    bool is_detached = _content_widget->parentWidget() != this;

    stream << is_detached;
    stream << _content_widget->saveGeometry();

    return state_array;
}

void PanelContainer::restore_state(const QByteArray &state_data)
{
    X_CALL;

    if(!state_data.isEmpty())
    {
        DataStream stream(state_data);

        bool is_detached = false;

        stream >> is_detached;

        if(is_detached)
        {
            _state = stream.read_next_array();

            QTimer::singleShot(0, this, SLOT(detach()));
        }
    }
}

bool PanelContainer::event(QEvent *event)
{
    if(event->type() == QEvent::HideToParent)
    {
        emit visibility_changed(false);
    }
    else if(event->type() == QEvent::ShowToParent)
    {
        emit visibility_changed(true);
    }

    return QFrame::event(event);
}

bool PanelContainer::eventFilter(QObject *object, QEvent *event)
{
    if(object == _content_widget)
    {
        if(event->type() == QEvent::WindowTitleChange)
        {
            ui->title_label->setText(_content_widget->windowTitle());
        }
        else if((event->type() == QEvent::Close) ||
                ((event->type() == QEvent::KeyPress) && static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape))
        {
            _state = _content_widget->saveGeometry();

            ui->content_layout->addWidget(_content_widget);

            // QEvent::Close become BEFORE widget to be hidden

            QTimer::singleShot(0, _content_widget, SLOT(show()));

            this->show();

            emit dock_changed(true);
        }
        else if(event->type() == QEvent::MouseButtonDblClick)
        {
            emit double_clicked();
        }
    }
    else if(object == ui->header_widget)
    {
        if(event->type() == QEvent::Enter)
        {
            ui->tool_bar->show();
        }
        else if(event->type() == QEvent::Leave)
        {
            ui->tool_bar->hide();
        }
    }

    return false;
}

void PanelContainer::detach()
{
    X_CALL;

    _content_widget->setParent(this->window(), Qt::Dialog);
    _content_widget->setWindowFlags(Qt::Dialog);

    QRect geometry(0, 0, qMin(_content_widget->width() * 2, this->window()->width() / 2), this->window()->height() / 2);

    geometry.moveCenter(this->window()->geometry().center());

    _content_widget->setGeometry(geometry);

    if(!_state.isEmpty())
    {
        _content_widget->restoreGeometry(_state);
    }

    _content_widget->show();

    this->hide();

    emit dock_changed(false);
}
