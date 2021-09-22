#ifndef COMMON_UI_TOOLS_H
#define COMMON_UI_TOOLS_H

#include <QStyledItemDelegate>
#include <QTreeView>
#include <QStandardItem>
#include <QDialog>
#include <QTableView>
#include <QListView>
#include <QLineEdit>

#include "base_item_views.h"
#include "tree_view.h"

static const int UnicodeBullet = 0x2022;

class TraceDataModel;

QString string_format_bytes(qint64 bytes);

class FancyItemDelegate : public QStyledItemDelegate
{
public:
    FancyItemDelegate(QObject *parent);

public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class TableItemDelegate : public FancyItemDelegate
{
public:
    TableItemDelegate(QObject *parent);

public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    void set_highlight_top_row(int row);

private:
    int _highlight_top_row;
};

class SearchItemDelegate : public TableItemDelegate
{
public:
    SearchItemDelegate(QObject *parent);

public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class TreeItemDelegate : public FancyItemDelegate
{
public:
    TreeItemDelegate(QObject *parent);

public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class ListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit ListModel(const QString &header, QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;

    void clear();

    QList<QString> _data;
    QString _header;
};

class MessageListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit MessageListModel(TraceDataModel *data_model, QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;

    inline TraceDataModel * data_model() { return _data_model; }

private:
    TraceDataModel *_data_model;
};

class ModelTreeView : public BaseTreeView
{
    Q_OBJECT

public:
    explicit ModelTreeView(QWidget *parent = 0);

    void setModel(QAbstractItemModel *model);

protected:
    void keyPressEvent(QKeyEvent *event);

private:
    void update_layout();
    void walk_first_column(const QModelIndex &parent);
};

class LineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit LineEdit(QWidget *parent = 0);

    void set_tip(const QString &text);

protected:
    void paintEvent(QPaintEvent *event);

private:
    QString _tip;
};

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *content_widget, QWidget *parent);
};

#endif // COMMON_UI_TOOLS_H
