//-----------------------------------------------------------------------------
/** @file libpentobi_base/Variant.h
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifndef LIBPENTOBI_BASE_VARIANT_H
#define LIBPENTOBI_BASE_VARIANT_H

#include <string>
#include "Color.h"
#include "Geometry.h"
#include "libboardgame_base/PointTransform.h"

namespace libpentobi_base {

using libboardgame_base::PointTransform;

//-----------------------------------------------------------------------------

enum class PieceSet
{
    classic,

    junior,

    trigon
};

//-----------------------------------------------------------------------------

enum class BoardType
{
    classic,

    duo,

    trigon,

    trigon_3
};

//-----------------------------------------------------------------------------

/** Game variant. */
enum class Variant
{
    classic,

    classic_2,

    classic_3,

    duo,

    junior,

    trigon,

    trigon_2,

    trigon_3
};

//-----------------------------------------------------------------------------

/** Get name of game variant as in the GM property in Blokus SGF files. */
const char* to_string(Variant variant);

/** Get a short lowercase string without spaces that can be used as
    a identifier for a game variant.
    The strings used are "classic", "classic_2", "duo", "trigon", "trigon_2",
    "trigon_3", "junior" */
const char* to_string_id(Variant variant);

/** Parse name of game variant as in the GM property in Blokus SGF files.
    The parsing is case-insensitive, leading and trailing whitespaced are
    ignored.
    @param s
    @param[out] variant
    @result True if the string contained a valid game variant. */
bool parse_variant(const string& s, Variant& variant);

/** Parse short lowercase name of game variant as returned to_string_id().
    @param s
    @param[out] variant
    @result True if the string contained a valid game variant. */
bool parse_variant_id(const string& s, Variant& variant);

Color::IntType get_nu_colors(Variant variant);

inline Color::Range get_colors(Variant variant)
{
    return Color::Range(get_nu_colors(variant));
}

Color::IntType get_nu_players(Variant variant);

const Geometry& get_geometry(BoardType board_type);

const Geometry& get_geometry(Variant variant);

BoardType get_board_type(Variant variant);

PieceSet get_piece_set(Variant variant);

/** Get invariance transformations for a game variant.
    The invariance transformations depend on the symmetry of the board type and
    the starting points.
    @param variant The game variant.
    @param[out] transforms The invariance transformations.
    @param[out] inv_transforms The inverse transformations of the elements in
    transforms. */
void get_transforms(Variant variant,
                    vector<unique_ptr<PointTransform<Point>>>& transforms,
                    vector<unique_ptr<PointTransform<Point>>>& inv_transforms);

//-----------------------------------------------------------------------------

} // namespace libpentobi_base

#endif // LIBPENTOBI_BASE_VARIANT_H
