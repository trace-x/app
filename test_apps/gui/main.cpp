#include "mainwindow.h"

#include <iostream>

#include <QApplication>

#ifdef QT_IS_STATIC

#include <QtPlugin>

Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)

#endif

#include "trace_x/trace_x.h"
#include "trace_x/tools/qt_trace_handler.h"

#include "test_apps/lib0/lib0.h"

class Singleton
{
public:
    ~Singleton()
    {
        std::cerr << "~Singleton()\n" << std::endl;
        std::cerr << "~Singleton()\n" << std::endl;
    }
};

static Singleton singletone;

int main(int argc, char *argv[])
{
   // trace_x::start_trace_receiver(trace_x::MODE_SAVE_CRASH);

    std::cerr << "from app " << libzero::Singletone::instance() << std::endl;

    X_INFO_F("----");

    QApplication app(argc, argv);

    MainWindow m;

    m.show();

    return app.exec();
}
