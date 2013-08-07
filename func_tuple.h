#ifndef _FUNC_TUPLE_
#define _FUNC_TUPLE_

template<typename F, typename P = void*>
struct func_tuple_t
{
    func_tuple_t()
    {
    }

    func_tuple_t(const F& f, const P& p):
        func_(f), param_(p)
    {
    }

    func_tuple_t(const func_tuple_t& c):
        func_(c.func_), param_(c.param_)
    {
    }

    F func_;
    P param_;
};

#endif
