#include "data_storage.h"

#include <QtConcurrent/QtConcurrent>

#include "trace_x/trace_x.h"

class DumpTask : public QRunnable
{
public:
    DumpTask(DataStorage *storage_):
        storage(storage_)
    {
    }

    void run()
    {
        storage->swap_memory();
    }

    DataStorage *storage;
};

DataStorage::DataStorage(QObject *parent) : QObject(parent),
    _total_size(0),
    _memory_data_size(0),
    _memory_limit(512 * 1024 * 1024ull),
    _swap_limit(1024 * 1024 * 1024ull),
    _use_swap(true)
{
    X_CALL;

    _swap_thread_pool.setMaxThreadCount(1);
}

void DataStorage::append_data(uint64_t index, const QByteArray &array)
{
    X_CALL;

    QMutexLocker lock(&_mutex);

    _memory_data_size += array.size();
    _total_size += array.size();

    _memory_map[index] = array;

    _memory_data_list.append(index);

    if(_memory_data_size > _memory_limit)
    {
        if(_use_swap)
        {
            //push task to single thread queue

            _swap_thread_pool.start(new DumpTask(this));
        }
        else
        {
            // no swapping, so let`s free 10 % memory

            size_t size_to_free = 0.1 * double(_memory_limit);
            size_t free_counter = 0;

            do
            {
                quint64 index = _memory_data_list.takeFirst();

                free_counter += _memory_map[index].size();

                _memory_map.remove(index);
            }
            while(free_counter < size_to_free);

            _total_size -= free_counter;
            _memory_data_size -= free_counter;
        }
    }
}

void DataStorage::clear()
{
    X_CALL;

    QMutexLocker lock(&_mutex);

    if(_swap_file.autoRemove())
    {
        _swap_file.resize(0);
        _swap_file.seek(0);
    }

    _total_size = 0;
    _memory_data_size = 0;

    _memory_map.clear();
    _memory_data_list.clear();

    _swap_map.clear();
    _swap_data_list.clear();
}

void DataStorage::set_swap_state(bool enabled)
{
    X_CALL;

    if(_use_swap == enabled) return;

    _use_swap = enabled;

    X_VALUE(_use_swap);

    if(!enabled)
    {
        _total_size -= _swap_file.size();

        _swap_file.close();
        _swap_map.clear();
        _swap_data_list.clear();
    }
    else if(!_swap_file.isOpen())
    {
        open_swap();
    }
}

void DataStorage::set_swap_dir(const QString &path)
{
    X_CALL;

    if(_swap_dir == path) return;

    _swap_dir = path;

    X_VALUE(_swap_dir);

    _swap_file.close();

    _swap_file.setFileTemplate(_swap_dir + "/trace_x");

    open_swap();
}

void DataStorage::set_swap_file(const QString &filename, qint64 offset)
{
    X_CALL;

    _swap_file.setFileName(filename);

    if(_swap_file.open())
    {
        _swap_file.setAutoRemove(false);

        X_VALUE("_swap_file", filename);

        QDataStream file_stream(&_swap_file);

        _swap_file.seek(offset);

        file_stream >> _swap_data_list >> _swap_map;
    }
    else
    {
        X_ERROR("can`t open swap file at {}", filename);
    }
}

void DataStorage::set_memory_limit(size_t size)
{
    X_CALL;

    QMutexLocker lock(&_mutex);

    _memory_limit = size;

    X_VALUE(_memory_limit);
}

void DataStorage::set_swap_limit(size_t size)
{
    X_CALL;

    if(_swap_limit == size) return;

    QMutexLocker lock(&_mutex);

    X_VALUE(_swap_limit);

    _swap_limit = size;

    if(_swap_file.size() > _swap_limit)
    {
        //truncate swap file

        _swap_map.clear();

        _swap_file.resize(_swap_limit);
    }
}

void DataStorage::remove_from_tail(uint64_t index)
{
    X_CALL;

    QMutexLocker lock(&_mutex);

    if(!_memory_data_list.empty() && _memory_data_list.first() == index)
    {
        quint64 data_size = _memory_map[index].size();

        _total_size -= data_size;
        _memory_data_size -= data_size;

        _memory_map.remove(index);
        _memory_data_list.removeOne(index);
    }

    if(!_swap_data_list.empty() && _swap_data_list.first() == index)
    {
        quint64 data_size = _memory_map[index].size();

        _total_size -= data_size;

        _swap_map.remove(index);
        _swap_data_list.removeOne(index);
    }
}

QByteArray DataStorage::get_data(uint64_t index, bool detach)
{
    X_CALL;

    QByteArray data_array;

    if(_swap_map.contains(index))
    {
        DataObject &data = _swap_map[index];

        data_array.resize(data.size);

        X_INFO("read swap #{} : {} - {}", index, data.start, data.end);

        read_swap(data_array.data(), data.start, data.size);
    }
    else if(_memory_map.contains(index))
    {
        data_array = _memory_map[index];
    }

    if(detach)
    {
        //detach because we won`t change original data buffer

        data_array.detach();
    }

    return data_array;
    //return qUncompress(data_array);
}

void DataStorage::free_swap(quint64 start, quint64 size)
{
    X_CALL;

    if(!_swap_data_list.empty())
    {
        if(start + size > _swap_limit)
        {
            quint64 chunk = _swap_limit - start;

            free_swap(start, chunk);
            free_swap(0, size - chunk);
        }
        else
        {
            do
            {
                quint64 index = _swap_data_list.first();

                const DataObject &obj = _swap_map[index];

                if(is_intersects(obj.start, obj.end, start, start + size - 1))
                {
                    _total_size -= obj.size;

                    _swap_data_list.removeFirst();
                    _swap_map.remove(index);

                    X_INFO("remove {} from swap [{}, {}]", index, obj.start, obj.end);
                }
                else
                {
                    break;
                }
            }
            while(!_swap_data_list.isEmpty());
        }
    }
}

void DataStorage::write_swap(quint64 start, const char *data, quint64 size)
{
    X_CALL;

    if(start + size > _swap_limit)
    {
        quint64 chunk = _swap_limit - start;

        write_swap(start, data, chunk);
        write_swap(0, data + chunk, size - chunk);
    }
    else
    {
        X_INFO("write swap to {} : {} bytes", start, size);

        _swap_file.seek(start);

        _swap_file.write(data, size);
    }
}

void DataStorage::read_swap(char *dest, quint64 start, quint64 size)
{
    X_CALL;

    if(start + size > quint64(_swap_file.size()))
    {
        quint64 chunk = _swap_file.size() - start;

        read_swap(dest, start, chunk);
        read_swap(dest + chunk, 0, size - chunk);
    }
    else
    {
        qint64 orig_pos = _swap_file.pos();

        X_INFO("read {} bytes from {} ", size, start);

        _swap_file.seek(start);

        // TODO read by chunks for break_flag support
        _swap_file.read(dest, size);

        _swap_file.seek(orig_pos);
    }
}

void DataStorage::swap_memory()
{
    X_CALL;

    _total_size -= _memory_data_size;

    _memory_data_size = 0;

    QMutexLocker lock(&_mutex);

    quint64 data_size = 0;

    foreach (quint64 index, _memory_data_list)
    {
        data_size += _memory_map[index].size();
    }

    // forget about old data in tale of swap - it`s gone ¯\_(ツ)_/¯

    free_swap(_swap_file.pos(), data_size);

    if(data_size > _swap_limit)
    {
        // we can`t swap all data in memory - remove data from tail

        do
        {
            quint64 index = _memory_data_list.takeFirst();
            quint64 size =  _memory_map[index].size();

            data_size -= size;

            X_IMPORTANT("drop #{} : ", index, size);
        }
        while(data_size > _swap_limit);
    }

    foreach (quint64 index, _memory_data_list)
    {
        const QByteArray &data = _memory_map[index];

        X_INFO("write swap #{}", index);

        DataObject &swap_info = _swap_map[index];

        swap_info.start = _swap_file.pos();
        swap_info.size = data.size();

        write_swap(_swap_file.pos(), data.data(), data.size());

        swap_info.end = _swap_file.pos() - 1;

        _swap_data_list.append(index);

        _total_size += data.size();
    }

    _memory_data_list.clear();
    _memory_map.clear();
}

void DataStorage::copy_data_to(QFile *dest)
{
    X_CALL;

    QMutexLocker lock(&_mutex);

    QDataStream stream(dest);

    QMap<quint64, DataObject> swap_map;

    QList<quint64> data_list = _swap_data_list + _memory_data_list;

    stream << data_list;

    foreach (quint64 index, data_list)
    {
        swap_map[index].start = 0;
    }

    qint64 start_pos = dest->pos();

    stream << swap_map;

    foreach (quint64 index, data_list)
    {
        QByteArray array = get_data(index, false);

        DataObject &swap_info = swap_map[index];

        swap_info.start = dest->pos();
        swap_info.size = array.size();

        dest->write(array);

        swap_info.end = dest->pos() - 1;
    }

    dest->seek(start_pos);

    stream << swap_map;
}

void DataStorage::open_swap()
{
    X_CALL;

    if(_swap_file.open())
    {
        X_VALUE("_swap_file", _swap_file.fileName());
    }
    else
    {
        X_ERROR("can`t create swap file at {}", _swap_file.fileTemplate());
    }
}

QByteArray DataStorage::request_data(uint64_t index /*boost::atomic<bool> *break_flag*/)
{
    X_CALL;

    QMutexLocker lock(&_mutex);

    return get_data(index, true);
}

bool DataStorage::is_intersects(quint64 left_1, quint64 right_1, quint64 left_2, quint64 right_2) const
{
    if(left_1 > right_1)
    {
        return is_intersects(left_1, _swap_limit, left_2, right_2) || is_intersects(0, right_1, left_2, right_2);
    }
    else
    {
        return !((left_1 > right_2) || (left_2 > right_1));
    }
}

size_t DataStorage::memory_limit() const
{
    return _memory_limit;
}

size_t DataStorage::total_data_size() const
{
    return _total_size;
}

size_t DataStorage::swap_size() const
{
    return _swap_file.size();
}

QDataStream &operator <<(QDataStream &out, const DataObject &value)
{
    out << value.start << value.end << value.size;

    return out;
}

QDataStream &operator >>(QDataStream &in, DataObject &value)
{
    in >> value.start >> value.end >> value.size;

    return in;
}
