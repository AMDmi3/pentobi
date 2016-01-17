//-----------------------------------------------------------------------------
/** @file libpentobi_base/StartingPoints.cpp
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "StartingPoints.h"

namespace libpentobi_base {

//-----------------------------------------------------------------------------

void StartingPoints::add_colored_starting_point(const Geometry& geo,
                                                unsigned x, unsigned y,
                                                Color c)
{
    Point p = geo.get_point(x, y);
    m_is_colored_starting_point[p] = true;
    m_starting_point_color[p] = c;
    m_starting_points[c].push_back(p);
}

void StartingPoints::add_colorless_starting_point(const Geometry& geo,
                                                  unsigned x, unsigned y)
{
    Point p = geo.get_point(x, y);
    m_is_colorless_starting_point[p] = true;
    for_each_color([&](Color c) {
        m_starting_points[c].push_back(p);
    });
}

void StartingPoints::init(Variant variant, const Geometry& geo)
{
    m_is_colored_starting_point.fill(false, geo);
    m_is_colorless_starting_point.fill(false, geo);
    for_each_color([&](Color c) {
        m_starting_points[c].clear();
    });
    switch (get_board_type(variant))
    {
    case BoardType::classic:
        add_colored_starting_point(geo, 0, 0, Color(0));
        add_colored_starting_point(geo, 19, 0, Color(1));
        add_colored_starting_point(geo, 19, 19, Color(2));
        add_colored_starting_point(geo, 0, 19, Color(3));
        break;
    case BoardType::duo:
        add_colored_starting_point(geo, 4, 4, Color(0));
        add_colored_starting_point(geo, 9, 9, Color(1));
        break;
    case BoardType::trigon:
        add_colorless_starting_point(geo, 17, 3);
        add_colorless_starting_point(geo, 17, 14);
        add_colorless_starting_point(geo, 9, 6);
        add_colorless_starting_point(geo, 9, 11);
        add_colorless_starting_point(geo, 25, 6);
        add_colorless_starting_point(geo, 25, 11);
        break;
    case BoardType::trigon_3:
        add_colorless_starting_point(geo, 15, 2);
        add_colorless_starting_point(geo, 15, 13);
        add_colorless_starting_point(geo, 7, 5);
        add_colorless_starting_point(geo, 7, 10);
        add_colorless_starting_point(geo, 23, 5);
        add_colorless_starting_point(geo, 23, 10);
        break;
    case BoardType::nexos:
        add_colored_starting_point(geo, 4, 3, Color(0));
        add_colored_starting_point(geo, 3, 4, Color(0));
        add_colored_starting_point(geo, 5, 4, Color(0));
        add_colored_starting_point(geo, 4, 5, Color(0));
        add_colored_starting_point(geo, 20, 3, Color(1));
        add_colored_starting_point(geo, 19, 4, Color(1));
        add_colored_starting_point(geo, 21, 4, Color(1));
        add_colored_starting_point(geo, 20, 5, Color(1));
        add_colored_starting_point(geo, 20, 19, Color(2));
        add_colored_starting_point(geo, 19, 20, Color(2));
        add_colored_starting_point(geo, 21, 20, Color(2));
        add_colored_starting_point(geo, 20, 21, Color(2));
        add_colored_starting_point(geo, 4, 19, Color(3));
        add_colored_starting_point(geo, 3, 20, Color(3));
        add_colored_starting_point(geo, 5, 20, Color(3));
        add_colored_starting_point(geo, 4, 21, Color(3));
        break;
    case BoardType::callisto:
    case BoardType::callisto_2:
    case BoardType::callisto_3:
        break;
    }
}

//-----------------------------------------------------------------------------

} // namespace libpentobi_base
