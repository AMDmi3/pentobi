//-----------------------------------------------------------------------------
/** @file libpentobi_base/PointList.h
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifndef LIBPENTOBI_BASE_POINT_LIST_H
#define LIBPENTOBI_BASE_POINT_LIST_H

#include "Point.h"
#include "libboardgame_util/ArrayList.h"

namespace libpentobi_base {

//-----------------------------------------------------------------------------

using PointList = ArrayList<Point, Point::range_onboard>;

//-----------------------------------------------------------------------------

} // namespace libpentobi_base

#endif // LIBPENTOBI_BASE_POINT_LIST_H
