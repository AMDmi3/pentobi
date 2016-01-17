//-----------------------------------------------------------------------------
/** @file libpentobi_mcts/SharedConst.cpp
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "SharedConst.h"

namespace libpentobi_mcts {

using libboardgame_base::PointTransfRot180;
using libpentobi_base::BoardConst;
using libpentobi_base::BoardType;
using libpentobi_base::Move;
using libpentobi_base::Piece;
using libpentobi_base::PieceSet;

//-----------------------------------------------------------------------------

namespace {

void filter_min_size(const BoardConst& bc, unsigned min_size,
                     PieceMap<bool>& is_piece_considered)
{
    for (Piece::IntType i = 0; i < bc.get_nu_pieces(); ++i)
    {
        Piece piece(i);
        auto& piece_info = bc.get_piece_info(piece);
        if (piece_info.get_score_points() < min_size)
            is_piece_considered[piece] = false;
    }
}

/** Check if an adjacent status is a possible follow-up status for another
    one. */
inline bool is_followup_adj_status(unsigned status_new, unsigned status_old)
{
    return (status_new & status_old) == status_old;
}

void set_piece_considered(const BoardConst& bc, const char* name,
                          PieceMap<bool>& is_piece_considered,
                          bool is_considered = true)
{
    Piece piece;
    bool found = bc.get_piece_by_name(name, piece);
    LIBBOARDGAME_UNUSED_IF_NOT_DEBUG(found);
    LIBBOARDGAME_ASSERT(found);
    is_piece_considered[piece] = is_considered;
}

void set_pieces_considered(const Board& bd, unsigned nu_moves,
                           PieceMap<bool>& is_piece_considered)
{
    auto& bc = bd.get_board_const();
    unsigned nu_colors = bd.get_nu_colors();
    is_piece_considered.fill(true);
    switch (bc.get_board_type())
    {
    case BoardType::duo:
        if (nu_moves < 2 * nu_colors)
            filter_min_size(bc, 5, is_piece_considered);
        else if (nu_moves < 3 * nu_colors)
            filter_min_size(bc, 4, is_piece_considered);
        else if (nu_moves < 5 * nu_colors)
            filter_min_size(bc, 3, is_piece_considered);
        break;
    case BoardType::classic:
        if (nu_moves < nu_colors)
        {
            is_piece_considered.fill(false);
            set_piece_considered(bc, "V5", is_piece_considered);
            set_piece_considered(bc, "Z5", is_piece_considered);
        }
        else if (nu_moves < 2 * nu_colors)
        {
            filter_min_size(bc, 5, is_piece_considered);
            set_piece_considered(bc, "F", is_piece_considered, false);
            set_piece_considered(bc, "P", is_piece_considered, false);
            set_piece_considered(bc, "T5", is_piece_considered, false);
            set_piece_considered(bc, "U", is_piece_considered, false);
            set_piece_considered(bc, "X", is_piece_considered, false);
        }
        else if (nu_moves < 3 * nu_colors)
        {
            filter_min_size(bc, 5, is_piece_considered);
            set_piece_considered(bc, "P", is_piece_considered, false);
            set_piece_considered(bc, "U", is_piece_considered, false);
        }
        else if (nu_moves < 5 * nu_colors)
            filter_min_size(bc, 4, is_piece_considered);
        else if (nu_moves < 7 * nu_colors)
            filter_min_size(bc, 3, is_piece_considered);
        break;
    case BoardType::trigon:
    case BoardType::trigon_3:
        if (nu_moves < nu_colors)
        {
            is_piece_considered.fill(false);
            set_piece_considered(bc, "V", is_piece_considered);
            set_piece_considered(bc, "I6", is_piece_considered);
        }
        if (nu_moves < 4 * nu_colors)
        {
            filter_min_size(bc, 6, is_piece_considered);
            // O is a bad early move, it neither extends, nor blocks well
            set_piece_considered(bc, "O", is_piece_considered, false);
        }
        else if (nu_moves < 5 * nu_colors)
            filter_min_size(bc, 5, is_piece_considered);
        else if (nu_moves < 7 * nu_colors)
            filter_min_size(bc, 4, is_piece_considered);
        else if (nu_moves < 9 * nu_colors)
            filter_min_size(bc, 3, is_piece_considered);
        break;
    case BoardType::nexos:
        if (nu_moves < 3 * nu_colors)
            filter_min_size(bc, 4, is_piece_considered);
        else if (nu_moves < 5 * nu_colors)
            filter_min_size(bc, 3, is_piece_considered);
        break;
    case BoardType::callisto:
    case BoardType::callisto_2:
    case BoardType::callisto_3:
        is_piece_considered[bd.get_one_piece()] = false;
        break;
    }
}

} // namespace

//-----------------------------------------------------------------------------

SharedConst::SharedConst(const Color& to_play)
    : board(nullptr),
      to_play(to_play),
      avoid_symmetric_draw(true)
{ }

void SharedConst::init(bool is_followup)
{
    auto& bd = *board;
    auto& bc = bd.get_board_const();

    // Initialize precomp_moves
    for (Color c : bd.get_colors())
    {
        auto& precomp = precomp_moves[c];
        auto& old_precomp = (is_followup ? precomp : bc.get_precomp_moves());

        m_is_forbidden.set();
        for (Point p : bd)
            if (! bd.is_forbidden(p, c))
            {
                auto adj_status = bd.get_adj_status(p, c);
                for (Piece piece : bd.get_pieces_left(c))
                {
                    if (! old_precomp.has_moves(piece, p, adj_status))
                        continue;
                    for (Move mv : old_precomp.get_moves(piece, p, adj_status))
                        if (m_is_forbidden[mv] && ! bd.is_forbidden(c, mv))
                            m_is_forbidden.clear(mv);
                }
            }

        // Don't use bd.get_pieces_left() because its ordering is not preserved
        // during a game. The in-place construction requires that the loop
        // iterates in the same order as during the last construction such that
        // it doesn't overwrite elements it still needs to read.
        Board::PiecesLeftList pieces;
        for (Piece::IntType i = 0; i < bc.get_nu_pieces(); ++i)
            if (bd.is_piece_left(c, Piece(i)))
                pieces.push_back(Piece(i));
        if (! is_followup)
            for (Point p : bd)
                if (! bd.is_forbidden(p, c))
                {
                    auto adj_status = bd.get_adj_status(p, c);
                    for (unsigned i = 0; i < PrecompMoves::nu_adj_status; ++i)
                        if (is_followup_adj_status(i, adj_status))
                            for (auto piece : pieces)
                                precomp.set_list_range(p, i, piece, 0, 0);
                }
        unsigned n = 0;
        for (Point p : bd)
        {
            if (bd.is_forbidden(p, c))
                continue;
            auto adj_status = bd.get_adj_status(p, c);
            for (unsigned i = 0; i < PrecompMoves::nu_adj_status; ++i)
            {
                if (! is_followup_adj_status(i, adj_status))
                    continue;
                for (auto piece : pieces)
                {
                    if (! old_precomp.has_moves(piece, p, i))
                        continue;
                    auto begin = n;
                    for (auto& mv : old_precomp.get_moves(piece, p, i))
                        if (! m_is_forbidden[mv])
                            precomp.set_move(n++, mv);
                    precomp.set_list_range(p, i, piece, begin, n - begin);
                }
            }
        }
    }

    if (! is_followup)
        init_pieces_considered();
    if (bd.get_piece_set() == PieceSet::callisto)
        init_one_piece_callisto(is_followup);
    symmetric_points.init(bd.get_geometry(), PointTransfRot180<Point>());
}

void SharedConst::init_one_piece_callisto(bool is_followup)
{
    auto& bd = *board;
    //auto& points = one_piece_points_callisto;
    Point useless_point = Point::null();
    unsigned n = 0;
    if (! is_followup)
    {
        for (Point p : bd)
            if (! bd.is_center_section(p) && bd.get_point_state(p).is_empty())
            {
                if (! is_useless_one_piece_point(p))
                    one_piece_points_callisto.get_unchecked(n++) = p;
                else
                    useless_point = p;
            }
    }
    else
        for (Point p : one_piece_points_callisto)
            if (bd.get_point_state(p).is_empty())
            {
                if (! is_useless_one_piece_point(p))
                    one_piece_points_callisto.get_unchecked(n++) = p;
                else
                    useless_point = p;
            }
    if (n == 0 && ! useless_point.is_null())
        // Allow one useless point if no useful points exist to avoid that
        // the player fails to generate a move (in the unlikely case that
        // no moves with larger pieces exist).
        one_piece_points_callisto.get_unchecked(n++) = useless_point;
    one_piece_points_callisto.resize(n);
}

void SharedConst::init_pieces_considered()
{
    auto& bd = *board;
    auto& bc = bd.get_board_const();
    is_piece_considered_list.clear();
    bool is_callisto = (bd.get_piece_set() == PieceSet::callisto);
    for (auto i = bd.get_nu_onboard_pieces(); i < Board::max_game_moves; ++i)
    {
        PieceMap<bool> is_piece_considered;
        set_pieces_considered(bd, i, is_piece_considered);
        bool are_all_considered = true;
        for (Piece::IntType j = 0; j < bc.get_nu_pieces(); ++j)
            if (! is_piece_considered[Piece(j)]
                    && ! (is_callisto && Piece(j) == bd.get_one_piece()))
            {
                are_all_considered = false;
                break;
            }
        if (are_all_considered)
        {
            min_move_all_considered = i;
            break;
        }
        auto pos = find(is_piece_considered_list.begin(),
                        is_piece_considered_list.end(),
                        is_piece_considered);
        if (pos != is_piece_considered_list.end())
            this->is_piece_considered[i] = &(*pos);
        else
        {
            is_piece_considered_list.push_back(is_piece_considered);
            this->is_piece_considered[i] = &is_piece_considered_list.back();
        }
    }
    is_piece_considered_all.fill(true);
    if (is_callisto)
        is_piece_considered_all[bd.get_one_piece()] = false;
    is_piece_considered_none.fill(false);
}

/** Check if a point is a useless move for the 1-piece.
    @return true if neighbors are occupied, because the 1-piece doesn't
    contribute to the score and playing there neither enables own moves
    nor prevents opponent moves with larger pieces. */
bool SharedConst::is_useless_one_piece_point(Point p) const
{
    auto& bd = *board;
    auto& geo = bd.get_geometry();
    auto x = geo.get_x(p);
    auto y = geo.get_y(p);
    if (x > 0 && geo.is_onboard(x - 1, y)
            && bd.get_point_state(geo.get_point(x - 1, y)).is_empty())
        return false;
    if (x < geo.get_width() - 1 && geo.is_onboard(x + 1, y)
            && bd.get_point_state(geo.get_point(x + 1, y)).is_empty())
        return false;
    if (y > 0 && geo.is_onboard(x, y - 1)
            && bd.get_point_state(geo.get_point(x, y - 1)).is_empty())
        return false;
    if (y < geo.get_height() - 1 && geo.is_onboard(x, y + 1)
            && bd.get_point_state(geo.get_point(x, y + 1)).is_empty())
        return false;
    return true;
}

//-----------------------------------------------------------------------------

} // namespace libpentobi_mcts
