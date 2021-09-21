#ifndef TRACE_X_TYPES_H_
#define TRACE_X_TYPES_H_

#include "stdint.h"

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

namespace
{
    const char* TraceIDDefault = "trace_x";
    const char* RxFlagID = "rx_flag";
    const char* TxFlagID = "tx_flag";
    const char* TxFilterIndexID = "tx_filter_index";
    const char* TxFilterMutexID = "tx_filter_mutex";
    const char* TraceServerNameEnvID = "TRACE_X_SERVER_NAME";
    const char* TraceXConfigEnvID = "TRACE_X_CONFIG";
}

typedef uint64_t index_t;
typedef uint16_t pid_index_t;
typedef uint16_t tid_index_t;
typedef uint32_t context_index_t;
typedef uint16_t module_index_t;
typedef uint16_t class_index_t;
typedef uint32_t function_index_t;
typedef uint16_t source_index_t;
typedef uint32_t label_index_t;

namespace trace_x
{

enum PacketType
{
    TRACE_MESSAGE,
    CONNECT,
    COMMAND
};

//////

using boost::multi_index_container;
using namespace boost::multi_index;

struct message_filter_t
{
    message_filter_t(uint8_t type, uint64_t module,
                    uint64_t tid, uint64_t context,
                    uint64_t function, uint64_t source):
        type(type), module_id(module),
        thread_id(tid), context_id(context),
        function_id(function), source_id(source),
        is_accepted(true)
    {}

    uint8_t    type;
    uint64_t   module_id;
    uint64_t   thread_id;
    uint64_t   context_id;
    uint64_t   function_id;
    uint64_t   source_id;

    mutable module_index_t   module_index;
    mutable tid_index_t      thread_index;
    mutable context_index_t  context_index;
    mutable function_index_t function_index;
    mutable source_index_t   source_index;

    mutable bool is_accepted;

    struct ByKey {};
};

typedef boost::interprocess::allocator<message_filter_t, boost::interprocess::managed_shared_memory::segment_manager> shmem_allocator_f;

typedef boost::multi_index_container<message_filter_t,
indexed_by<
hashed_unique<
tag<message_filter_t::ByKey>, composite_key<
message_filter_t,
member<message_filter_t, uint8_t,  &message_filter_t::type>,
member<message_filter_t, uint64_t, &message_filter_t::module_id>,
member<message_filter_t, uint64_t, &message_filter_t::thread_id>,
member<message_filter_t, uint64_t, &message_filter_t::context_id>,
member<message_filter_t, uint64_t, &message_filter_t::function_id>,
member<message_filter_t, uint64_t, &message_filter_t::source_id>
>
>
>,
shmem_allocator_f
> filter_index_t;

typedef boost::interprocess::allocator<trace_x::filter_index_t, boost::interprocess::managed_shared_memory::segment_manager> shmem_allocator_sstf;

struct filter_index
{
    filter_index():
        index(0),
        mutex(0)
    {}

    filter_index_t *index;
    boost::interprocess::named_mutex *mutex;
};

}

#endif // TRACE_X_TYPES_H_
