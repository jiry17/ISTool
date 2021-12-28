//
// Created by pro on 2021/12/4.
//

#include "istool/basic/time_guard.h"
#include <sys/time.h>

TimeGuard::TimeGuard(double _time_limit): time_limit(_time_limit) {
    gettimeofday(&start_time, NULL);
}

double TimeGuard::getPeriod() const {
    timeval now;
    gettimeofday(&now, NULL);
    return (now.tv_sec - start_time.tv_sec) + (now.tv_usec - start_time.tv_usec) / 1e6;
}

double TimeGuard::getRemainTime() const {
    return time_limit - getPeriod();
}

void TimeGuard::check() const {
    if (getRemainTime() < 0) throw TimeOutError();
}