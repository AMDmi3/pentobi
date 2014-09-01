//-----------------------------------------------------------------------------
/** @file libboardgame_util/CpuTimeSource.cpp
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "CpuTimeSource.h"

#include "libboardgame_sys/CpuTime.h"

namespace libboardgame_util {

//-----------------------------------------------------------------------------

double CpuTimeSource::operator()()
{
    return libboardgame_sys::cpu_time();
}

//----------------------------------------------------------------------------

} // namespace libboardgame_util
