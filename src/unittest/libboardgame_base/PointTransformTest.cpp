//-----------------------------------------------------------------------------
/** @file unittest/libboardgame_base/PointTransformTest.cpp
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "libboardgame_base/PointTransform.h"
#include "libboardgame_base/RectGeometry.h"
#include "libboardgame_base/SpShtStrRep.h"
#include "libboardgame_test/Test.h"

using namespace std;
using libboardgame_base::SpShtStrRep;

//-----------------------------------------------------------------------------

typedef
libboardgame_base::Point<19 * 19, 19, 19, unsigned short, SpShtStrRep> Point;
typedef libboardgame_base::RectGeometry<Point> RectGeometry;

//-----------------------------------------------------------------------------

LIBBOARDGAME_TEST_CASE(boardgame_point_transform_get_transformed)
{
    unsigned sz = 9;
    auto& geo = RectGeometry::get(sz, sz);
    Point p = geo.get_point(1, 2);
    {
        libboardgame_base::PointTransfIdent<Point> transform;
        LIBBOARDGAME_CHECK(transform.get_transformed(p, geo) == p);
    }
    {
        libboardgame_base::PointTransfRot180<Point> transform;
        LIBBOARDGAME_CHECK(transform.get_transformed(p, geo)
                           == geo.get_point(7, 6));
    }
    {
        libboardgame_base::PointTransfRot270Refl<Point> transform;
        LIBBOARDGAME_CHECK(transform.get_transformed(p, geo)
                           == geo.get_point(2, 1));
    }
}

//-----------------------------------------------------------------------------
