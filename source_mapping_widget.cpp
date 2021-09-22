#include "source_mapping_widget.h"
#include "ui_source_mapping_widget.h"

#include "trace_x/trace_x.h"

#include <QDebug>
#include <QFileDialog>
#include <QPushButton>

class PathListModel : public QAbstractListModel
{
public:
    explicit PathListModel(QObject *parent = 0) : QAbstractListModel(parent) { }

    int rowCount(const QModelIndex &parent = QModelIndex()) const
    {
        return list.count();
    }

    QVariant data(const QModelIndex &index, int role) const
    {
        if(index.row() >= list.size())
        {
            return QVariant();
        }

        if(role == Qt::DisplayRole)
        {
            return list.at(index.row());
        }

        return QVariant();
    }

    QStringList list;
};

SourceMappingWidget::SourceMappingWidget(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::SourceMappingWidget)
{
    ui->setupUi(this);

    ui->path_list_view->set_logo(QPixmap(":/icons/pathes"));
    ui->path_list_view->setModel(_path_list_model = new PathListModel);

    connect(ui->add_path_button, &QPushButton::clicked, this, &SourceMappingWidget::add_path);
    connect(ui->remove_button, &QPushButton::clicked, this, &SourceMappingWidget::remove_selected);
}

SourceMappingWidget::~SourceMappingWidget()
{
    delete ui;
}

QStringList SourceMappingWidget::path_list() const
{
    return _path_list_model->list;
}

QByteArray SourceMappingWidget::save_state() const
{
    X_CALL;

    QByteArray state;

    QDataStream stream(&state, QIODevice::WriteOnly);

    stream << _path_list;

    return state;
}

void SourceMappingWidget::restore_state(const QByteArray &data_array)
{
    X_CALL;

    if(!data_array.isEmpty())
    {
        QDataStream stream(data_array);

        stream >> _path_list;

        restore();
    }
}

void SourceMappingWidget::save()
{
    X_CALL;

    _path_list = _path_list_model->list;

    emit map_list_changed(_path_list);
}

void SourceMappingWidget::restore()
{
    X_CALL;

    emit _path_list_model->layoutAboutToBeChanged();

    _path_list_model->list = _path_list;

    emit _path_list_model->layoutChanged();

    emit map_list_changed(_path_list);
}

void SourceMappingWidget::add_path()
{
    X_CALL;

    QString path = QFileDialog::getExistingDirectory();

    if(!path.isEmpty())
    {
        if(!_path_list_model->list.contains(path))
        {
            _path_list_model->list.append(path);

            emit _path_list_model->layoutChanged();
        }
    }
}

void SourceMappingWidget::remove_selected()
{
    X_CALL;

    int current_row = ui->path_list_view->currentIndex().row();

    if(current_row != -1)
    {
        _path_list_model->removeRow(current_row);
        _path_list_model->list.removeAt(current_row);

        emit _path_list_model->layoutChanged();

        ui->path_list_view->updateGeometry();
    }
}
