//-----------------------------------------------------------------------------
/** @file libpentobi_base/CallistoGeometry.h
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifndef LIBPENTOBI_BASE_CALLISTO_GEOMETRY_H
#define LIBPENTOBI_BASE_CALLISTO_GEOMETRY_H

#include <map>
#include <memory>
#include "Geometry.h"

namespace libpentobi_base {

using namespace std;

//-----------------------------------------------------------------------------

/** Geometry for the board game Callisto.
    To fit in with the assumptions of the Blokus engine, points are "diagonal"
    to each other if they are actually adjacent on the real board and the
    "adjacent" relationship is not used. */
class CallistoGeometry final
    : public Geometry
{
public:
    /** Create or reuse an already created geometry.
        @param nu_colors The number of colors (2, 3, or 4). */
    static const CallistoGeometry& get(unsigned nu_colors);

    static bool is_center_section(unsigned x, unsigned y, unsigned nu_colors);


    AdjCoordList get_adj_coord(int x, int y) const override;

    DiagCoordList get_diag_coord(int x, int y) const override;

    unsigned get_point_type(int x, int y) const override;

    unsigned get_period_x() const override;

    unsigned get_period_y() const override;

protected:
    bool init_is_onboard(unsigned x, unsigned y) const override;

private:
    /** Stores already created geometries by number of colors. */
    static map<unsigned, shared_ptr<CallistoGeometry>> s_geometry;


    unsigned m_edge;


    explicit CallistoGeometry(unsigned nu_colors);
};

//-----------------------------------------------------------------------------

} // namespace libpentobi_base

#endif // LIBPENTOBI_BASE_CALLISTO_GEOMETRY_H
