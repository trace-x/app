#ifndef ISSUESLISTMODEL_H
#define ISSUESLISTMODEL_H

#include <QObject>

#include "common_ui_tools.h"
#include "trace_data_model.h"

//! Модель для хранения списка проблемных сообщений
//! В модель встроена логика сортировки
class IssuesListModel : public MessageListModel
{
public:
    IssuesListModel(QObject *parent = 0);
    ~IssuesListModel();

    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;

    void append(const trace_message_t *message);
    void clear();

    int issues_count(int issue_type);

    void remove_from_tail(index_t index);

private:
    TraceDataModel *_data_model; //! data model for messages, grouped by issues ("linear" tree model)

    enum
    {
        IssueFlag = 100,
    };

    struct issue_t
    {
        issue_t();

        void clear();
        void inc_shift(bool inc_start = false);
        void dec_shift(bool dec_start = false);

        size_t start; //! start index of issue in data model
        size_t end; //! end index of issue in data model
        size_t size; //! issues count

        trace_message_t title_message;
        QColor color;

        issue_t *prev;
        issue_t *next;
    };

    QVector<issue_t> _issues;
    QVector<trace_x::MessageType> _issue_types;
};

#endif // ISSUESLISTMODEL_H
