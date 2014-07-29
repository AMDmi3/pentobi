//-----------------------------------------------------------------------------
/** @file unittest/libboardgame_util/StatisticsTest.cpp
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "libboardgame_util/Statistics.h"
#include "libboardgame_test/Test.h"

using namespace std;
using namespace libboardgame_util;

//-----------------------------------------------------------------------------

LIBBOARDGAME_TEST_CASE(libboardgame_util_statistics_basic)
{
    Statistics<double> s;
    s.add(12);
    s.add(11);
    s.add(14);
    s.add(16);
    s.add(15);
    LIBBOARDGAME_CHECK_EQUAL(s.get_count(), 5.);
    LIBBOARDGAME_CHECK_CLOSE_EPS(s.get_mean(), 13.6, 1e-6);
    LIBBOARDGAME_CHECK_CLOSE_EPS(s.get_variance(), 3.44, 1e-6);
    LIBBOARDGAME_CHECK_CLOSE_EPS(s.get_deviation(), 1.854723, 1e-6);
}

//-----------------------------------------------------------------------------
