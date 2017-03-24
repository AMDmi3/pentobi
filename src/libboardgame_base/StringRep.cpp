//-----------------------------------------------------------------------------
/** @file libboardgame_base/StringRep.cpp
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "StringRep.h"

#include <cstdio>
#include <iostream>
#include "libboardgame_util/StringUtil.h"
#include "libboardgame_util/Unused.h"

namespace libboardgame_base {

using libboardgame_util::get_letter_coord;

//-----------------------------------------------------------------------------

StringRep::~StringRep() = default;

//-----------------------------------------------------------------------------

StdStringRep::~StdStringRep() = default;

bool StdStringRep::read(string::const_iterator begin,
                        string::const_iterator end, unsigned width,
                        unsigned height, unsigned& x, unsigned& y) const
{
    auto p = begin;
    while (p != end && isspace(*p))
        ++p;
    bool read_x = false;
    x = 0;
    int c;
    while (p != end && isalpha(*p))
    {
        c = tolower(*(p++));
        if (c < 'a' || c > 'z')
            return false;
        x = 26 * x + (c - 'a' + 1);
        if (x > width)
            return false;
        read_x = true;
    }
    if (! read_x)
        return false;
    --x;
    bool read_y = false;
    y = 0;
    while (p != end && isdigit(*p))
    {
        c = *(p++);
        y = 10 * y + (c - '0');
        if (y > height)
            return false;
        read_y = true;
    }
    if (! read_y)
        return false;
    y = height - y;
    while (p != end)
        if (! isspace(*(p++)))
            return false;
    return true;
}

void StdStringRep::write(ostream& out, unsigned x, unsigned y, unsigned width,
                         unsigned height) const
{
    LIBBOARDGAME_UNUSED(width);
    out << get_letter_coord(x) << (height - y);
}

//-----------------------------------------------------------------------------

} // namespace libboardgame_base
