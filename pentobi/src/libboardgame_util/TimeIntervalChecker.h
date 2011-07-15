//-----------------------------------------------------------------------------
/** @file TimeIntervalChecker.h */
//-----------------------------------------------------------------------------

#ifndef LIBBOARDGAME_UTIL_TIME_INTERVAL_CHECKER_H
#define LIBBOARDGAME_UTIL_TIME_INTERVAL_CHECKER_H

#include "IntervalChecker.h"

namespace libboardgame_util {

//-----------------------------------------------------------------------------

class TimeIntervalChecker
    : public IntervalChecker
{
public:
    TimeIntervalChecker(TimeSource& time_source, double time_interval,
                        double max_time);

    /** Constructor with automatically set time_interval.
        The time interval will be set to 0.1, if max_time > 1, otherwise
        to 0.1 * max_time */
    TimeIntervalChecker(TimeSource& time_source, double max_time);

private:
    double m_max_time;

    double m_last_time;

    bool check_time();
};

//-----------------------------------------------------------------------------

} // namespace libboardgame_util

#endif // LIBBOARDGAME_UTIL_TIME_INTERVAL_CHECKER_H
