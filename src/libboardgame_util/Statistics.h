//-----------------------------------------------------------------------------
/** @file libboardgame_util/Statistics.h
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifndef LIBBOARDGAME_UTIL_STATISTICS_H
#define LIBBOARDGAME_UTIL_STATISTICS_H

#include <atomic>
#include <cmath>
#include <iomanip>
#include <iosfwd>
#include <limits>
#include <sstream>
#include <string>
#include "Assert.h"
#include "FmtSaver.h"

namespace libboardgame_util {

using namespace std;

//-----------------------------------------------------------------------------

template<typename FLOAT = double>
class StatisticsBase
{
public:
    /** Constructor.
        @param init_val The value to return in get_mean() if count is 0. This
        value does not affect the mean returned if count is greater 0. */
    StatisticsBase(FLOAT init_val = 0);

    void add(FLOAT val);

    void clear(FLOAT init_val = 0);

    FLOAT get_count() const;

    FLOAT get_mean() const;

    void write(ostream& out, bool fixed = false,
               unsigned precision = 6) const;

private:
    FLOAT m_count;

    FLOAT m_mean;
};

template<typename FLOAT>
inline StatisticsBase<FLOAT>::StatisticsBase(FLOAT init_val)
{
    clear(init_val);
}

template<typename FLOAT>
void StatisticsBase<FLOAT>::add(FLOAT val)
{
    FLOAT count = m_count;
    ++count;
    val -= m_mean;
    m_mean +=  val / count;
    m_count = count;
}

template<typename FLOAT>
inline void StatisticsBase<FLOAT>::clear(FLOAT init_val)
{
    m_count = 0;
    m_mean = init_val;
}

template<typename FLOAT>
inline FLOAT StatisticsBase<FLOAT>::get_count() const
{
    return m_count;
}

template<typename FLOAT>
inline FLOAT StatisticsBase<FLOAT>::get_mean() const
{
    return m_mean;
}

template<typename FLOAT>
void StatisticsBase<FLOAT>::write(ostream& out, bool fixed,
                                  unsigned precision) const
{
    FmtSaver saver(out);
    if (fixed)
        out << std::fixed;
    out << setprecision(precision) << m_mean;
}

//----------------------------------------------------------------------------

template<typename FLOAT = double>
class Statistics
{
public:
    Statistics(FLOAT init_val = 0);

    void add(FLOAT val);

    void clear(FLOAT init_val = 0);

    FLOAT get_mean() const;

    FLOAT get_count() const;

    FLOAT get_deviation() const;

    FLOAT get_error() const;

    FLOAT get_variance() const;

    void write(ostream& out, bool fixed = false,
               unsigned precision = 6) const;

private:
    StatisticsBase<FLOAT> m_statistics_base;

    FLOAT m_variance;
};

template<typename FLOAT>
inline Statistics<FLOAT>::Statistics(FLOAT init_val)
{
    clear(init_val);
}

template<typename FLOAT>
void Statistics<FLOAT>::add(FLOAT val)
{
    if (get_count() > 0)
    {
        FLOAT count_old = get_count();
        FLOAT mean_old = get_mean();
        m_statistics_base.add(val);
        FLOAT mean = get_mean();
        FLOAT count = get_count();
        m_variance = (count_old * (m_variance + mean_old * mean_old)
                      + val * val) / count  - mean * mean;
    }
    else
    {
        m_statistics_base.add(val);
        m_variance = 0;
    }
}

template<typename FLOAT>
inline void Statistics<FLOAT>::clear(FLOAT init_val)
{
    m_statistics_base.clear(init_val);
    m_variance = 0;
}

template<typename FLOAT>
inline FLOAT Statistics<FLOAT>::get_count() const
{
    return m_statistics_base.get_count();
}

template<typename FLOAT>
inline FLOAT Statistics<FLOAT>::get_deviation() const
{
    // m_variance can become negative (due to rounding errors?)
    return (m_variance < 0 ? 0 : sqrt(m_variance));
}

template<typename FLOAT>
inline FLOAT Statistics<FLOAT>::get_error() const
{
    auto count = get_count();
    return count == 0 ? 0 : get_deviation() / sqrt(count);
}

template<typename FLOAT>
inline FLOAT Statistics<FLOAT>::get_mean() const
{
    return m_statistics_base.get_mean();
}

template<typename FLOAT>
inline FLOAT Statistics<FLOAT>::get_variance() const
{
    return m_variance;
}

template<typename FLOAT>
void Statistics<FLOAT>::write(ostream& out, bool fixed,
                              unsigned precision) const
{
    FmtSaver saver(out);
    if (fixed)
        out << std::fixed;
    out << setprecision(precision) << get_mean() << " dev="
        << get_deviation();
}

//----------------------------------------------------------------------------

template<typename FLOAT = double>
class StatisticsExt
{
public:
    StatisticsExt(FLOAT init_val = 0);

    void add(FLOAT val);

    void clear(FLOAT init_val = 0);

    FLOAT get_mean() const;

    FLOAT get_count() const;

    FLOAT get_max() const;

    FLOAT get_min() const;

    FLOAT get_deviation() const;

    FLOAT get_variance() const;

    void write(ostream& out, bool fixed = false, unsigned precision = 6,
               bool integer_values = false) const;

    string to_string(bool fixed = false, unsigned precision = 6,
                     bool integer_values = false) const;

private:
    Statistics<FLOAT> m_statistics;

    FLOAT m_max;

    FLOAT m_min;
};

template<typename FLOAT>
inline StatisticsExt<FLOAT>::StatisticsExt(FLOAT init_val)
{
    clear(init_val);
}

template<typename FLOAT>
void StatisticsExt<FLOAT>::add(FLOAT val)
{
    m_statistics.add(val);
    if (val > m_max)
        m_max = val;
    if (val < m_min)
        m_min = val;
}

template<typename FLOAT>
inline void StatisticsExt<FLOAT>::clear(FLOAT init_val)
{
    m_statistics.clear(init_val);
    m_min = numeric_limits<FLOAT>::max();
    m_max = -numeric_limits<FLOAT>::max();
}

template<typename FLOAT>
inline FLOAT StatisticsExt<FLOAT>::get_count() const
{
    return m_statistics.get_count();
}

template<typename FLOAT>
inline FLOAT StatisticsExt<FLOAT>::get_deviation() const
{
    return m_statistics.get_deviation();
}

template<typename FLOAT>
inline FLOAT StatisticsExt<FLOAT>::get_max() const
{
    return m_max;
}

template<typename FLOAT>
inline FLOAT StatisticsExt<FLOAT>::get_mean() const
{
    return m_statistics.get_mean();
}

template<typename FLOAT>
inline FLOAT StatisticsExt<FLOAT>::get_min() const
{
    return m_min;
}

template<typename FLOAT>
inline FLOAT StatisticsExt<FLOAT>::get_variance() const
{
    return m_statistics.get_variance();
}

template<typename FLOAT>
string StatisticsExt<FLOAT>::to_string(bool fixed, unsigned precision,
                                       bool integer_values) const
{
    ostringstream s;
    write(s, fixed, precision, integer_values);
    return s.str();
}

template<typename FLOAT>
void StatisticsExt<FLOAT>::write(ostream& out, bool fixed, unsigned precision,
                                 bool integer_values) const
{
    m_statistics.write(out, fixed, precision);
    FmtSaver saver(out);
    if (fixed)
        out << std::fixed;
    if (integer_values)
        out << setprecision(0);
    else
        out << setprecision(precision);
    out << " min=" << m_min << " max=" << m_max;
}

//----------------------------------------------------------------------------

/** Like StatisticsBase, but for lock-free multithreading with potentially
    lost updates.
    Updates and accesses of the moving average and the count are atomic but
    not synchronized and use memory_order_relaxed. Therefore, updates can be
    lost. Initializing via the constructor, operator= or clear() uses
    memory_order_seq_cst */
template<typename FLOAT = double>
class StatisticsDirtyLockFree
{
public:
    /** Constructor.
        @param init_val See StatisticBase::StatisticBase() */
    StatisticsDirtyLockFree(FLOAT init_val = 0);

    StatisticsDirtyLockFree& operator=(const StatisticsDirtyLockFree& s);

    void add(FLOAT val);

    void clear(FLOAT init_val = 0);

    FLOAT get_count() const;

    FLOAT get_mean() const;

    void write(ostream& out, bool fixed = false,
               unsigned precision = 6) const;

private:
    atomic<FLOAT> m_count;

    atomic<FLOAT> m_mean;
};

template<typename FLOAT>
inline StatisticsDirtyLockFree<FLOAT>::StatisticsDirtyLockFree(FLOAT init_val)
{
    clear(init_val);
}

template<typename FLOAT>
StatisticsDirtyLockFree<FLOAT>&
StatisticsDirtyLockFree<FLOAT>::operator=(const StatisticsDirtyLockFree& s)
{
    m_count = s.m_count.load();
    m_mean = s.m_mean.load();
    return *this;
}

template<typename FLOAT>
void StatisticsDirtyLockFree<FLOAT>::add(FLOAT val)
{
    FLOAT count = m_count.load(memory_order_relaxed);
    FLOAT mean = m_mean.load(memory_order_relaxed);
    ++count;
    mean +=  (val - mean) / count;
    m_mean.store(mean, memory_order_relaxed);
    m_count.store(count, memory_order_relaxed);
}

template<typename FLOAT>
inline void StatisticsDirtyLockFree<FLOAT>::clear(FLOAT init_val)
{
    m_count = 0;
    m_mean = init_val;
}

template<typename FLOAT>
inline FLOAT StatisticsDirtyLockFree<FLOAT>::get_count() const
{
    return m_count.load(memory_order_relaxed);
}

template<typename FLOAT>
inline FLOAT StatisticsDirtyLockFree<FLOAT>::get_mean() const
{
    return m_mean.load(memory_order_relaxed);
}

template<typename FLOAT>
void StatisticsDirtyLockFree<FLOAT>::write(ostream& out, bool fixed,
                                           unsigned precision) const
{
    FmtSaver saver(out);
    if (fixed)
        out << std::fixed;
    out << setprecision(precision) << get_mean();
}

//----------------------------------------------------------------------------

template<typename FLOAT>
inline ostream& operator<<(ostream& out, const StatisticsExt<FLOAT>& s)
{
    s.write(out);
    return out;
}

//----------------------------------------------------------------------------

} // namespace libboardgame_util

#endif // LIBBOARDGAME_UTIL_STATISTICS_H
