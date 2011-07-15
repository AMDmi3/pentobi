//-----------------------------------------------------------------------------
/** @file GeometryTest.cpp */
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "libboardgame_base/ChessPointStringRep.h"
#include "libboardgame_base/Geometry.h"
#include "libboardgame_base/PointList.h"
#include "libboardgame_test/Test.h"

using namespace std;
using libboardgame_base::ChessPointStringRep;
using libboardgame_base::NullTermList;

//-----------------------------------------------------------------------------

typedef libboardgame_base::Point<19, ChessPointStringRep> Point;
typedef libboardgame_base::Geometry<Point> Geometry;
typedef libboardgame_base::PointList<Point> PointList;

//-----------------------------------------------------------------------------

LIBBOARDGAME_TEST_CASE(boardgame_geometry_get_adj_diag)
{
    const Geometry* g = Geometry::get(9);
    PointList l;
    for (NullTermList<Point, 8>::Iterator i(g->get_adj_diag(Point("B9")));
         i; ++i)
        l.push_back(*i);
    LIBBOARDGAME_CHECK_EQUAL(l.size(), 5u);
    LIBBOARDGAME_CHECK(l.contains(Point("A9")));
    LIBBOARDGAME_CHECK(l.contains(Point("C9")));
    LIBBOARDGAME_CHECK(l.contains(Point("A8")));
    LIBBOARDGAME_CHECK(l.contains(Point("B8")));
    LIBBOARDGAME_CHECK(l.contains(Point("C8")));
}

LIBBOARDGAME_TEST_CASE(boardgame_geometry_iterate)
{
    const Geometry* g = Geometry::get(3);
    Geometry::Iterator i(*g);
    LIBBOARDGAME_CHECK(i);
    LIBBOARDGAME_CHECK_EQUAL(Point(0, 0), *i);
    ++i;
    LIBBOARDGAME_CHECK(i);
    LIBBOARDGAME_CHECK_EQUAL(Point(1, 0), *i);
    ++i;
    LIBBOARDGAME_CHECK(i);
    LIBBOARDGAME_CHECK_EQUAL(Point(2, 0), *i);
    ++i;
    LIBBOARDGAME_CHECK(i);
    LIBBOARDGAME_CHECK_EQUAL(Point(0, 1), *i);
    ++i;
    LIBBOARDGAME_CHECK(i);
    LIBBOARDGAME_CHECK_EQUAL(Point(1, 1), *i);
    ++i;
    LIBBOARDGAME_CHECK(i);
    LIBBOARDGAME_CHECK_EQUAL(Point(2, 1), *i);
    ++i;
    LIBBOARDGAME_CHECK(i);
    LIBBOARDGAME_CHECK_EQUAL(Point(0, 2), *i);
    ++i;
    LIBBOARDGAME_CHECK(i);
    LIBBOARDGAME_CHECK_EQUAL(Point(1, 2), *i);
    ++i;
    LIBBOARDGAME_CHECK(i);
    LIBBOARDGAME_CHECK_EQUAL(Point(2, 2), *i);
    ++i;
    LIBBOARDGAME_CHECK(! i);
}

LIBBOARDGAME_TEST_CASE(boardgame_geometry_dist_to_edge)
{
    const Geometry* g = Geometry::get(9);
    LIBBOARDGAME_CHECK_EQUAL(g->get_dist_to_edge(Point(3, 0)), 0u);
    LIBBOARDGAME_CHECK_EQUAL(g->get_dist_to_edge(Point(3, 2)), 2u);
    LIBBOARDGAME_CHECK_EQUAL(g->get_dist_to_edge(Point(6, 8)), 0u);
    LIBBOARDGAME_CHECK_EQUAL(g->get_dist_to_edge(Point(6, 5)), 2u);
}

LIBBOARDGAME_TEST_CASE(boardgame_geometry_second_dist_to_edge)
{
    const Geometry* g = Geometry::get(9);
    LIBBOARDGAME_CHECK_EQUAL(g->get_second_dist_to_edge(Point(3, 0)), 3u);
    LIBBOARDGAME_CHECK_EQUAL(g->get_second_dist_to_edge(Point(3, 2)), 3u);
    LIBBOARDGAME_CHECK_EQUAL(g->get_second_dist_to_edge(Point(6, 8)), 2u);
    LIBBOARDGAME_CHECK_EQUAL(g->get_second_dist_to_edge(Point(6, 5)), 3u);
}

//-----------------------------------------------------------------------------
