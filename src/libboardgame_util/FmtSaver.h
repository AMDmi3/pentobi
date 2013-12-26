//----------------------------------------------------------------------------
/** @file libboardgame_util/FmtSaver.h
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//----------------------------------------------------------------------------

#ifndef LIBBOARDGAME_UTIL_FMT_SAVER_H
#define LIBBOARDGAME_UTIL_FMT_SAVER_H

#include <iostream>
#include <fstream>

namespace libboardgame_util {

using namespace std;

//-----------------------------------------------------------------------------

/** Saves the formatting state of a stream and restores it in its
    destructor. */
class FmtSaver
{
public:
    FmtSaver(ostream& out)
        : m_out(out)
    {
        m_dummy.copyfmt(out);
    }

    ~FmtSaver()
    {
        m_out.copyfmt(m_dummy);
    }

private:
    ostream& m_out;

    fstream m_dummy;
};

//----------------------------------------------------------------------------

} // namespace libboardgame_util

#endif // LIBBOARDGAME_UTIL_FMT_SAVER_H
