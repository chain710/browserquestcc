#ifndef _STRING_HELPER_H_
#define _STRING_HELPER_H_

#include <string>
#include <stdarg.h>
#include <string.h>
#include "simple_ptr.h"

typedef simple_ptr<char, ArrayDeleter<char> > string_ptr;

template <class ITERATOR>
size_t IterDataSize(const ITERATOR& iter)
{
    return iter->size();
}

template <class ITERATOR>
std::string IterData(const ITERATOR& iter)
{
    return *iter;
}

template <class ITERATOR>
void JoinStringsIterator(const ITERATOR& start,
                         const ITERATOR& end,
                         const char* delim,
                         std::string* result) 
{
    result->clear();
    int delim_length = strlen(delim);

    // Precompute resulting length so we can reserve() memory in one shot.
    int length = 0;
    for (ITERATOR iter = start; iter != end; ++iter) {
        if (iter != start) {
            length += delim_length;
        }
        length += IterDataSize(iter);
    }

    result->reserve(length);

    // Now combine everything.
    for (ITERATOR iter = start; iter != end; ++iter) {
        if (iter != start) {
            result->append(delim, delim_length);
        }
        result->append(IterData(iter));
    }
}

int format_string(string_ptr* out, size_t reserve_len, const char* fmt, ...);
int vformat_string(string_ptr* out, size_t reserve_len, const char* fmt, va_list ap);

#endif
