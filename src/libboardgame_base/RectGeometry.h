//-----------------------------------------------------------------------------
/** @file libboardgame_base/RectGeometry.h
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifndef LIBBOARDGAME_BASE_RECT_GEOMETRY_H
#define LIBBOARDGAME_BASE_RECT_GEOMETRY_H

#include <map>
#include <memory>
#include "Geometry.h"
#include "libboardgame_util/Unused.h"

namespace libboardgame_base {

using namespace std;

//-----------------------------------------------------------------------------

/** Geometry of a regular rectangular grid.
    @tparam P An instantiation of libboardgame_base::Point */
template<class P>
class RectGeometry final
    : public Geometry<P>
{
public:
    typedef P Point;

    using AdjCoordList = typename Geometry<P>::AdjCoordList;
    using DiagCoordList = typename Geometry<P>::DiagCoordList;
    using AdjList = typename Geometry<P>::AdjList;
    using DiagList = typename Geometry<P>::DiagList;

    /** Create or reuse an already created geometry with a given size. */
    static const RectGeometry& get(unsigned width, unsigned height);

    RectGeometry(unsigned width, unsigned height);

    AdjCoordList get_adj_coord(int x, int y) const override;

    DiagCoordList get_diag_coord(int x, int y) const override;

    unsigned get_point_type(int x, int y) const override;

    unsigned get_period_x() const override;

    unsigned get_period_y() const override;

protected:
    bool init_is_onboard(unsigned x, unsigned y) const override;

private:
    /** Stores already created geometries by width and height. */
    static map<pair<unsigned, unsigned>, shared_ptr<RectGeometry>> s_geometry;
};

template<class P>
map<pair<unsigned, unsigned>, shared_ptr<RectGeometry<P>>>
    RectGeometry<P>::s_geometry;

template<class P>
RectGeometry<P>::RectGeometry(unsigned width, unsigned height)
{
    Geometry<P>::init(width, height);
}

template<class P>
const RectGeometry<P>& RectGeometry<P>::get(unsigned width, unsigned height)
{
    auto key = make_pair(width, height);
    auto pos = s_geometry.find(key);
    if (pos != s_geometry.end())
        return *pos->second;
    auto geometry = make_shared<RectGeometry>(width, height);
    return *s_geometry.insert(make_pair(key, geometry)).first->second;
}

template<class P>
auto RectGeometry<P>::get_adj_coord(int x, int y) const -> AdjCoordList
{
    AdjCoordList l;
    l.push_back(CoordPoint(x, y - 1));
    l.push_back(CoordPoint(x - 1, y));
    l.push_back(CoordPoint(x + 1, y));
    l.push_back(CoordPoint(x, y + 1));
    return l;
}

template<class P>
auto RectGeometry<P>::get_diag_coord(int x, int y) const -> DiagCoordList
{
    DiagCoordList l;
    l.push_back(CoordPoint(x - 1, y - 1));
    l.push_back(CoordPoint(x + 1, y - 1));
    l.push_back(CoordPoint(x - 1, y + 1));
    l.push_back(CoordPoint(x + 1, y + 1));
    return l;
}

template<class P>
unsigned RectGeometry<P>::get_period_x() const
{
    return 1;
}

template<class P>
unsigned RectGeometry<P>::get_period_y() const
{
    return 1;
}

template<class P>
unsigned RectGeometry<P>::get_point_type(int x, int y) const
{
    LIBBOARDGAME_UNUSED(x);
    LIBBOARDGAME_UNUSED(y);
    return 0;
}

template<class P>
bool RectGeometry<P>::init_is_onboard(unsigned x, unsigned y) const
{
    LIBBOARDGAME_UNUSED(x);
    LIBBOARDGAME_UNUSED(y);
    return true;
}

//-----------------------------------------------------------------------------

} // namespace libboardgame_base

#endif // LIBBOARDGAME_BASE_RECT_GEOMETRY_H
