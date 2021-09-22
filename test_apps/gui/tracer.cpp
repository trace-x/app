#include "tracer.h"
#include "ui_tracer.h"

#include <iostream>
#include <sstream>
#include <map>
#include <limits>

#include <boost/bind.hpp>
#include <boost/filesystem.hpp>

#include <QThread>
#include <QElapsedTimer>
#include <QDateTime>
#include <QMap>
#include <QList>
#include <QDebug>
#include <QPainter>
#include <QFile>
#include <QDir>

#define TRACE_X_MODULE_NAME "tracer_module"

//#define TRACE_X_PROFILE

#include "trace_x/trace_x.h"

#include "tracebutton.h"

//#include "test_apps/lib1/lib1.h"
//#include "test_apps/lib2/lib1.h"
#include "test_apps/lib0/lib0.h"

#include "CImg.h"

#ifdef WITH_OPENCV
  #include "trace_x/tools/cv_image_trace_x.h"
#endif

#ifdef WITH_UNILOG
//  #include <unilog/unilog.h>
#endif

namespace cimg_library
{

template<class T>
inline trace_x::image_stream &operator<<(trace_x::image_stream &os, const CImg<T> &image)
{
    os.data.assign_numeric_matrix(image.width(), image.height(), image.spectrum(),
                                  trace_x::type_of<T>::type, image.data());

    os.data.set_axes({1, 2, 3, 0});

    switch (image.spectrum())
    {
    case 2: os.data.set_band_map({ trace_x::BAND_GRAY, trace_x::BAND_ALPHA }); break;
    case 3: os.data.set_band_map({ trace_x::BAND_RED, trace_x::BAND_GREEN, trace_x::BAND_BLUE }); break;
    case 4: os.data.set_band_map({ trace_x::BAND_RED, trace_x::BAND_GREEN, trace_x::BAND_BLUE, trace_x::BAND_ALPHA }); break;
    }

    return os;
}

}

template<class T>
struct CImgX
{
    cimg_library::CImg<T> image;
    std::vector<uint8_t> axes;
    std::vector<uint8_t> band_map;
};

template<class T>
inline trace_x::image_stream &operator<<(trace_x::image_stream &os, const CImgX<T> &image)
{
    os << image.image;

    if(!image.axes.empty())
    {
        os.data.set_axes(image.axes);
    }

    if(!image.band_map.empty())
    {
        os.data.set_band_map(image.band_map);
    }

    return os;
}

enum XEnum
{
    XEnum_Value_1,
    XEnum_Value_2
};

std::ostream &operator<<(std::ostream &os, XEnum v)
{
    switch (v)
    {
    case XEnum_Value_1: os << "XEnum_Value_1"; break;
    case XEnum_Value_2: os << "XEnum_Value_2"; break;
    }

    return os;
}

namespace foo_namespace
{
class Point
{
public:
    Point(int x_, int y_):
        x(x_), y(y_)
    {
    }

    int x;
    int y;
};

std::ostream &operator<<(std::ostream &os, const Point &p)
{
    return os << p.x << ';' << p.y;
}
}

class Date
{
    int year_, month_, day_;
public:
    Date(): year_(0), month_(0), day_(0) {}

    Date(int year, int month, int day) : year_(year), month_(month), day_(day) {}

    friend std::ostream &operator<<(std::ostream &os, const Date &d);
};

std::ostream &operator<<(std::ostream &os, const Date &d)
{
    return os << d.year_ << '-' << d.month_ << '-' << d.day_ << "\n" << "date";
}

QDebug &operator<<(QDebug os, const Date &d)
{
    std::ostringstream std_os;

    std_os << d;

    return os << QString::fromStdString(std_os.str());
}

namespace space_1
{

void lambda_call()
{
    auto lambda = []
    {
        X_CALL_F;
    };

    lambda();
}

namespace space_2
{

template<class T>
class MyClass
{
public:
    template<class K>
    void foo1(K /*arg1*/, std::vector<K> /*arg2*/ = std::vector<K>()) const
    {
        X_CALL;

        X_ASSERT(1 == 2);
    }

    virtual void foo2(std::vector<float> *) const
    {
        X_CALL;

        X_ASSERT(1 == 2);
    }
};

}

class MyClass
{
public:
    ~MyClass()
    {
        X_CALL;
    }

    void on_null()
    {
        X_CALL;
    }

    void on_deleted()
    {
        X_CALL;

        _field = 0;
    }

    void bar()
    {
        int bar_value = 0;

        X_VALUE(bar_value);
    }

private:
    int _field;
};

std::ostream &operator<<(std::ostream &os, const MyClass &)
{
    return os << "std stream operator for MyClass";
}

trace_x::stream &operator<<(trace_x::stream &os, const MyClass &)
{
    os.s << "trace stream operator for MyClass";

    return os;
}

}

Tracer::Tracer(QThread *thread, QWidget *parent) :
    QFrame(parent),
    ui(new Ui::Tracer)
{
    ui->setupUi(this);

    // copy unilog config file

    QString unilog_config = QDir::tempPath() + "/unilog.conf";

    QFile::remove(unilog_config);
    QFile::copy(":/test_data/unilog_config", unilog_config);

//    unilog::Unilog::instance()->configure(unilog_config.toLocal8Bit().data());

//    auto unilogger = unilog::Unilog::instance()->getLogger(unilog::Unilog::FC_CORE, "trace_x_test_app");

//    UNILOG_INFO(unilogger) << "Info message 1: " << 345436 << std::endl;

    // fetch test images from resources

    _test_data_dir = QDir(QDir::tempPath() + "/trace_x_test_data");

    _test_data_dir.mkpath(_test_data_dir.absolutePath());

    foreach (QString image_name, QDir(":/image/test_images").entryList())
    {
        QFile::copy(QDir(":/image/test_images").absoluteFilePath(image_name), _test_data_dir.absoluteFilePath(image_name));
    }

    //

    qRegisterMetaType< boost::function<void()> >("boost::function<void()>");

    TraceWorker *new_worker = new TraceWorker;

    new_worker->moveToThread(thread);

    connect(this, &Tracer::do_trace, new_worker, &TraceWorker::do_trace);
    connect(new_worker, &TraceWorker::finished, this, &Tracer::slot_finished);

    if(!thread->isRunning())
    {
        connect(this, &Tracer::get_tid, new_worker, &TraceWorker::get_id, Qt::BlockingQueuedConnection);

        thread->start();

        quint64 tid = 0;

        emit get_tid(tid);

        ui->label_tid->setText(QString("TID = %1").arg(tid));
    }
    else
    {
        ui->label_tid->setText(QString("TID = %1").arg(quint64(QThread::currentThreadId())));
    }

    add_trace("trace to file", boost::bind(&Tracer::trace_to_file, this));
    add_trace_2("inf images", boost::bind(&Tracer::test_inf_images, this, _1));
    add_trace("test big images", boost::bind(&Tracer::test_big_images, this));
    add_trace("test images", boost::bind(&Tracer::test_images, this));
    add_trace("test 10k images", boost::bind(&Tracer::test_many_images, this));
    add_trace("test rotation", boost::bind(&Tracer::test_rotation, this));
    add_trace("test profiler", boost::bind(&Tracer::test_profiler, this));
    add_trace("test profiler 2", boost::bind(&Tracer::test_profiler2, this));
    add_trace("1M messages, crash", boost::bind(&Tracer::trace5000k_crash, this));
    add_trace("1M messages  ", boost::bind(&Tracer::trace1000k, this));
    add_trace("100K messages", boost::bind(&Tracer::trace100k, this));
    add_trace("10x100 ms sleep", boost::bind(&Tracer::trace_1000ms, this));
    add_trace_2("inf 100 ms", boost::bind(&Tracer::trace_inf, this, _1));
    add_trace_2("inf", boost::bind(&Tracer::trace_inf_2, this, _1));
    add_trace("with crash", boost::bind(&Tracer::test_with_crash, this));
    add_trace("test messages", boost::bind(&Tracer::trace_test, this));
    add_trace("1 message    ", boost::bind(&Tracer::trace1, this));
}

Tracer::~Tracer()
{
    delete ui;
}

void Tracer::init()
{
}

void Tracer::trace_call()
{
    X_CALL;

    X_INFO("hello");
}

void Tracer::trace1()
{
    X_WARNING("warning!");
}

#undef TRACE_X_MODULE_NAME
#define TRACE_X_MODULE_NAME "tracer_module_2"

void test_help_1()
{
    //test for equal class short names

    space_1::space_2::MyClass<double> c;

    c.foo1<int>(0);

    space_1::MyClass c2;

    c2.bar();
}

#undef TRACE_X_MODULE_NAME
#define TRACE_X_MODULE_NAME "tracer_module_3"

void test_help_2()
{
    X_WARNING_F("warning {1} {0}", 2, 1);
}

#undef TRACE_X_MODULE_NAME
#define TRACE_X_MODULE_NAME "tracer_module_4"

void test_help_3()
{
    X_DEBUG_F("warning {1} {0}", 2, 1);
}

#undef TRACE_X_MODULE_NAME
#define TRACE_X_MODULE_NAME "tracer_module_5"

void test_help_4()
{
    X_IMPORTANT_F("warning {1} {0}", 2, 1);
}

#undef TRACE_X_MODULE_NAME
#define TRACE_X_MODULE_NAME "tracer_module_1"

void Tracer::trace100k()
{
    for(size_t i = 0; i < 100000; i++)
    {
        X_INFO_F("hello {}", i);
    }
}

void test_call_stack_3()
{
    X_CALL_F;

    X_INFO_F("on test_call_stack_3() 1");

    X_INFO_F("on test_call_stack_3() 2");
}

#undef TRACE_X_MODULE_NAME
#define TRACE_X_MODULE_NAME "tracer_module_3"

void test_call_stack_2()
{
    X_CALL_F;

    test_call_stack_3();

    X_INFO_F("on test_call_stack_2() 1");

    test_call_stack_3();

    X_INFO_F("on test_call_stack_2() 2");
}

#undef TRACE_X_MODULE_NAME
#define TRACE_X_MODULE_NAME "tracer_module_1"

void test_call_stack()
{
    X_CALL_F;

    X_INFO_F("on test_call_stack() 1");

    test_call_stack_2();

    X_INFO_F("on test_call_stack() 2");
}

#include "trace_x/detail/trace_x_off.h"

void Tracer::off_test_1()
{
    X_CALL;
    X_DEBUG("1") << 1;
    X_DEBUG_F("1")
            << 1 << "lol"
            << 1 << "lol";

    X_DEBUG_S("1");
    X_DEBUG_S_F("1");
    X_INFO("1") << 1;
    X_INFO(X_T(4, 5 ,6));
    X_INFO_F("1") << 1;
    X_INFO_S("1");
    X_INFO_S_F("1");
    X_WARNING("1") << 1;
    X_WARNING_F("1") << 1;
    X_WARNING_S("1");
    X_WARNING_S_F("1");
    X_ERROR("1") << 1;
    X_ERROR_F("1") << 1;
    X_ERROR_S("1");
    X_ERROR_S_F("1");
    X_EXCEPTION("1") << 1;
    X_EXCEPTION_F("1") << 1;
    X_EXCEPTION_S("1");
    X_EXCEPTION_S_F("1");
    X_IMPORTANT("1") << 1;
    X_IMPORTANT_F("1") << 1;
    X_IMPORTANT_S("1");
    X_IMPORTANT_S_F("1");
    X_ASSERT(1 == 2);
    X_ASSERT_F(1 == 2);

    X_VALUE(1);
    X_VALUES("numbers", 1, 2, 3);
    X_PARAMS("1");
    X_PARAMS_F("1");
    X_PARAMS_S("1");
    X_PARAMS_S_F("1");

    X_SUSPEND("1") << 1;
    X_SUSPEND_F("1") << 1;
    X_SUSPEND_S("1");
    X_SUSPEND_S_F("1");

    X_RESUME("1") << 1;
    X_RESUME_F("1") << 1;
    X_RESUME_S("1");
    X_RESUME_S_F("1");

    X_SIGNAL("1") << 1;
    X_SIGNAL_F("1") << 1;
    X_SIGNAL_S("1");
    X_SIGNAL_S_F("1");

    X_GUID(1);
    X_GUID_F(1);
}

#include "trace_x/trace_x.h"

void simple_function()
{
    X_CALL_F;
}

void Tracer::trace_test()
{
    X_CALL;

    trace_call();

    /// stream API

    X_INFO() << "test" << "<<" << "API";
    X_WARNING() << "test" << "<<" << "API ";

    trace_x::x_logger stdout_log = trace_x::logger::cout();

    X_WARNING(stdout_log) << "test" << "<<" << "API";
    X_WARNING(stdout_log, "test") << "<<" << "API";
    X_ERROR("{}", "test") << "<<" << "API";

    ///  format API

    boost::filesystem::path test = "path";

    std::cerr << "test std::cerr " << test << std::endl;

    int value = 0;

    X_INFO("++value + 6 = {}", ++value + 6);

    {
        std::vector<Date> std_vector;

        std_vector.push_back(Date(1989, 1, 1));
        std_vector.push_back(Date(1982, 12, 1));
        std_vector.push_back(Date(1939, 11, 1));

#if ((_MSC_VER > 1600) || !_MSC_VER)

        std::vector<std::string> vv = {"hello", "world"};

        X_ERROR("this is std::vector: {}", vv);

        std::vector<uint8_t> char_vector = { 1, 2, 3, 4, 5 };

        X_ERROR("this is std::vector<uint8_t>: {}", char_vector);

        X_ERROR() << char_vector;

#endif

        QVector<Date> q_vector;

        q_vector.push_back(Date(1989, 1, 1));
        q_vector.push_back(Date(1982, 12, 1));
        q_vector.push_back(Date(1939, 11, 1));

        X_ERROR("this is QVector: {}", q_vector);

        X_WARNING(X_T(q_vector));

        //

        std::map<int, std::string> std_map;

        std_map[0] = "zero";
        std_map[1] = "one";
        std_map[2] = "two";

        X_WARNING(X_T(std_map));

        std::pair<double, Date> std_pair = std::make_pair<double, Date>(1.0, Date(1989, 1, 1));

        X_WARNING(X_T(std_pair));
    }

    {
        std::list<float> std_list;

        std_list.push_back(101);
        std_list.push_back(102);
        std_list.push_back(103);

        X_ERROR_F("this is std::list: {}", std_list);

        std::vector< std::list<float> > std_2dvector;

        std_2dvector.push_back(std_list);
        std_2dvector.push_back(std_list);
        std_2dvector.push_back(std_list);

        X_WARNING_F("this is 2D stl container: {}", std_2dvector);
    }

    //lambda function test
    auto lambda_1 = []
    {
        X_CALL_F;

        X_WARNING_F("{0}:{0}", 4);
    };

    lambda_1();

    auto lambda_2 = []
    {
        X_CALL_F;
    };

    lambda_2();

    space_1::lambda_call();

    //

    qDebug() << "qDebug:" << QDateTime::currentDateTime();

    connect(this, SIGNAL(nonexistent_signal()), this, SLOT(trace1000k()));

    {
        Date date(1989, 19, 10);

        X_INFO(X_T(date));

        X_INFO(date);
        X_VALUE(date);

        Date *pdate = &date;

        X_INFO("pdate: {}", (void*)(pdate));

        // X_VALUE(pdate);
    }

    X_EXCEPTION("iii");
    X_EXCEPTION("aaa");

    X_ERROR("error");

    QString russian_string = "русские буквы";

    X_INFO("русские буквы");
    X_INFO(russian_string);

    X_INFO("max(uint64_t): {}", std::numeric_limits<uint64_t>::max());

    {
        Date date(2011, 19, 10);

        X_VALUE(date);
    }

    test_help_1();
    test_help_2();
    test_help_3();
    test_help_4();

    std::cout << "test" << " std::cout 1" << std::endl;
    std::cout << "test"; std::cout << " std::cout 2" << std::endl;

    std::clog << "test" << " std::clog" << std::endl;

    libzero::foo();

    //libone::foo();

    //test for non-unique function_name resolving
    //libtwo::foo();

    X_INFO("info without args");
    X_INFO_S("info without args");

    //qt types test

    QString q_string = "q_string";
    X_INFO(q_string);

    QMap<QString, QList<int>> qmap;

    qmap["key_1"] = QList<int>() << 0 << 1 << 2;
    qmap["key_2"] = QList<int>() << 10 << 100 << 200;

    X_INFO("this is a QMap: {}", qmap);

    QMap<int, double> qmap_2;

    qmap_2[0] = 1.1;
    qmap_2[1] = 2.23;

    X_INFO("this is a QMap: {}", qmap_2);

#if ((_MSC_VER > 1600) || !_MSC_VER)
    int x = 1;

    X_INFO("named arguments: {x}, {qmap}", X_CAPTURE_ARGS(x, qmap));
    X_INFO("named arguments: {name_1} {name_2}", X_CAPTURE_ARG("name_1", x), X_CAPTURE_ARG("name_2", qmap));

#endif

    //

    {
        space_1::MyClass cls;

        //test for std:: and trace_x:: otput streams

        std::cout << cls << std::endl;
        X_DEBUG("{}", cls);
    }

    //non-allocated context test
//    {
//        space_1::MyClass *cls;

//        cls->on_null();
//    }

    char data[] = "data";

    X_DEBUG("this is raw c-array: {}", X_CAPTURE_ARRAY(data, 4));

    // raw c-array
    int arr[] = { 1, 4, 9, 16, 32 };

    X_DEBUG("this is raw c-array: {}", X_CAPTURE_ARRAY(arr));

#if ((_MSC_VER > 1600) || !_MSC_VER)

    std::vector<int> v = { 1, 2 ,3 };

    X_DEBUG("this is raw c-array: {}", X_CAPTURE_ARRAY(v.data(), v.size()));

#endif

    //TODO
    //X_DEBUG("brace-enclosed: {}", {1, 2});

    //TODO deleted context test
    //    {
    //        space_1::MyClass *cls = new space_1::MyClass;

    //        delete cls;

    //        cls->on_deleted();
    //    }

    //

    {
        QRect rect(100, 150, 190, 200);

        X_VALUE(rect);
        rect.setTop(110);
        X_VALUE(rect);
        X_VALUE("rect_1", rect);
        X_VALUE("rect_2", "{};{} {}x{}", rect.left(), rect.top(), rect.width(), rect.height());
    }

    int r = 100; int g = 150; int b = 255;

    X_VALUE("color", X_T(r, g, b));

    X_INFO(X_T(r, g, b));

    X_INFO_S("r=%d, g=%d, b=%d", r, g, b) << " colors";

    //    X_INFO("r, g, b");

    //    X_INFO(r, g, b);

    void *tracer_pointer = this;

    X_VALUE_S_F(tracer_pointer, "0x%x");
    X_VALUE(tracer_pointer);
    X_INFO("tracer_pointer : {}", tracer_pointer);

    //

    int8_t *flag = new int8_t;
    *flag = 1;

    X_INFO_S("flag: %d", *flag);
    X_INFO("flag: {0:d}", *flag);

    delete flag;

    uint8_t char_number = 10;

    std::cout << "char_number: " << +char_number << std::endl;

    X_VALUE(char_number);

    //

    foo_namespace::Point p(10, 20);

    X_DEBUG(p);

    std::vector<foo_namespace::Point> point_vector;

    point_vector.push_back(p);

    X_INFO(point_vector);

    X_INFO("enum: {}", XEnum_Value_2);

    //

    QImage qt_image(_test_data_dir.absoluteFilePath("rgba32.png"));

    X_IMAGE(qt_image);

    //

    X_DEBUG("test\nmultiline\nstring");

    //

    test_call_stack();

    off_test_1();
    off_test();

#ifdef Q_OS_WIN
    GUID guid;

    CoCreateGuid(&guid);

    X_GUID(guid);
#endif

    //

    //#ifdef WITH_OPENMP

    //#pragma omp parallel for
    //    for(int i = 0; i < 16; ++i)
    //    {
    //        simple_function();

    //        X_INFO("from thread {}", i);
    //    }

    //#endif

    {
        trace_x::x_logger null_log = trace_x::logger::null();

        X_INFO(null_log, "null_log");

        trace_x::x_logger file_log = trace_x::logger::file("log.txt");

        X_INFO(file_log, "hello {}", "world");
        X_INFO(file_log, 666);
        X_INFO(file_log, "hello {}", "world");

        trace_x::x_logger stdout_log = trace_x::logger::cout();

        X_WARNING(stdout_log, "hello {}", "world");
        X_WARNING(stdout_log, 666);
        X_WARNING_F(stdout_log, "hello {}", "world");

        trace_x::x_logger complex_log = trace_x::logger::complex({file_log, stdout_log});

        X_ERROR(complex_log, "complex_log hello {}", "world");
        X_ERROR(complex_log, 666);
        X_ERROR_F(complex_log, "complex_log hello {}", "world");
    }

    {
        X_INFO(stdout, "stdout {}", "test");
        X_INFO(stderr, "stderr {}", "test");

        X_INFO(std::cout, "std::cout {}", "test");
        X_INFO(std::cerr, "std::cerr {}", "test");
        X_INFO(std::clog, "std::clog {}", "test");
    }

    {
        intptr_t *refcount = new intptr_t;

        *refcount = 100;

        X_INFO_S("refcount: %p: %d", (void*)(refcount), *refcount);

        delete refcount;
    }
}

#undef TRACE_X_MODULE_NAME
#define TRACE_X_MODULE_NAME "tracer_module_2"

void Tracer::trace1000k()
{
    for(size_t i = 0; i < 1000000; i++)
    {
        X_INFO("hello {}", i);
    }

    X_WARNING("1000k warning");
}

void Tracer::trace5000k_crash()
{
    for(size_t i = 1; i <= 1000000; i++)
    {
        X_INFO("hello {}", i);
    }

    int *dead = (int*)(0xDEAD);

    delete dead;
}

void Tracer::trace_1000ms()
{
    for(size_t i = 0; i < 10; ++i)
    {
        X_VALUE(i);

        QThread::currentThread()->msleep(100);
    }
}

void Tracer::trace_inf(int *flag)
{
    int age = 90;
    std::string name = "bob";

    while(*flag)
    {
        X_VALUE("client", X_T(age, name));

        QThread::currentThread()->msleep(100);
    }
}

void Tracer::trace_inf_2(int *flag)
{
    int a = 100;

    while(*flag)
    {
        trace_test();
        //X_INFO("{}", a);
    }
}

void Tracer::test_with_crash()
{
    X_CALL;

    QImage *image;

    image->fill(255);
}

///////////////////////////////////////////////

void profile_1()
{
    X_CALL_F;

    QThread::currentThread()->msleep(200);
}

void profile_2()
{
    X_CALL_F;

    QThread::currentThread()->msleep(150);

    profile_1();
}

void Tracer::test_profiler()
{
    X_CALL;

    QThread::currentThread()->msleep(100);

    profile_2();
    profile_1();
}

void test_profiler_fun()
{
    X_CALL_F;
}

void Tracer::test_profiler2()
{
    for(size_t i = 0; i < 1000000; i++)
    {
        test_profiler_fun();
    }
}

void Tracer::test_rotation()
{
    static int index = 0;

    for(size_t i = 0; i < 50; i++)
    {
        X_WARNING(index);
        X_EXCEPTION(index);
        X_ERROR(index);

        index++;
    }
}

void Tracer::test_images()
{
    X_CALL;

    QImage qt_null_image;

    X_IMAGE(qt_null_image);

    QImage qt_rgba(_test_data_dir.absoluteFilePath("rgba32.png"));

    X_IMAGE(qt_rgba);
    X_IMAGE(qt_rgba, "qt_rgba_alias");
    X_IMAGE(qt_rgba, "qt_rgba_alias", "simple message");
    X_IMAGE(qt_rgba, "qt_rgba_alias", "{} message", "formatted");

    qt_rgba = qt_rgba.convertToFormat(QImage::Format_RGBA8888);

    X_IMAGE(qt_rgba);

    //

    QImage qt_bgr(_test_data_dir.absoluteFilePath("rgb24.bmp"));

    qt_bgr = qt_bgr.convertToFormat(QImage::Format_RGB888);

    X_IMAGE(qt_bgr);

    //

    QImage qt_RGB444 = qt_bgr.convertToFormat(QImage::Format_RGB444);

    X_IMAGE(qt_RGB444);

    //

    QImage qt_mono_binary = QImage(_test_data_dir.absoluteFilePath("mono_binary.bmp"));

    X_IMAGE(qt_mono_binary);

    //

    QImage qt_mono_color = QImage(_test_data_dir.absoluteFilePath("mono_color.bmp"));

    X_IMAGE(qt_mono_color);

    //

#ifdef cimg_use_tiff

    cimg_library::CImg<uint8_t> c_img_8u_1ch(qPrintable(_test_data_dir.absoluteFilePath("8u_1ch.tif")));

    X_IMAGE(c_img_8u_1ch);

    //

    CImgX<uint8_t> c_img_8u_1ch_binary;

    uint8_t color_8u = 0;
    c_img_8u_1ch_binary.image = c_img_8u_1ch;

    c_img_8u_1ch_binary.image.draw_text(10, 10, "binary", &color_8u);
    c_img_8u_1ch_binary.band_map = { trace_x::BAND_BINARY };

    X_IMAGE(c_img_8u_1ch_binary);

    //

    cimg_library::CImg<uint8_t> c_img_8u_3ch(qPrintable(_test_data_dir.absoluteFilePath("rgb24.bmp")));

    X_IMAGE(c_img_8u_3ch);

    //

    c_img_8u_3ch.permute_axes("yzcx");
    CImgX<uint8_t> c_img_8u_3ch_yzcx;
    c_img_8u_3ch_yzcx.image.assign(c_img_8u_3ch.data(), qt_bgr.width(), qt_bgr.height(), 1, 3);
    c_img_8u_3ch_yzcx.axes = { 2, 3, 0, 1 };

    X_IMAGE(c_img_8u_3ch_yzcx);

    //

    cimg_library::CImg<float> c_img_32f_1ch(qPrintable(_test_data_dir.absoluteFilePath("32f_1ch.tif")));

    X_IMAGE(c_img_32f_1ch);

    //

    cimg_library::CImg<uint16_t> c_img_16u_1ch(qPrintable(_test_data_dir.absoluteFilePath("16u_1ch.tif")));

    //

    X_IMAGE(c_img_16u_1ch);

    CImgX<uint16_t> c_img_16u_1ch_binary;

    uint16_t color = 0;
    c_img_16u_1ch_binary.image = c_img_16u_1ch;

    c_img_16u_1ch_binary.image.draw_text(10, 10, "binary", &color);
    c_img_16u_1ch_binary.band_map = { trace_x::BAND_BINARY };

    X_IMAGE(c_img_16u_1ch_binary);

    cimg_library::CImg<uint32_t> c_img_32u_3ch(qPrintable(_test_data_dir.absoluteFilePath("32u_3ch.tif")));

    X_IMAGE(c_img_32u_3ch);

    //

    c_img_32u_3ch.permute_axes("xzyc");
    CImgX<uint32_t> c_img_32u_3ch_xzyc;
    c_img_32u_3ch_xzyc.image.assign(c_img_32u_3ch.data(), 73, 43, 1, 3);
    c_img_32u_3ch_xzyc.axes = { 1, 3, 2, 0 };
    X_IMAGE(c_img_32u_3ch_xzyc);

    //

    cimg_library::CImg<uint16_t> c_img_16u_4ch(qPrintable(_test_data_dir.absoluteFilePath("16u_4ch.tif")));

    X_IMAGE(c_img_16u_4ch);

    //

    cimg_library::CImg<int32_t> c_img_32s_1ch(qPrintable(_test_data_dir.absoluteFilePath("32s_1ch.tif")));

    X_IMAGE(c_img_32s_1ch);

    //

    cimg_library::CImg<int32_t> c_img_32s_1ch_2(qPrintable(_test_data_dir.absoluteFilePath("32s_1ch_2.tif")));

    X_IMAGE(c_img_32s_1ch_2);

    //

    cimg_library::CImg<uint8_t> c_img_8u_2ch(qPrintable(_test_data_dir.absoluteFilePath("8u_2ch.tif")));

    X_IMAGE(c_img_8u_2ch);

    //

    cimg_library::CImg<int16_t> c_img_16s_2ch(qPrintable(_test_data_dir.absoluteFilePath("16s_2ch.tif")));

    X_IMAGE(c_img_16s_2ch);

    //    cimg_library::CImg<double> c_img_64f_2ch(qPrintable(_test_data_dir.absoluteFilePath("64f_2ch.tif")));

    //    X_IMAGE(c_img_64f_2ch);

#endif // cimg_use_tiff

#ifdef WITH_OPENCV

    cv::Mat cv_mat_bgr = cv::imread(std::string(_test_data_dir.absoluteFilePath("rgb24.bmp").toLocal8Bit()), cv::IMREAD_UNCHANGED);

    X_IMAGE(cv_mat_bgr);

    cv::Mat cv_mat_bgra = cv::imread(std::string(_test_data_dir.absoluteFilePath("rgba32.png").toLocal8Bit()), cv::IMREAD_UNCHANGED);

    X_IMAGE(cv_mat_bgra);

    cv::Mat cv_roi_bgr = cv_mat_bgr(cv::Rect(2, 2, 56, 12));

    X_IMAGE(cv_roi_bgr);

    cv::Mat cv_roi_f = cv::imread(std::string(_test_data_dir.absoluteFilePath("32f_1ch.tif").toLocal8Bit()), cv::IMREAD_UNCHANGED)(cv::Rect(40, 40, 60, 40));

    X_IMAGE(cv_roi_f);
#endif // WITH_OPENCV
}

void Tracer::test_many_images()
{
    X_CALL_F;

    for(int i = 0; i < 10000; ++i)
    {
        QImage qt_image(_test_data_dir.absoluteFilePath("rgba32.png"));

        QPainter painter(&qt_image);

        painter.drawText(qt_image.rect(), QString::number(i + 1));

        X_IMAGE(qt_image);
    }
}

void Tracer::test_inf_images(int *flag)
{
    X_CALL_F;

    while(*flag)
    {
        test_images();

        QThread::currentThread()->msleep(100);
    }
}

void Tracer::test_big_images()
{
    X_CALL_F;

    static int counter = 1;

    if(QFile::exists("big.bmp"))
    {
        QImage qt_big_image("big.bmp");

        for(int i = 0; i < 1; ++i)
        {
            QPainter painter(&qt_big_image);

            painter.setFont(QFont(painter.font().family(), 1024));
            painter.drawText(qt_big_image.rect(), QString::number(counter));

            counter++;

            X_IMAGE(qt_big_image);
        }
    }
}

void Tracer::trace_to_file()
{
    trace_x::set_trace_logger(trace_x::logger::file("trace.txt"));
}

///////////////////////////////////////////////

void Tracer::slot_finished(quint64 elapsed, void *arg)
{
    static_cast<TraceButton*>(arg)->set_elapsed(elapsed);
}

void Tracer::add_trace(const QString &name, const boost::function<void ()> &function, bool checkable)
{
    TraceButton *button = new TraceButton(name, checkable);

    ui->verticalLayout->insertWidget(1, button);

    connect(button, &TraceButton::triggered, this, [this, button, function]
    {
        button->clear();

        emit do_trace(function, button);
    });
}

void Tracer::add_trace_2(const QString &name, const boost::function<void (int*)> &function)
{
    TraceButton *button = new TraceButton(name, true);

    ui->verticalLayout->insertWidget(1, button);

    connect(button, &TraceButton::triggered, this, [this, button, function](bool checked)
    {
        if(checked)
        {
            button->clear();

            emit do_trace(boost::bind(function, &button->flag), button);
        }
    });
}

//////////////////////////////////////////////////////////////////////////

TraceWorker::TraceWorker(QObject *parent):
    QObject(parent)
{
}

void TraceWorker::do_trace(boost::function<void()> function, void *arg)
{
    QElapsedTimer timer;

    timer.start();

    function();

    emit finished(timer.elapsed(), arg);
}

void TraceWorker::get_id(quint64 &id)
{
    id = quint64(QThread::currentThreadId());
}
