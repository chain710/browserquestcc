#include "string_helper.h"
#include <stdarg.h>
#include <stdio.h>

int format_string( string_ptr* out, size_t reserve_len, const char* fmt, ... )
{
    va_list ap;
    va_start(ap, fmt);
    int ret = vformat_string(out, reserve_len, fmt, ap);
    va_end(ap);

    return ret;
}

int vformat_string( string_ptr* out, size_t reserve_len, const char* fmt, va_list ap )
{
    if (NULL == out || NULL == fmt)
    {
        return -1;
    }

    va_list calc_ap;
    va_copy(calc_ap, ap);
    int str_len = vsnprintf(NULL, 0, fmt, calc_ap);
    va_end(calc_ap);
    if (str_len < 0)
    {
        return str_len;
    }

    size_t buf_size = reserve_len + str_len + 1;
    if (out->get_size() < buf_size)
    {
        out->reset(new char[buf_size], buf_size);
        if (NULL == out->get())
        {
            return -1;
        }
    }
    
    vsnprintf(out->get()+reserve_len, buf_size-reserve_len, fmt, ap);
    return str_len;
}


