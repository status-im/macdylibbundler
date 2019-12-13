#ifndef _ParallelForEach_h_
#define _ParallelForEach_h_

#include <dispatch/dispatch.h>

template<typename It, typename F>
inline void parallel_for_each(It a, It b, F&& f)
{
    size_t count = std::distance(a,b);
    using data_t = std::pair<It, F>;
    data_t helper = data_t(a, std::forward<F>(f));
    dispatch_apply_f(count,
                     dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
                     &helper,
                     [](void* ctx, size_t cnt) {
        data_t* d = static_cast<data_t*>(ctx);
        auto elem_it = std::next(d->first, cnt);
        (*d).second(*(elem_it));
    });
}

#endif
