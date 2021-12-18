//
// Created by pro on 2021/12/4.
//

#include "time_guard.h"
#include <sys/time.h>

TimeGuard::TimeGuard(double _time_limit): time_limit(_time_limit) {
    gettimeofday(&start_time, NULL);
}

void TimeGuard::check() const {
    timeval now;
    gettimeofday(&now, NULL);
    double period = (now.tv_sec - start_time.tv_sec) + (now.tv_usec - start_time.tv_usec) / 1e6;
    if (period > time_limit) throw TimeOutError();
}