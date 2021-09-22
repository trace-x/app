#include "completer.h"

#include <QApplication>
#include <QEvent>
#include <QFocusEvent>
#include <QHeaderView>

#include <QShortcut>
#include <QAction>

#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QPainter>
#include <QScrollBar>

#include <QSizeGrip>

#include "trace_x/trace_x.h"

#include "trace_controller.h"
#include "common_ui_tools.h"
#include "tree_view.h"

class HtmlDelegate : public QStyledItemDelegate
{
protected:
    void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
    QSize sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const;
};

void HtmlDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;

    initStyleOption(&opt, index);

    const QWidget *widget = option.widget;

    QStyle *style = widget ? widget->style() : QApplication::style();

    QTextDocument doc;
    doc.setHtml(opt.text);

    /// Painting item without text
    opt.text = QString();

    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

    QAbstractTextDocumentLayout::PaintContext ctx;

    // Highlighting text if item is selected
    if (opt.state & QStyle::State_Selected)
        ctx.palette.setColor(QPalette::Text, opt.palette.color(QPalette::Active, QPalette::HighlightedText));

    QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt);
    painter->save();
    painter->translate(textRect.topLeft());
    painter->setClipRect(textRect.translated(-textRect.topLeft()));
    doc.documentLayout()->draw(painter, ctx);
    painter->restore();
}

QSize HtmlDelegate::sizeHint(const QStyleOptionViewItem &option_, const QModelIndex &index) const
{
    QStyleOptionViewItem option = option_;

    initStyleOption(&option, index);

    QTextDocument doc;
    doc.setHtml(option.text);
    doc.setTextWidth(option.rect.width());

    return QSize(doc.idealWidth(), doc.size().height());
}

static void set_line_edit_text_format(QLineEdit* lineEdit, const QList<QTextLayout::FormatRange>& formats)
{
    if(!lineEdit)
        return;

    QList<QInputMethodEvent::Attribute> attributes;
    foreach(const QTextLayout::FormatRange& fr, formats)
    {
        QInputMethodEvent::AttributeType type = QInputMethodEvent::TextFormat;
        int start = fr.start - lineEdit->cursorPosition();
        int length = fr.length;
        QVariant value = fr.format;
        attributes.append(QInputMethodEvent::Attribute(type, start, length, value));
    }
    QInputMethodEvent event(QString(), attributes);
    QCoreApplication::sendEvent(lineEdit, &event);
}

class CompletionList : public BaseTreeView
{
public:
    CompletionList(QWidget *parent = 0) :
        BaseTreeView(parent),
        _is_empty(false),
        _grip_corner(Qt::BottomRightCorner)
    {
        setItemDelegate(new TreeItemDelegate(this));

        // for _size_grip support
        setWindowFlags(Qt::SubWindow);

        _size_grip = new QSizeGrip(this);

        // fix place for _size_grip
        setCornerWidget(new QWidget);

        setIndentation(0);
        setRootIsDecorated(false);
        setUniformRowHeights(true);
        header()->hide();

        setDragEnabled(true);
        setDragDropMode(QAbstractItemView::DragOnly);

        setSelectionMode(QAbstractItemView::ExtendedSelection);
    }

    void set_empty(bool is_empty)
    {
        _is_empty = is_empty;
    }

    void next()
    {
        int index = currentIndex().row() + 1;

        if(index >= model()->rowCount(QModelIndex()))
        {
            // wrap
            index = 0;
        }

        setCurrentIndex(model()->index(index, 0));
    }

    void previous()
    {
        int index = currentIndex().row() - 1;

        if(index < 0)
        {
            // wrap
            index = model()->rowCount(QModelIndex()) - 1;
        }

        setCurrentIndex(model()->index(index, 0));
    }

    void resizeEvent(QResizeEvent *event)
    {
        if(event)
        {
            QTreeView::resizeEvent(event);
        }

        QRect geometry(0, 0, 16, 16);

        if(_grip_corner == Qt::BottomRightCorner)
        {
            geometry.moveBottomRight(this->rect().adjusted(0, 0, -2, -2).bottomRight());
        }
        else if(_grip_corner == Qt::TopRightCorner)
        {
            geometry.moveTopRight(this->rect().adjusted(-2, -2, 0, 0).topRight());
        }

        _size_grip->setGeometry(geometry);

        update();

        _size_grip->stackUnder(verticalScrollBar()->parentWidget());
    }

    void paintEvent(QPaintEvent *event)
    {
        QTreeView::paintEvent(event);

        if(model() && _is_empty)
        {
            QPainter painter;
            painter.begin(viewport());
            painter.save();

            painter.drawText(viewport()->geometry(), Qt::AlignCenter, tr("Items not found"));

            painter.restore();
            painter.end();
        }

        //TODO

//        if(_grip_corner == Qt::TopRightCorner)
//        {
//            QPainter painter;
//            painter.begin(viewport());
//            painter.save();

//            painter.drawPixmap(_size_grip->);

//            painter.restore();
//            painter.end();
//        }
    }

    QSize _preferredSize;
    QSizeGrip *_size_grip;
    bool _is_empty;
    Qt::Corner _grip_corner;
};

Completer::Completer(LineEdit *line_edit, QWidget *completer_parent, bool manual_mode, QObject *parent):
    QObject(parent),
    _line_edit(line_edit),
    _always_visible(false),
    _manual_mode(manual_mode),
    _close_action(0),
    _clear_action(0)
{
    X_CALL;

    connect(this, &Completer::completer_shows, this, &Completer::update_action, Qt::QueuedConnection);

    if(!_manual_mode)
    {
        _close_action = line_edit->addAction(QIcon(":/icons/down_arrow_dark"), QLineEdit::TrailingPosition);
        _close_action->setEnabled(false);
    }

    connect(line_edit, &QLineEdit::textChanged, this, &Completer::process_text);

    _line_edit->setFocusPolicy(Qt::ClickFocus);
    _line_edit->installEventFilter(this);

    _completer_view = new CompletionList(completer_parent);

    ///  _completer_view->setItemDelegate(new HtmlDelegate);

    _completer_view->setVisible(false);
    _completer_view->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(_completer_view, &QAbstractItemView::activated, this, &Completer::accept);

    _completer_view->resizeColumnToContents(0);
    _completer_view->installEventFilter(this);

    //

    if(!_manual_mode)
    {
        QShortcut *shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_K), completer_parent);

        connect(shortcut, &QShortcut::activated, this, &Completer::show_completer);
    }
}

BaseTreeView *Completer::completer_view() const
{
    return _completer_view;
}

void Completer::set_grip_enabled(bool enable)
{
    if(!enable)
    {
        _completer_view->setCornerWidget(0);
        _completer_view->_size_grip->hide();
    }
}

void Completer::set_grip_corner(Qt::Corner corner)
{
    X_CALL;

    _completer_view->_grip_corner = corner;
    _completer_view->resizeEvent(0);
}

void Completer::set_always_visible(bool enable)
{
    _always_visible = enable;

    if(enable)
    {
        delete _close_action;

        _close_action = 0;
    }
}

bool Completer::eventFilter(QObject *object, QEvent *event)
{
    if(object == _line_edit)
    {
        if(event->type() == QEvent::KeyPress)
        {
            QKeyEvent *key_event = static_cast<QKeyEvent*>(event);

            QKeySequence key(key_event->modifiers() | key_event->key());

            switch(key_event->key())
            {
            case Qt::Key_Up:
            case Qt::Key_Down:
            case Qt::Key_PageUp:
            case Qt::Key_PageDown:
            {
                if(!_manual_mode)
                {
                    show_completer();
                }

                if(key_event->key() == Qt::Key_Up)
                {
                    if((_completer_view->currentIndex().row() == 0) && _completer_view->currentIndex().parent().isValid() &&
                            (_completer_view->currentIndex().parent().row() == 0))
                    {
                        _completer_view->scrollTo(_completer_view->model()->index(0, 0));
                    }
                }

                QApplication::sendEvent(_completer_view, event);

                return true;
            }
            case Qt::Key_Enter:
            case Qt::Key_Return:
            {
                accept();

                emit text_accepted();

                if(_manual_mode)
                {
                    hide_completer(false);
                }

                //emit _completer_view->activated(_completer_view->currentIndex());
                // scheduleAcceptCurrentEntry();
                return true;
            }
            case Qt::Key_Escape:
                if(_completer_view->isVisible())
                {
                    hide_completer(false);
                    key_event->accept();
                    return true;
                }
                else
                {
                    return false;
                }
            case Qt::Key_Tab:
                _completer_view->next();
                return true;
            case Qt::Key_Backtab:
                _completer_view->previous();
                return true;
            case Qt::Key_Alt:
                if (key_event->modifiers() == Qt::AltModifier)
                {
                    return true;
                }
                break;
            default:
                break;
            }

            if(key_event->matches(QKeySequence::Find))
            {
                if(_completer_view->isVisible() && _completer_view->currentIndex().isValid())
                {
                    emit _completer_view->find();
                }
                else
                {
                    emit find();
                }

                return true;
            }

            if(!_manual_mode)
            {
                show_completer();
            }
            else if(key == QKeySequence(Qt::CTRL + Qt::Key_Space))
            {
                show_completer();

                return true;
            }
        }
        else if(event->type() == QEvent::FocusOut)
        {
            if(!_completer_view->hasFocus())
            {
                hide_completer();
            }
        }
        else if((event->type() == QEvent::MouseButtonPress) && !_manual_mode)
        {
            show_completer();
        }
    }
    else
    {
        if(event->type() == QEvent::FocusOut)
        {
            if(!_line_edit->hasFocus())
            {
                hide_completer();
            }
        }
        else if(event->type() == QEvent::KeyPress)
        {
            QKeyEvent *key_event = static_cast<QKeyEvent *>(event);

            switch(key_event->key())
            {
            case Qt::Key_Escape:
                if(_completer_view->isVisible())
                {
                    hide_completer(false);
                    key_event->accept();
                    return true;
                }
            }
        }
    }

    return QObject::eventFilter(object, event);
}

void Completer::process_text(const QString &text)
{
    X_CALL;

    if(!text.isEmpty())
    {
        if(!_clear_action && !_manual_mode)
        {
            _clear_action = _line_edit->addAction(QIcon(":/icons/left_arrow"), QLineEdit::TrailingPosition);

            connect(_clear_action, &QAction::triggered, _line_edit, &QLineEdit::clear);
        }
    }
    else
    {
        delete _clear_action;
        _clear_action = 0;
    }
}

void Completer::show_completer()
{
    X_CALL;

    emit completer_shows();

    _completer_view->show();

    _line_edit->setFocus();
}

void Completer::update_action()
{
    if(_close_action)
    {
        _close_action->setIcon(QIcon(":/icons/up_arrow_dark"));
        _close_action->setEnabled(true);

        connect(_close_action, &QAction::triggered, this, &Completer::hide_completer);
    }
}

void Completer::hide_completer(bool clear_focus)
{
    X_CALL;

    if(!_always_visible)
    {
        if(_close_action)
        {
            _close_action->setIcon(QIcon(":/icons/down_arrow_dark"));
            _close_action->disconnect(this);
            _close_action->setEnabled(false);
        }

        _completer_view->hide();

        if(clear_focus)
        {
            _line_edit->clearFocus();
        }
    }
}

////////////////

TraceCompleter::TraceCompleter(const QList<EntityClass> &class_list, TraceController *trace_controller, LineEdit *line_edit, QWidget *completer_parent, bool manual_mode, QObject *parent):
    Completer(line_edit, completer_parent, manual_mode, parent),
    _controller(trace_controller),
    _help_mode(true),
    _message_search_model(tr("message text")),
    _current_class(-1)
{
    X_CALL;

    _completer_view->setModel(&_view_model);

    connect(&_view_model, &QAbstractItemModel::layoutChanged, _completer_view, &QTreeView::expandAll);
    connect(&_view_model, &QAbstractItemModel::layoutChanged, this, &TraceCompleter::resize_columns);

    //

    ColorGenerator color_gen;
    color_gen.next();

    foreach(const EntityClass entity_class, class_list)
    {
        switch (entity_class)
        {
        case ProcessNameEntity:  _entry_set.append(Entry("pn", tr("process name"), _controller->process_names())); break;
        case ProcessIdEntity:    _entry_set.append(Entry("p",  tr("process id"), _controller->process_models(), ProcessModel::PidRole)); break;
        case ProcessUserEntity:  _entry_set.append(Entry("pu", tr("process user"), _controller->process_users())); break;
        case ModuleNameEntity:   _entry_set.append(Entry("m",  tr("module name"), _controller->modules())); break;
        case ThreadIdEntity:     _entry_set.append(Entry("th", tr("thread id"), _controller->threads(), Qt::DisplayRole, false)); break;
        case ContextIdEntity:    _entry_set.append(Entry("o",  tr("object id"), _controller->contexts(), Qt::DisplayRole, false)); break;
        case ClassNameEntity:    _entry_set.append(Entry("c",  tr("class"), _controller->classes())); break;
        case FunctionNameEntity: _entry_set.append(Entry("f",  tr("function"), _controller->functions())); break;
        case SourceNameEntity:   _entry_set.append(Entry("s",  tr("source"), _controller->sources(), Qt::ToolTipRole)); break;
        case MessageTypeEntity:  _entry_set.append(Entry("t",  tr("message type"), _controller->full_message_types(), Qt::DisplayRole, false)); break;
        case LabelNameEntity:    _entry_set.append(Entry("v",  tr("variable name"), _controller->variables())); break;
        case MessageTextEntity:  _entry_set.append(Entry("me", tr("message"), 0, Qt::DisplayRole, false)); break;
        }

        Entry entry = _entry_set.entries.last();

        _prefix_hash[entity_class] = entry.prefix;

        QStandardItem *item = new QStandardItem(entry.prefix + QString("\t") + entry.name);

        item->setDragEnabled(false);
        item->setData(entry.prefix);
        item->setData(QColor(color_gen.next().bg_color), Qt::DecorationRole);

        _help_model.appendRow(item);
    }

    //    foreach (const prefix_t &prefix, prefixes)
    //    {
    //        _help_model.appendRow(new QStandardItem("<b>User:</b> %s. <span style=\"float:right;\">#1</span><br/>)"));
    //    }

    connect(&_search_watcher, &QFutureWatcherBase::finished, this, &TraceCompleter::find_message_finished);

    switch_to_help();
}

void TraceCompleter::set_default_filters(const QSet<int> &default_filters)
{
    _default_filter_set = default_filters;

    process_text("");
}

void TraceCompleter::set_current_class(EntityClass class_id, const QString &pre_id)
{
    X_CALL;

    _line_edit->deselect();
    _line_edit->clear();

    _current_class = class_id;

    if((_current_class != -1) && !_default_filter_set.contains(_current_class))
    {
        _line_edit->setText(_prefix_hash[int(class_id)] + " ");
        _line_edit->setCursorPosition(_line_edit->text().length() + 1);
    }

    if(!pre_id.isEmpty())
    {
        _line_edit->blockSignals(true);

        _current_text = pre_id;

        _line_edit->insert(pre_id);

        if(_manual_mode)
        {
            process_text(_line_edit->text());
        }

        _line_edit->blockSignals(false);
    }
}

void TraceCompleter::clear()
{
    X_CALL;

    _entry_search_model.clear();
    _message_search_model.clear();
}

QString TraceCompleter::current_text() const
{
    return _current_text;
}

int TraceCompleter::current_class() const
{
    return _current_class;
}

FilterItem TraceCompleter::current_filter() const
{
    return FilterItem(_current_text, _current_class);
}

QModelIndex TraceCompleter::current_index() const
{
    QModelIndex index;

    if(_completer_view->isVisible())
    {
        index = _completer_view->currentIndex();
    }

    return index;
}

const QSet<int> &TraceCompleter::default_filters() const
{
    return _default_filter_set;
}

void TraceCompleter::switch_to_help()
{
    X_CALL;

    emit _view_model.layoutAboutToBeChanged();

    _view_model.setSourceModel(&_help_model);

    emit _view_model.layoutChanged();

    _help_mode = true;
}

void TraceCompleter::process_text(const QString &text)
{
    X_CALL;

    Completer::process_text(text);

    _completer_view->set_empty(false);

    QString prefix;

    if(text.isEmpty() && !_help_mode)
    {
        _current_class = -1;
        _current_text = text;

        switch_to_help();
    }
    else
    {
        _help_mode = false;

        // parse string & update model
        int first_space = text.indexOf(' ', 0, Qt::CaseInsensitive);

        _current_class = -1;
        _current_text = text;

        if(first_space != -1)
        {
            prefix = text.left(first_space);

            _current_class = _prefix_hash.key(prefix, -1);

            _current_text = text.mid(first_space + 1);
        }

        if(_current_class == -1)
        {
            _current_text = text;
        }

        X_VALUE("locate_params", X_T(_current_text, prefix));

        find_by_class(prefix, _current_text);

        emit _view_model.layoutAboutToBeChanged();

        _view_model.setSourceModel(&_search_model);

        emit _view_model.layoutChanged();

      //  _completer_view->setCurrentIndex(_view_model.index(0, 0).child(0, 0));

        //

        QTextCharFormat format;

        format.setFontWeight(QFont::Bold);
        QTextLayout::FormatRange prefix_format;
        prefix_format.start = 0;
        prefix_format.length = (_current_class != -1) ? prefix.length() : 0;
        prefix_format.format = format;

        set_line_edit_text_format(_line_edit, QList<QTextLayout::FormatRange>() << prefix_format);
    }

    if(_manual_mode)
    {
        QString class_name = _controller->filter_class_name(_current_class);

        if(prefix.isEmpty() || (_current_class == -1))
        {
            class_name = _controller->filter_class_name(_default_filter_set.toList().first());
        }

        _line_edit->set_tip(": " + class_name);
    }
}

void TraceCompleter::accept()
{
    X_CALL;

    if(!_completer_view->isVisible()) return;

    const QModelIndex index = _completer_view->currentIndex();

    if(!index.isValid()) return;

    if(_help_mode)
    {
        _line_edit->setText(index.data(Qt::UserRole + 1).toString() + " ");
        _line_edit->setFocus();

        _help_mode = false;
    }
    else
    {
//        if(_manual_mode)
//        {
//            _line_edit->insert(index.data().toString());
//        }

        // ?
        emit activated();

       //
       // if(!_current_text.isEmpty())
        {
            emit entry_accepted(index);
        }
    }
}

void TraceCompleter::find_by_class(const QString &prefix, const QString &pattern)
{
    X_CALL;

    _search_break_flag = true;

    _search_watcher.waitForFinished();

    if(prefix == "me")
    {
        _search_model.setSourceModel(&_message_search_model);

        _search_watcher.setFuture(QtConcurrent::run(this, &TraceCompleter::find_message, QRegExp(pattern, Qt::CaseInsensitive, QRegExp::Wildcard)));
    }
    else
    {
        _entry_search_model.clear();

        _search_model.setSourceModel(&_entry_search_model);

        QList<QStandardItem*> result;

        find_in_entries(result, prefix, pattern, _entry_set);

        if(!result.empty())
        {
            _entry_search_model.invisibleRootItem()->appendRows(result);
        }

        if(result.empty() || (result.first()->rowCount() == 0))
        {
            _completer_view->set_empty(true);
        }
    }
}

void TraceCompleter::resize_columns()
{
    _completer_view->resizeColumnToContents(0);
}

void TraceCompleter::find_message(const QRegExp &regexp)
{
    X_CALL;

    // Процедура поиска сообщения в трассе
    // Запускается в отдельном потоке

    _message_search_model._data = QList<QString>();

    _search_break_flag = false;

    //TODO: parallel

    QSet<QString> message_set;

    for(size_t i = 0; i < _controller->trace_model().size(); ++i)
    {
        const trace_message_t *message = _controller->trace_model().at(i);

        if(message->type > trace_x::MESSAGE_RETURN)
        {
            if(message->message_text.contains(regexp))
            {
                if(!message_set.contains(message->message_text))
                {
                    _message_search_model._data.append(QString(message->message_text).replace('\n', ' '));

                    message_set.insert(message->message_text);
                }
            }
        }

        if(_search_break_flag)
        {
            break;
        }
    }

    if(_search_break_flag)
    {
        _message_search_model._data = QList<QString>();
    }
}

void TraceCompleter::find_message_finished()
{
    X_CALL;

    if(_message_search_model._data.empty())
    {
        _completer_view->set_empty(true);
    }

    emit _message_search_model.layoutChanged();

    _search_break_flag = false;
}
