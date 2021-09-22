#include "issues_list_model.h"

#include "trace_x/trace_x.h"

#include "trace_model.h"

IssuesListModel::IssuesListModel(QObject *parent):
    MessageListModel(_data_model = new TraceDataModel, parent)
{
    X_CALL;

    _issues.resize(trace_x::_MESSAGE_END_);

    //TODO call-ret detections, bad pointers

    _issue_types << trace_x::MESSAGE_ASSERT <<
                    trace_x::MESSAGE_ERROR <<
                    trace_x::MESSAGE_EXCEPTION <<
                    trace_x::MESSAGE_WARNING <<
                    trace_x::MESSAGE_CRASH;

    for(int i = 0; i < _issue_types.size(); ++i)
    {
        int type = int(_issue_types[i]);

        _issues[type].title_message.type = type;
        _issues[type].title_message.message_text = MessageTypeItem::message_type_name(type);
        _issues[type].color = MessageTypeItem::message_type_color(type);

        if(i > 0)
        {
            _issues[type].prev = &_issues[int(_issue_types[i - 1])];
        }

        if(i < _issue_types.size() - 1)
        {
            _issues[type].next = &_issues[int(_issue_types[i + 1])];
        }
    }
}

IssuesListModel::~IssuesListModel()
{
    delete _data_model;
}

QVariant IssuesListModel::data(const QModelIndex &index, int role) const
{    
    if(index.row() >= int(_data_model->safe_size()))
    {
        return QVariant();
    }

    if(role == Qt::DisplayRole)
    {
        _data_model->lock();

        QString message = _data_model->at(index.row())->message_text;

        _data_model->unlock();

        return message.replace('\n', ' ');
    }

    if(role == Qt::DecorationRole)
    {
        _data_model->lock();

        trace_message_t message = *_data_model->at(index.row());

        _data_model->unlock();

        if(message.flags == IssueFlag) return _issues[message.type].color;
    }

    if(role == Qt::ToolTipRole)
    {
        _data_model->lock();

        trace_message_t message = *_data_model->at(index.row());

        _data_model->unlock();

        if(message.flags == IssueFlag) return QString("%1 %2 issues").arg(_issues[message.type].size).arg(message.message_text);
    }

    return MessageListModel::data(index, role);
}

Qt::ItemFlags IssuesListModel::flags(const QModelIndex &index) const
{
    trace_message_t message;

    {
        QMutexLocker lock(_data_model->mutex());

        if(index.row() >= int(_data_model->size()))
        {
            return Qt::NoItemFlags;
        }

        message = *_data_model->at(index.row());
    }

    if(message.flags == IssueFlag) return Qt::NoItemFlags;

    return MessageListModel::flags(index);
}

void IssuesListModel::append(const trace_message_t *message)
{
    X_CALL;

    issue_t &issue = _issues[message->type];

    // check if message is issue
    if(!issue.title_message.message_text.isEmpty())
    {
        issue.size++;

        if(issue.size == 1)
        {
            // append title fake-message for new issue
            _data_model->insert(&issue.title_message, issue.end);

     //       qDebug() << "append HEADER" << issue.title_message.message_text << issue.end;

            issue.inc_shift();

            issue.start = issue.end;
        }

        _data_model->insert(message, issue.end);

    //    qDebug() << "append MESSAGE" << issue.title_message.message_text << issue.end;

        issue.inc_shift();
    }
}

void IssuesListModel::clear()
{
    X_CALL;

    emit layoutAboutToBeChanged();

    _data_model->clear();

    for(int i = 0; i < _issues.size(); ++i)
    {
        _issues[i].clear();
    }

    emit layoutChanged();
}

int IssuesListModel::issues_count(int issue_type)
{
    return _issues[issue_type].size;
}

void IssuesListModel::remove_from_tail(index_t index)
{
    X_CALL;

    foreach (trace_x::MessageType type, _issue_types)
    {
        issue_t &issue = _issues[type];

        if(issue.size && (_data_model->at(issue.start)->index == index))
        {
            issue.size--;

           // qDebug() << "remove" << _data_model->trace_list().at(issue.start)->message_text << issue.title_message.message_text;

            _data_model->trace_list().removeAt(issue.start);

            issue.dec_shift();

            if(issue.size == 0)
            {
             //   qDebug() << "remove HEADER" << _data_model->trace_list().at(issue.start - 1)->message_text;

                _data_model->trace_list().removeAt(issue.start - 1);

                issue.dec_shift();

                issue.size = 0;
                issue.end = 0;
                issue.start = 0;
            }

            return;
        }
    }
}

IssuesListModel::issue_t::issue_t():
    start(0),
    end(0),
    size(0),
    next(0),
    prev(0)
{
    title_message.flags = IssueFlag;
}

void IssuesListModel::issue_t::clear()
{
    size = 0;
    end = 0;
    start = 0;
}

void IssuesListModel::issue_t::inc_shift(bool inc_start)
{
    if(inc_start)
    {
        start++;
    }

    end++;

    if(next)
    {
        next->inc_shift(true);
    }
}

void IssuesListModel::issue_t::dec_shift(bool dec_start)
{
    if(dec_start && (start > 0))
    {
        start--;
    }

    if(end > 0)
    {
        end--;
    }

    if(next)
    {
        next->dec_shift(true);
    }
}
