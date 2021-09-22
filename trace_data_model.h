#ifndef TRACE_DATA_MODEL_H
#define TRACE_DATA_MODEL_H

#include <QObject>
#include <QMutexLocker>

#include <boost/atomic.hpp>

#include "trace_model.h"
#include "tx_index.h"

//! Класс модели данных трассы
class TraceDataModel : public QObject
{
    Q_OBJECT

public:
    explicit TraceDataModel(QObject *parent = 0);

    inline size_t size() const { return _trace_list.count(); }
    inline size_t safe_size() const { return _safe_size; }

    inline void remove_first(index_t index)
    {
        if(!_trace_list.isEmpty() && (_trace_list.first()->index == index))
        {
            _trace_list.removeFirst();
        }
    }

    //! return false, if equal index is not finded. In this case relative_index contains nearest relative index
    bool find_relative_index(index_t trace_index, index_t &relative_index) const;

    index_t relative_index(index_t index) const { return index - _trace_list.first()->index; }
    index_t trace_index(index_t index) const { return index + _trace_list.first()->index; }

    inline trace_message_t *at(size_t i) { return const_cast<trace_message_t*>(_trace_list[int(i)]); }
    inline const trace_message_t *at(size_t i) const { return _trace_list.at(int(i)); }
    inline trace_message_t get(size_t i) const { return *_trace_list.at(int(i)); }
    inline const trace_message_t *safe_at(size_t i) const { QMutexLocker locker(&_trace_mutex); return _trace_list.at(i); }
    inline const trace_message_t *value(size_t i) const { QMutexLocker locker(&_trace_mutex); return _trace_list.value(i, 0); }

    void append(const trace_message_t *message);
    void append_fast(const trace_message_t *message) { _trace_list.append(message);  _has_new_messages = true; }
    void insert(const trace_message_t *message, int index);
    void update_index(const trace_message_t *message);
    void emit_refiltered();
    void emit_updated();

    void set_message_list(const QList<const trace_message_t *> &list);

    void clear();

    inline void lock() { _trace_mutex.lock(); }
    inline void unlock() { _trace_mutex.unlock(); }

    inline QMutex *mutex() { return &_trace_mutex; }
    inline QMutex *index_mutex() { return &_index_mutex; }

    const QList<const trace_message_t*> &trace_list() const;
    QList<const trace_message_t*> &trace_list() { return _trace_list; }

    const trace_index_t & index_model() const { return _model_index; }

    bool get_nearest_by_type(index_t current, uint8_t type, index_t &index) const;
    bool get_next_by_type(index_t current, uint8_t type, index_t &index) const;
    bool get_prev_by_type(index_t current, uint8_t type, index_t &index) const;

signals:
    void updated();
    void cleaned();
    void refiltered();
    void model_changed();

private slots:
    void check_updates();

private:
    friend class TraceController;
    friend class TraceModelService;

    mutable QMutex _trace_mutex;
    mutable QMutex _index_mutex;

    boost::atomic<bool> _has_new_messages;
    boost::atomic<bool> _has_new_indexes;
    boost::atomic<bool> _emit_refiltered;

    QList<const trace_message_t*> _trace_list;
    mutable trace_index_t _model_index;

    size_t _safe_size; //! thread-safe size field, used in main gui thread
};

#endif // TRACE_DATA_MODEL_H
