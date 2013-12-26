//-----------------------------------------------------------------------------
/** @file libpentobi_base/Setup.h
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifndef LIBPENTOBI_BASE_SETUP_H
#define LIBPENTOBI_BASE_SETUP_H

#include "ColorMap.h"
#include "Move.h"

namespace libpentobi_base {

//-----------------------------------------------------------------------------

/** Definition of a setup position.
    A setup position consists of a number of pieces that are placed at once
    (in no particular order) on the board and a color to play next. */
struct Setup
{
    /** Maximum number of pieces on board per color. */
    static const unsigned max_pieces = 24;

    typedef ArrayList<Move,max_pieces> PlacementList;

    Color to_play;

    ColorMap<PlacementList> placements;

    Setup();

    void clear();
};

inline Setup::Setup()
    : to_play(Color(0))
{
}

inline void Setup::clear()
{
    to_play = Color(0);
    LIBPENTOBI_FOREACH_COLOR(c, placements[c].clear());
}

//-----------------------------------------------------------------------------

} // namespace libpentobi_base

#endif // LIBPENTOBI_BASE_SETUP_H
