#ifndef PROFILE_MODEL_H
#define PROFILE_MODEL_H

#include <QObject>
#include <QAbstractTableModel>
#include <QTableView>
#include <QToolButton>

#include "trace_x/impl/types.h"
#include "trace_data_model.h"
#include "common_ui_tools.h"

struct ProfileModelPrivate;

class ProfileModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum TraceColumns
    {
        Process = 0,
        Thread,
        Module,
        Function,
        Calls,
        MinTime,
        MaxTime,
        AvgTime,
        TotalTime,
        ProcessRate,
        ThreadRate,

        LastColumn
    };

    ProfileModel(TraceController *controller, TraceDataModel *trace_data, QObject *parent = 0);
    ~ProfileModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    QString column_size_hint(int column) const;

    void clear();

    void set_inclusive_mode(bool is_inclusive);

protected:
    struct ColumnData
    {
        ColumnData() {}
        ColumnData(const QString &text_, const QString tool_tip_, const QString &width_):
            text(text_), tool_tip(tool_tip_), width(width_) {}

        QString text;
        QString tool_tip;
        QString width;
    };

private:
    ProfileModelPrivate *_p;

    TraceController *_controller;

    QVector<ColumnData> _header_model;

    bool _exclusive_mode;
};

class ProfilerTable : public TableView
{
public:
    ProfilerTable(TraceController *controller, TraceDataModel *trace_data, QWidget *parent = 0);

    ProfileModel *model();

    ~ProfilerTable();

protected:
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);

private:
    void update_layout();

private:
    ProfileModel *_model;
    QToolButton *_menu_button;
};

#endif // PROFILER_H
