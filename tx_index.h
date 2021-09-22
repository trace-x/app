#ifndef TX_INDEX_H
#define TX_INDEX_H

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include "trace_x/impl/types.h"

#include <QMap>

using boost::multi_index_container;
using namespace boost::multi_index;

class FilterModel;

//! Structure for multi-index container trace_index_t. Contains message description and filters map.
struct message_index_t
{
    message_index_t() {}

    message_index_t(uint8_t type, pid_index_t process, module_index_t module,
                    tid_index_t thread, context_index_t context, function_index_t function,
                    source_index_t source, label_index_t label):
        type(type), process_index(process), module_index(module),
        tid_index(thread), context_index(context),
        function_index(function), source_index(source), label_index(label)
    {}

    uint8_t          type;
    pid_index_t      process_index;
    module_index_t   module_index;
    tid_index_t      tid_index;
    context_index_t  context_index;
    function_index_t function_index;
    source_index_t   source_index;
    label_index_t    label_index;

    // Filter boolean map(first+second filter pair as key) for fast filtration of each message
    mutable QMap<QPair<const FilterModel*, const FilterModel*>, bool> filters_map;

    // Tags
    struct ByType {};
    struct ByPID {};
    struct ByModule {};
    struct ByTID {};
    struct ByContext {};
    struct ByFunction {};
    struct BySource {};

    struct ByKey {};
};

//! Multi-index container for trace model storing
typedef boost::multi_index_container<message_index_t,
indexed_by<
hashed_non_unique<
tag<message_index_t::ByType>, member<message_index_t, uint8_t, &message_index_t::type>
>,
hashed_non_unique<
tag<message_index_t::ByPID>, member<message_index_t, pid_index_t, &message_index_t::process_index>
>,
hashed_non_unique<
tag<message_index_t::ByModule>, member<message_index_t, module_index_t, &message_index_t::module_index>
>,
hashed_non_unique<
tag<message_index_t::ByTID>, member<message_index_t, tid_index_t, &message_index_t::tid_index>
>,
hashed_non_unique<
tag<message_index_t::ByContext>, member<message_index_t, context_index_t, &message_index_t::context_index>
>,
hashed_non_unique<
tag<message_index_t::ByFunction>, member<message_index_t, function_index_t, &message_index_t::function_index>
>,
hashed_non_unique<
tag<message_index_t::BySource>, member<message_index_t, source_index_t, &message_index_t::source_index>
>,

hashed_unique<
tag<message_index_t::ByKey>, composite_key<
message_index_t,
member<message_index_t, uint8_t, &message_index_t::type>,
member<message_index_t, pid_index_t, &message_index_t::process_index>,
member<message_index_t, module_index_t, &message_index_t::module_index>,
member<message_index_t, tid_index_t, &message_index_t::tid_index>,
member<message_index_t, context_index_t, &message_index_t::context_index>,
member<message_index_t, function_index_t, &message_index_t::function_index>,
member<message_index_t, source_index_t, &message_index_t::source_index>,
member<message_index_t, label_index_t, &message_index_t::label_index>
>
>
>
> trace_index_t;

#endif //TX_INDEX_H
