#ifndef _SIMPLE_PTR_H_
#define _SIMPLE_PTR_H_

#include <stddef.h>

template<typename T>
struct Deleter
{
    void operator() (T* d)
    {
        delete d;
    }
};

template<typename T>
struct ArrayDeleter
{
    void operator() (T* d)
    {
        delete []d;
    }
};

template <typename T, typename D = Deleter<T> >
class simple_ptr
{
public:
    simple_ptr(T* d = NULL, size_t num = 0):
        ptr_(d),
        size_(sizeof(T)*num)
    {

    }
    ~simple_ptr()
    {
        reset(NULL, 0);
    }

    // Releases the ownership of the managed object
    T* release()
    {
        T* ret = ptr_;
        ptr_ = NULL;
        size_ = 0;
        return ret;
    }

    // Replaces the managed object.
    void reset(T* d, size_t num)
    {
        if (d == ptr_)
        {
            // same object, do nothing
            return;
        }

        T* old = ptr_;
        ptr_ = d;
        size_ = sizeof(T)*num;
        if (old)
        {
            D()(old);
        }
    }

    T* get()
    {
        return ptr_;
    }

    operator T*()
    {
        return ptr_;
    }

    size_t get_size()
    {
        return size_;
    }

    const T* operator->() const
    {
        return  ptr_;
    }

    T* operator->()
    {
        return  ptr_;
    }
    
    const T& operator*()  const
    {
        return *ptr_;
    }

    T& operator*()
    {
        return *ptr_;
    }
private:
    simple_ptr(const simple_ptr&) {}
    simple_ptr& operator = (const simple_ptr&) {}

    T* ptr_;
    size_t size_;
};

#endif
