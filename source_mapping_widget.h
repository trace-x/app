#ifndef SOURCE_MAPPING_WIDGET_H
#define SOURCE_MAPPING_WIDGET_H

#include <QFrame>

namespace Ui {
class SourceMappingWidget;
}

class PathListModel;

class SourceMappingWidget : public QFrame
{
    Q_OBJECT

public:
    explicit SourceMappingWidget(QWidget *parent = 0);
    ~SourceMappingWidget();

    QStringList path_list() const;

    QByteArray save_state() const;
    void restore_state(const QByteArray &data_array);

signals:
    void map_list_changed(const QStringList &path_list);

public slots:
    void save();
    void restore();

private:
    void add_path();
    void remove_selected();

private:
    Ui::SourceMappingWidget *ui;

    PathListModel *_path_list_model;
    QStringList _path_list;
};

#endif // SOURCE_MAPPING_WIDGET_H
