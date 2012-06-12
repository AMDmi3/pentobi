//-----------------------------------------------------------------------------
/** @file pentobi/RatingHistory.h */
//-----------------------------------------------------------------------------

#ifndef PENTOBI_RATING_HISTORY_H
#define PENTOBI_RATING_HISTORY_H

#include <string>
#include <vector>
#include <boost/filesystem.hpp>
#include "libboardgame_base/Rating.h"
#include "libpentobi_base/Color.h"
#include "libpentobi_base/Variant.h"

using namespace std;
using boost::filesystem::path;
using libboardgame_base::Rating;
using libpentobi_base::Color;
using libpentobi_base::Variant;

//-----------------------------------------------------------------------------

/** History of rated games in a certain game variant. */
class RatingHistory
{
public:
    /** Maximum number of games to remember ing the history. */
    static const unsigned int maxGames = 100;

    struct GameInfo
    {
        /** Game number.
            The first game played has number 0. */
        unsigned int number;

        /** Color played by the human.
            In game variants with multiple colors per player, the human played
            all colors played by the player of this color. */
        Color color;

        /** Game result.
            0=Loss, 0.5=tie, 1=win from the viewpoint of the human. */
        float result;

        /** Date of the game in "YYYY-MM-DD" format. */
        string date;

        /** The playing level of the computer opponent. */
        int level;

        /** The rating of the human after the game. */
        Rating rating;
    };

    RatingHistory(Variant variant, const path& datadir);

    /** Append a new game. */
    void add(unsigned int number, Color color, float result,
             const string& date, int level, Rating rating);

    /** Saves the history. */
    void save() const;

    const vector<GameInfo>& get() const;

private:
    path m_dir;

    path m_file;

    vector<GameInfo> m_games;
};

inline const vector<RatingHistory::GameInfo>& RatingHistory::get() const
{
    return m_games;
}

//-----------------------------------------------------------------------------

#endif // PENTOBI_RATING_HISTORY_H
