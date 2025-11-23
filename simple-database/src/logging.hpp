#ifndef _LOGGING_HPP_
#define _LOGGING_HPP_

#define LOG(MSG) ((void) 0)

namespace logging {
    template <typename T> void Print(T i) {
        (void) i;
    };
};

#endif //_LOGGING_HPP_
