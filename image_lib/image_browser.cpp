#include "image_browser.h"
#include "ui_image_browser.h"

#include <QTableView>
#include <QLineEdit>
#include <QLabel>
#include <QSpinBox>
#include <QLayout>
#include <QToolButton>
#include <QSplitter>
#include <QtMath>
#include <QImageWriter>

#include "trace_x/trace_x.h"

ImageBrowser::ImageBrowser(TraceController *controller, QWidget *parent) :
    QFrame(parent),
    ui(new Ui::ImageBrowser),
    _data_set(0),
    _current_index(0),
    _controller(controller)
{
    X_CALL;

    // TODO
    /*
     * Поиск по значению, диапазону значений
     * Сохранение картинки в контекстном меню таблицы
     * Сохранение ?относительных? позиций, обновление статистики и прочее при переключении изображений
     * Настройки точности отображения вещественных чисел, экспоненциальный вариант для больших чисел
     * Отображение более чем 4 каналов(назначение RGBA или серого)
     * Отображение каналов по-отдельности(выбор индекса)
     *
     * Правильное отображение каналов
     *
     * Неясно:
     * На чём писАть алгоритмы?
     *
     * Как выводить статистику по многоканалкам? По каждому каналу отдельно.
     *
     * ЗАГРУЗКА КАРТИНОК ?
     *
     */

    ui->setupUi(this);
    ui->state_widget->hide();
    ui->status_panel->setEnabled(false);

    //

    QStringList format_list;

    foreach (QByteArray format, QImageWriter::supportedImageFormats())
    {
        format_list.append(QString("*.%1").arg(QString(format)));
    }

    QString format_string = format_list.join(" ");

    _file_dialog.setDefaultSuffix("tif");
    _file_dialog.setNameFilter(tr("Image formats (%1)").arg(format_string));
    _file_dialog.setFileMode(QFileDialog::AnyFile);
    _file_dialog.setViewMode(QFileDialog::Detail);
    _file_dialog.setOption(QFileDialog::DontUseNativeDialog, false);

    //

    int font_h = QFontMetrics(ui->table_view->font()).height() + 5;

    ui->table_view->verticalHeader()->setDefaultSectionSize(font_h);

    ui->splitter->setSizes(QList<int>() << 100 << 100 << 100);

    _menu_bar = new QMenuBar();

    ui->menu_panel->layout()->addWidget(_menu_bar);

    _menu_bar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Maximum);

    // Image menu

    QMenu *image_menu = _menu_bar->addMenu(tr("Image"));

    QAction *action_save = image_menu->addAction(tr("Save ..."));

    action_save->setShortcut(QKeySequence::Save);

    connect(action_save, &QAction::triggered, this, &ImageBrowser::save_current);

    image_menu->addSeparator();

    QAction *action_smooth_upsampling = image_menu->addAction(tr("Smooth upsampling"));
    connect(action_smooth_upsampling, &QAction::toggled, this, &ImageBrowser::set_smooth_upsampling);

    action_smooth_upsampling->setCheckable(true);
    action_smooth_upsampling->setChecked(false);

    QAction *action_smooth_downsampling = image_menu->addAction(tr("Smooth downsampling"));
    connect(action_smooth_downsampling, &QAction::toggled, this, &ImageBrowser::set_smooth_downsampling);

    action_smooth_downsampling->setCheckable(true);
    action_smooth_downsampling->setChecked(true);

    // Window menu
    // QMenu *window_menu = _menu_bar->addMenu(tr("Window"));

    //

    connect(ui->image_view, &ImageView::mouse_move, this, &ImageBrowser::set_cursor);
    connect(ui->image_view, &ImageView::zoom_changed, this, &ImageBrowser::update_scale);

    ui->side_panel->hide();
    ui->table_view->hide();

    ui->image_view->setScene(&_image_scene);
    ui->table_view->setModel(&_table_model);

    _image_scene.addItem(&_pixel_trace_item);
    _pixel_trace_item.hide();

    connect(ui->x_spin_box, SIGNAL(valueChanged(int)), SLOT(sync_cursor()));
    connect(ui->y_spin_box, SIGNAL(valueChanged(int)), SLOT(sync_cursor()));

    connect(ui->data_table_button, &QToolButton::toggled, ui->table_view, &QTableView::setVisible);

    connect(ui->next_button, &QToolButton::clicked, this, &ImageBrowser::next_image);
    connect(ui->prev_button, &QToolButton::clicked, this, &ImageBrowser::prev_image);
}

void ImageBrowser::set_image(const Image &image)
{
    X_CALL;

    _image_scene.set_image(image);
    _table_model.set_image(image);

    if(image.data_start)
    {
        switch (image.channels)
        {
        case 1: ui->value_label->setToolTip("Value: Gray"); break;
        case 2: ui->value_label->setToolTip("Value: Gray, Alpha"); break;
        case 3: ui->value_label->setToolTip("Value: Red, Green, Blue"); break;
        case 4: ui->value_label->setToolTip("Value: Red, Green, Blue, Alpha"); break;
        }

        ui->state_widget->show();

        //

        QPoint current_cursor = QPoint(ui->x_spin_box->value(), ui->y_spin_box->value());

        ui->x_spin_box->setRange(0, image.display_image.width() - 1);
        ui->y_spin_box->setRange(0, image.display_image.height() - 1);

        ui->table_view->horizontalHeader()->setDefaultSectionSize(ui->table_view->fontMetrics().width(QString::number(0).repeated(image.max_width) + 3));

        if(!_pixel_trace_item.isVisible() || !image.display_image.rect().contains(current_cursor))
        {
            set_cursor(image.display_image.rect().center());
        }
        else
        {
            set_cursor(current_cursor);
        }

        update_scale();

        _pixel_trace_item.show();
    }
    else
    {
        ui->state_widget->hide();
        _pixel_trace_item.hide();
    }
}

void ImageBrowser::set_current_index(index_t index)
{
    X_CALL;

    _current_index = index;

    const trace_message_t *message = _data_set->at(index);

    setWindowTitle(QString("#%1 : %2").arg(index).arg(_data_set->get(index).message_text));

    ui->current_index_label->setText(QString("%1 / %2").arg(index + 1).arg(_data_set->size()));

    Image image = ::get_image(_controller->data_at(message));

    set_image(image);
}

void ImageBrowser::set_data_set(TraceDataModel *data_set)
{
    X_CALL;

    _data_set = data_set;

    ui->status_panel->setEnabled(_data_set->safe_size() > 0);

    connect(_data_set, &TraceDataModel::destroyed, this, &ImageBrowser::close);
}

void ImageBrowser::set_cursor(const QPointF &pos)
{
    X_CALL;

    if(!_image_scene.sceneRect().isNull())
    {
        QPoint pixel_pos = QPoint(int(pos.x()), int(pos.y()));

        if(pixel_pos.x() > ui->x_spin_box->maximum())
        {
            pixel_pos.rx() = ui->x_spin_box->maximum();
        }

        if(pixel_pos.x() < 0)
        {
            pixel_pos.rx() = 0;
        }

        if(pixel_pos.y() > ui->y_spin_box->maximum())
        {
            pixel_pos.ry() = ui->y_spin_box->maximum();
        }

        if(pixel_pos.y() < 0)
        {
            pixel_pos.ry() = 0;
        }

        ui->x_spin_box->blockSignals(true);
        ui->y_spin_box->blockSignals(true);

        ui->x_spin_box->setValue(pixel_pos.x());
        ui->y_spin_box->setValue(pixel_pos.y());

        ui->x_spin_box->blockSignals(false);
        ui->y_spin_box->blockSignals(false);

        _pixel_trace_item.set_position(pixel_pos);

        update_cursor();
    }
}

void ImageBrowser::update_cursor()
{
    X_CALL;

    QModelIndex table_index = _table_model.index(ui->y_spin_box->value(), ui->x_spin_box->value());

    ui->value_label->setText(table_index.data().toString());

    if(ui->table_view->isVisible())
    {
        ui->table_view->setCurrentIndex(table_index);
        ui->table_view->scrollTo(table_index, QAbstractItemView::PositionAtCenter);
    }
}

void ImageBrowser::sync_cursor()
{
    X_CALL;

    set_cursor(QPoint(ui->x_spin_box->value(), ui->y_spin_box->value()));
}

void ImageBrowser::next_image()
{
    X_CALL;

    if(_current_index + 1 < _data_set->safe_size())
    {
        set_current_index(_current_index + 1);
    }
    else
    {
        set_current_index(0);
    }
}

void ImageBrowser::prev_image()
{
    X_CALL;

    if(_current_index != 0)
    {
        set_current_index(_current_index - 1);
    }
    else
    {
        set_current_index(_data_set->safe_size() - 1);
    }
}

void ImageBrowser::save_current()
{
    X_CALL;

    _file_dialog.setAcceptMode(QFileDialog::AcceptSave);
    _file_dialog.setWindowTitle(tr("Save Trace"));

    if(_file_dialog.exec() == QDialog::Accepted)
    {
        if(!_file_dialog.selectedFiles().empty())
        {
            ::save_image(_image_scene.image(), _file_dialog.selectedFiles().first());
        }
    }
}

void ImageBrowser::set_smooth_upsampling(bool enabled)
{
    X_CALL;

    _image_scene.image_item()->set_grpahics_image_flag(GraphicsImageItem::ItemSmoothUpsampling, enabled);
}

void ImageBrowser::set_smooth_downsampling(bool enabled)
{
    X_CALL;

    _image_scene.image_item()->set_grpahics_image_flag(GraphicsImageItem::ItemSmoothDownsampling, enabled);
}

void ImageBrowser::update_scale()
{
    X_CALL;

    if(!ui->image_view->sceneRect().isNull())
    {
        ui->scale_label->setText(QString::number(qFloor(ui->image_view->current_scale() * 100)) + " %");
    }
    else
    {
        ui->scale_label->setText("-");
    }
}

ImageBrowser::~ImageBrowser()
{
    X_CALL;

    delete ui;
}

