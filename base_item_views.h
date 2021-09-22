#ifndef BASEITEMVIEW_H
#define BASEITEMVIEW_H

#include <QAbstractItemView>
#include <QListView>
#include <QPixmap>
#include <QTableView>

class ItemView : public QAbstractItemView
{
public:
    explicit ItemView(QWidget *parent = 0);

    void set_logo(const QPixmap &pixmap);

protected:
    void paintEvent(QPaintEvent *);

private:
    QPixmap _logo_pixmap;
};

class ListView : public QListView
{
    Q_OBJECT

public:
    explicit ListView(QWidget *parent = 0);

    void set_logo(const QPixmap &pixmap);

signals:
    void activated_ex(const QModelIndex &index);

protected:
    void paintEvent(QPaintEvent *);
    void keyPressEvent(QKeyEvent *event);

private:
    QPixmap _logo_pixmap;
};

class TableView : public QTableView
{
    Q_OBJECT

public:
    explicit TableView(QWidget *parent = 0);

    void set_logo(const QPixmap &pixmap);

    void copy_to_clipboard();

    QString range_text(int top, int bottom, int left, int right) const;
    QString layout_text(const QList<int> &layout_cols, const QList<int> &layout_rows) const;
    QString selected_text() const;

signals:
    void find();
    void activated_ex(const QModelIndex &index);

protected:
    void paintEvent(QPaintEvent *);
    void keyPressEvent(QKeyEvent *event);

private:
    QPixmap _logo_pixmap;
};

#endif // BASEITEMVIEW_H
