//-----------------------------------------------------------------------------
/** @file libboardgame_base/StringRep.h
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifndef LIBBOARDGAME_BASE_STRING_REP_H
#define LIBBOARDGAME_BASE_STRING_REP_H

#include <iosfwd>

namespace libboardgame_base {

using namespace std;

//-----------------------------------------------------------------------------

/** String representation of points. */
struct StringRep
{
    virtual ~StringRep();

    virtual bool read(istream& in, unsigned width, unsigned height,
                      unsigned& x, unsigned& y) const = 0;

    virtual void write(ostream& out, unsigned x, unsigned y, unsigned width,
                       unsigned height) const = 0;
};

//-----------------------------------------------------------------------------

/** Spreadsheet-style string representation of points.
    Can be used as a template argument for libboardgame_base::Point.
    Columns are represented as letters including the letter 'J'. After 'Z',
    multi-letter combinations are used: 'AA', 'AB', etc. Rows are represented
    by numbers starting with '1'. Note that unlike in spreadsheets, row number
    1 is at the bottom and increases to the top to be compatible with the
    convention used in chess. */
struct StdStringRep
        : public StringRep
{
    ~StdStringRep();

    bool read(istream& in, unsigned width, unsigned height, unsigned& x,
              unsigned& y) const override;

    void write(ostream& out, unsigned x, unsigned y, unsigned width,
               unsigned height) const override;
};

//-----------------------------------------------------------------------------

} // namespace libboardgame_base

#endif // LIBBOARDGAME_BASE_STRING_REP_H
