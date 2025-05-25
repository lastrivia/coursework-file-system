#ifndef MISC_H
#define MISC_H

#include <cstdint>

namespace cs2313 {

    inline int power_of_2(uint64_t n) {
        if (n > 0 && (n & (n - 1)) == 0) {
#if __has_builtin(__builtin_ctzll)
            return __builtin_ctzll(n);
#else
            int ret = 0;
            while (n > 1) {
                n >>= 1;
                ++ret;
            }
            return ret;
#endif
        }
        return -1; // not power of 2
    }

    inline bool is_uint(const char *s) {
        for (; *s; ++s)
            if (*s < '0' || *s > '9')
                return false;
        return true;
    };
}

#endif
