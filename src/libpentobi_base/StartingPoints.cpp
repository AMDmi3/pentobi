//-----------------------------------------------------------------------------
/** @file libpentobi_base/StartingPoints.cpp */
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "StartingPoints.h"

namespace libpentobi_base {

using namespace std;

//-----------------------------------------------------------------------------

void StartingPoints::add_colored_starting_point(unsigned int x, unsigned int y,
                                                Color c)
{
    Point p(x, y);
    m_is_colored_starting_point[p] = true;
    m_starting_point_color[p] = c;
    m_starting_points[c].push_back(p);
}

void StartingPoints::add_colorless_starting_point(unsigned int x,
                                                  unsigned int y)
{
    Point p(x, y);
    m_is_colorless_starting_point[p] = true;
    m_starting_points[Color(0)].push_back(p);
    m_starting_points[Color(1)].push_back(p);
    m_starting_points[Color(2)].push_back(p);
    m_starting_points[Color(3)].push_back(p);
}

void StartingPoints::init(Variant variant, const Geometry& geometry)
{
    m_is_colored_starting_point.init(geometry, false);
    m_is_colorless_starting_point.init(geometry, false);
    m_starting_point_color.init(geometry);
    m_starting_points[Color(0)].clear();
    m_starting_points[Color(1)].clear();
    m_starting_points[Color(2)].clear();
    m_starting_points[Color(3)].clear();
    if (variant == variant_classic || variant == variant_classic_2)
    {
        add_colored_starting_point(0, 19, Color(0));
        add_colored_starting_point(19, 19, Color(1));
        add_colored_starting_point(19, 0, Color(2));
        add_colored_starting_point(0, 0, Color(3));
    }
    else if (variant == variant_duo || variant == variant_junior)
    {
        add_colored_starting_point(4, 9, Color(0));
        add_colored_starting_point(9, 4, Color(1));
    }
    else if (variant == variant_trigon || variant == variant_trigon_2)
    {
        add_colorless_starting_point(17, 3);
        add_colorless_starting_point(17, 14);
        add_colorless_starting_point(9, 6);
        add_colorless_starting_point(9, 11);
        add_colorless_starting_point(25, 6);
        add_colorless_starting_point(25, 11);
    }
    else if (variant == variant_trigon_3)
    {
        add_colorless_starting_point(15, 2);
        add_colorless_starting_point(15, 13);
        add_colorless_starting_point(7, 5);
        add_colorless_starting_point(7, 10);
        add_colorless_starting_point(23, 5);
        add_colorless_starting_point(23, 10);
    }
    else
        LIBBOARDGAME_ASSERT(false);
}

//-----------------------------------------------------------------------------

} // namespace libpentobi_base
