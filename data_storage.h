#ifndef DATA_STORAGE_H
#define DATA_STORAGE_H

#include <QObject>
#include <QMap>
#include <QTemporaryFile>
#include <QThreadPool>
#include <QMutex>

#include <boost/function.hpp>
#include <boost/atomic.hpp>

struct DataObject
{
public:
    DataObject():
        start(0),
        end(0),
        size(0)
    {}

    quint64 start;
    quint64 end;
    quint64 size;
};

//! Class for data storaging. Supports asynchronous swapping and rotation
class DataStorage : public QObject
{
    Q_OBJECT

public:
    explicit DataStorage(QObject *parent = 0);

    void append_data(uint64_t index, const QByteArray &array);

    void remove_from_tail(uint64_t index);
    void clear();

    QByteArray request_data(uint64_t index);

    void set_swap_state(bool enabled);
    void set_swap_dir(const QString &path);
    void set_swap_file(const QString &filename, qint64 offset = 0);

    void set_memory_limit(size_t size);
    void set_swap_limit(size_t size);

    void copy_data_to(QFile *dest);

    size_t memory_limit() const;
    size_t total_data_size() const;
    size_t swap_size() const;

private:
    void open_swap();
    void swap_memory();
    void free_swap(quint64 start, quint64 size);
    void write_swap(quint64 start, const char *data, quint64 size);
    void read_swap(char *dest, quint64 start, quint64 size);
    bool is_intersects(quint64 left_1, quint64 right_1, quint64 left_2, quint64 right_2) const;

    QByteArray get_data(uint64_t index, bool detach = false);

private:
    friend class DumpTask;

    bool _use_swap;
    QString _swap_dir;

    size_t _memory_limit; // bytes
    size_t _swap_limit; // bytes

    QMap<quint64, QByteArray> _memory_map;
    QList<quint64> _memory_data_list;

    QMap<quint64, DataObject> _swap_map;
    QList<quint64> _swap_data_list;

    size_t _total_size;
    size_t _memory_data_size;

    QTemporaryFile _swap_file;

    QThreadPool _swap_thread_pool;

    QMutex _mutex;
};

QDataStream & operator << (QDataStream &out, const DataObject &value);
QDataStream & operator >> (QDataStream &in, DataObject &value);

#endif // DATA_STORAGE_H
