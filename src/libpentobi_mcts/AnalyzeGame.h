//-----------------------------------------------------------------------------
/** @file libpentobi_mcts/AnalyzeGame.h */
//-----------------------------------------------------------------------------

#ifndef LIBPENTOBI_MCTS_ANALYZE_GAME_H
#define LIBPENTOBI_MCTS_ANALYZE_GAME_H

#include "Search.h"
#include "libpentobi_base/Game.h"

namespace libpentobi_mcts {

using libpentobi_base::Game;
using libpentobi_base::Variant;

//-----------------------------------------------------------------------------

/** Evaluate each position in the main variation of a game. */
class AnalyzeGame
{
public:
    /** Run the analysis.
        The analysis can be aborted from a different thread with
        libboardgame_util::set_abort().
        @param game
        @param search
        @param nu_simulations
        @param progress_callback Function that will be called at the beginning
        of the analysis of a position. Arguments: number moves analyzed so far,
        total number of moves. */
    void run(const Game& game, Search& search, size_t nu_simulations,
             function<void(unsigned int,unsigned int)> progress_callback);

    Variant get_variant() const;

    unsigned int get_nu_moves() const;

    bool has_value(unsigned int i) const;

    ColorMove get_move(unsigned int i) const;

    double get_value(unsigned int i) const;

private:
    Variant m_variant;

    vector<ColorMove> m_moves;

    vector<bool> m_has_value;

    vector<double> m_values;
};

inline ColorMove AnalyzeGame::get_move(unsigned int i) const
{
    LIBBOARDGAME_ASSERT(i < m_moves.size());
    return m_moves[i];
}

inline unsigned int AnalyzeGame::get_nu_moves() const
{
    return static_cast<unsigned int>(m_moves.size());
}

inline double AnalyzeGame::get_value(unsigned int i) const
{
    LIBBOARDGAME_ASSERT(i < m_values.size());
    LIBBOARDGAME_ASSERT(has_value(i));
    return m_values[i];
}

inline Variant AnalyzeGame::get_variant() const
{
    return m_variant;
}

inline bool AnalyzeGame::has_value(unsigned int i) const
{
    LIBBOARDGAME_ASSERT(i < m_has_value.size());
    return m_has_value[i];
}

//-----------------------------------------------------------------------------

} // namespace libpentobi_mcts

#endif // LIBPENTOBI_MCTS_ANALYZE_GAME_H
