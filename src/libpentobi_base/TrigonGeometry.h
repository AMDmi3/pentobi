//-----------------------------------------------------------------------------
/** @file libpentobi_base/TrigonGeometry.h
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifndef LIBPENTOBI_BASE_TRIGON_GEOMETRY_H
#define LIBPENTOBI_BASE_TRIGON_GEOMETRY_H

#include <map>
#include <memory>
#include "Geometry.h"

namespace libpentobi_base {

using namespace std;

//-----------------------------------------------------------------------------

/** Geometry as used in the game Blokus Trigon.
    The board is a hexagon consisting of triangles. The coordinates are like
    in this example of a hexagon with edge size 3:
    <tt>
       0 1 2 3 4 5 6 7 8 9 10
    0     / \ / \ / \ / \
    1   / \ / \ / \ / \ / \
    2 / \ / \ / \ / \ / \ / \
    3 \ / \ / \ / \ / \ / \ /
    4   \ / \ / \ / \ / \ /
    5     \ / \ / \ / \ /
    </tt>
    There are two point types: 0=upward triangle, 1=downward triangle.
    @tparam P An instantiation of libboardgame_base::Point */
class TrigonGeometry final
    : public Geometry
{
public:
    using AdjList = Geometry::AdjList;

    using DiagList = Geometry::DiagList;

    /** Create or reuse an already created geometry with a given size.
        @param sz The edge size of the hexagon. */
    static const TrigonGeometry& get(unsigned sz);

    unsigned get_point_type(int x, int y) const override;

    unsigned get_period_x() const override;

    unsigned get_period_y() const override;

protected:
    bool init_is_onboard(unsigned x, unsigned y) const override;

    void init_adj_diag(Point p, AdjList& adj, DiagList& diag) const override;

private:
    /** Stores already created geometries by size. */
    static map<unsigned, shared_ptr<TrigonGeometry>> s_geometry;

    unsigned m_sz;

    TrigonGeometry(unsigned size);
};

//-----------------------------------------------------------------------------

} // namespace libpentobi_base

#endif // LIBPENTOBI_BASE_TRIGON_GEOMETRY_H
