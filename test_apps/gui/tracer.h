#ifndef TRACER_H
#define TRACER_H

#include <QFrame>
#include <QThread>
#include <QDir>

#include <boost/function.hpp>

namespace Ui {
class Tracer;
}

class TraceWorker : public QObject
{
    Q_OBJECT

public:
    TraceWorker(QObject *parent = 0);

public slots:
    void do_trace(boost::function<void()> function, void *arg);
    void get_id(quint64 &id);

signals:
    void finished(quint64 elapsed, void *arg);
};

class Tracer : public QFrame
{
    Q_OBJECT

public:
    explicit Tracer(QThread *thread, QWidget *parent = 0);
    ~Tracer();

    void init();

signals:
    void do_trace(boost::function<void()> function, void *arg);
    void get_tid(quint64 &tid);

private slots:
    void trace_call();
    void trace1();
    void trace100k();
    void trace1000k();
    void trace5000k_crash();
    void trace_test();
    void trace_1000ms();
    void trace_inf(int *flag);
    void trace_inf_2(int *flag);
    void test_with_crash();
    void slot_finished(quint64 elapsed, void *arg);
    void test_profiler();
    void test_profiler2();
    void test_rotation();
    void test_images();
    void test_many_images();
    void test_inf_images(int *flag);
    void test_big_images();
    void trace_to_file();

    void off_test();
    void off_test_1();

private:
    void add_trace(const QString &name, const boost::function<void()> &function, bool checkable = false);
    void add_trace_2(const QString &name, const boost::function<void (int *)> &function);

private:
    Ui::Tracer *ui;

    QDir _test_data_dir;
};

#endif // TRACER_H
