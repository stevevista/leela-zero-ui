/*
    This file is part of Leela Zero.
    Copyright (C) 2017 Gian-Carlo Pascutto

    Leela Zero is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Leela Zero is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Leela Zero.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"
#include "Utils.h"

#include <mutex>
#include <cstdarg>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/select.h>
#endif

#include "GTP.h"

Utils::ThreadPool thread_pool;

bool Utils::input_pending(void) {
    return GTP::input_pending();
}

static std::mutex IOmutex;

void Utils::myprintf(const char *fmt, ...) {
    if (cfg_quiet) {
        return;
    }
    va_list ap;
    va_start(ap, fmt);
    
    if (!GTP::vstderr(fmt, ap))
        vfprintf(stderr, fmt, ap);

    va_end(ap);

    if (cfg_logfile_handle) {
        std::lock_guard<std::mutex> lock(IOmutex);
        va_start(ap, fmt);
        vfprintf(cfg_logfile_handle, fmt, ap);
        va_end(ap);
    }
}


size_t Utils::ceilMultiple(size_t a, size_t b) {
    if (a % b == 0) {
        return a;
    }

    auto ret = a + (b - a % b);
    return ret;
}
