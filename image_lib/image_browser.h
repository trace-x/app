#ifndef IMAGE_BROWSER_H
#define IMAGE_BROWSER_H

#include <QFrame>
#include <QMenuBar>
#include <QFileDialog>

#include "image_scene.h"
#include "image_table_model.h"
#include "pixel_trace_graphics_item.h"
#include "trace_data_model.h"
#include "trace_controller.h"

namespace Ui {
class ImageBrowser;
}

class ImageBrowser : public QFrame
{
    Q_OBJECT

public:
    explicit ImageBrowser(TraceController *controller, QWidget *parent = 0);
    ~ImageBrowser();

    void set_image(const Image &image);
    void set_current_index(index_t index);
    void set_data_set(TraceDataModel *data_set);

    void set_cursor(const QPointF &pos);
    void update_cursor();

    void update_scale();

private slots:
    void sync_cursor();
    void next_image();
    void prev_image();
    void save_current();

    void set_smooth_upsampling(bool enabled);
    void set_smooth_downsampling(bool enabled);

private:
    Ui::ImageBrowser *ui;
    QMenuBar *_menu_bar;
    QFileDialog _file_dialog;

    TraceDataModel *_data_set;
    TraceController *_controller;

    index_t _current_index;

    ImageScene _image_scene;
    ImageTableModel _table_model;
    PixelTraceGraphicsItem _pixel_trace_item;
};

#endif // IMAGE_BROWSER_H
