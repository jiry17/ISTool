//
// Created by pro on 2021/12/4.
//

#ifndef ISTOOL_TIME_GUARD_H
#define ISTOOL_TIME_GUARD_H

#include <ctime>
#include <exception>

struct TimeOutError: public std::exception {
};

class TimeGuard {
public:
    timeval start_time;
    double time_limit;
    TimeGuard(double _time_limit);
    void check() const;
    double getRemainTime() const;
    double getPeriod() const;
    ~TimeGuard() = default;
};

#define TimeCheck(g) if (g) g->check()


#endif //ISTOOL_TIME_GUARD_H
