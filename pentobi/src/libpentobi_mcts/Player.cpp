//-----------------------------------------------------------------------------
/** @file libpentobi_mcts/Player.cpp */
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Player.h"

#include "libboardgame_util/WallTime.h"

namespace libpentobi_mcts {

using namespace std;
using libboardgame_util::log;
using libboardgame_util::WallTime;
using libpentobi_base::game_variant_classic;
using libpentobi_base::game_variant_duo;
using libpentobi_base::GameVariant;

//-----------------------------------------------------------------------------

namespace {

const bool use_weight_max_count = true;

#include "book_duo.blksgf.h"
#include "book_classic_2.blksgf.h"

} // namespace

//-----------------------------------------------------------------------------

Player::Player(const Board& bd)
    : libpentobi_base::Player(bd),
      m_is_book_loaded(false),
      m_use_book(true),
      m_level(4),
      m_fixed_simulations(0),
      m_search(bd)
{
    for (unsigned int i = 0; i < Board::max_player_moves; ++i)
        // Hand-tuned such that time per move is more evenly spread among all
        // moves than with a fixed number of simulations (because the
        // simulations per second increase rapidly with the move number) but
        // the average time per game is roughly the same
        weight_max_count[i] = ValueType(0.7 * exp(0.1 * i));
}

Player::~Player() throw()
{
}

Move Player::genmove(Color c)
{
    if (! m_bd.has_moves(c))
        return Move::pass();
    Move mv;
    GameVariant variant = m_bd.get_game_variant();
    if (m_use_book && variant != game_variant_classic)
    {
        if (! m_is_book_loaded
            || m_book.get_tree().get_game_variant() != variant)
        {
            const char* data;
            if (variant == game_variant_duo)
                data = builtin_book_duo;
            else
                data = builtin_book_classic_2;
            istringstream in(data);
            m_book.load(in);
            m_is_book_loaded = true;
        }
        double delta;
        if (m_level <= 1)
            delta = 0.05;
        else if (m_level <= 2)
            delta = 0.04;
        else if (m_level <= 3)
            delta = 0.03;
        else if (m_level <= 4)
            delta = 0.02;
        else if (m_level <= 5)
            delta = 0.015;
        else if (m_level >= 6)
            delta = 0.01;
        mv = m_book.genmove(m_bd, c, delta, 4 * delta);
        if (! mv.is_null())
            return mv;
    }
    double max_time = 0;
    WallTime time_source;
    ValueType max_count;
    if (m_fixed_simulations > 0)
        max_count = m_fixed_simulations;
    else
    {
        if (m_level <= 1)
            max_count = 125;
        else if (m_level >= 6)
            max_count = ValueType(125 * pow(2.0, (6 - 1) * 2));
        else
            max_count = ValueType(125 * pow(2.0, (m_level - 1) * 2));
        if (use_weight_max_count)
        {
            unsigned int player_move = m_bd.get_nu_moves() / Color::range;
            max_count = ceil(max_count * weight_max_count[player_move]);
        }
    }
    log() << "MaxCnt " << max_count << '\n';
    if (! m_search.search(mv, c, max_count, 0, max_time, time_source))
        return Move::null();
    return mv;
}

void Player::load_book(istream& in)
{
    m_book.load(in);
    m_is_book_loaded = true;
}

//-----------------------------------------------------------------------------

} // namespace libpentobi_mcts
