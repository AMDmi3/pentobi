//-----------------------------------------------------------------------------
/** @file libpentobi_base/BoardUtil.h
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifndef LIBPENTOBI_BASE_BOARDUTIL_H
#define LIBPENTOBI_BASE_BOARDUTIL_H

#include "Board.h"
#include "libboardgame_sgf/Writer.h"

namespace libpentobi_base {
namespace boardutil {

using libboardgame_sgf::Writer;

//-----------------------------------------------------------------------------

#if LIBBOARDGAME_DEBUG
string dump(const Board& bd);
#endif

/** Return the current position as setup.
    Merges all placements from Board::get_setup() and played moved into a
    single setup and sets the setup color to play to the current color to
    play. */
void get_current_position_as_setup(const Board& bd, Setup& setup);

void write_setup(Writer& writer, Variant variant, const Setup& setup);

//-----------------------------------------------------------------------------

} // namespace boardutil
} // namespace libpentobi_base

#endif // LIBPENTOBI_BASE_BOARDUTIL_H
