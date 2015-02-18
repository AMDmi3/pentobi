//-----------------------------------------------------------------------------
/** @file libpentobi_base/Color.h
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifndef LIBPENTOBI_BASE_COLOR_H
#define LIBPENTOBI_BASE_COLOR_H

#include <cstdint>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include "libboardgame_util/Assert.h"

namespace libpentobi_base {

using namespace std;

//-----------------------------------------------------------------------------

class Color
{
public:
    typedef uint_fast8_t IntType;

    class InvalidString
        : public runtime_error
    {
    public:
        InvalidString(const string& s)
            : runtime_error(s)
        { }
    };

    class Iterator
    {
    public:
        Iterator(IntType i)
        {
            m_i = i;
        }

        bool operator==(Iterator it) const
        {
            return m_i == it.m_i;
        }

        bool operator!=(Iterator it) const
        {
            return m_i != it.m_i;
        }

        void operator++()
        {
            ++m_i;
        }

        Color operator*() const
        {
            return Color(m_i);
        }

    private:
        IntType m_i;
    };

    class Range
    {
    public:
        Range(IntType nu_colors)
            : m_nu_colors(nu_colors)
        { }

        Iterator begin() const
        {
            return 0;
        }

        Iterator end() const
        {
            return m_nu_colors;
        }

    private:
        IntType m_nu_colors;
    };

    static const IntType range = 4;

    Color();

    explicit Color(IntType i);

    explicit Color(const string& s);

    bool operator==(const Color& c) const;

    bool operator!=(const Color& c) const;

    bool operator<(const Color& c) const;

    IntType to_int() const;

    Color get_next(IntType nu_colors) const;

    Color get_previous(IntType nu_colors) const;

private:
    static const IntType value_uninitialized = range;

    IntType m_i;

    bool is_initialized() const;
};


inline Color::Color()
{
#if LIBBOARDGAME_DEBUG
    m_i = value_uninitialized;
#endif
}

inline Color::Color(IntType i)
{
    LIBBOARDGAME_ASSERT(i < range);
    m_i = i;
}

inline bool Color::operator==(const Color& c) const
{
    LIBBOARDGAME_ASSERT(is_initialized());
    LIBBOARDGAME_ASSERT(c.is_initialized());
    return m_i == c.m_i;
}

inline bool Color::operator!=(const Color& c) const
{
    return ! operator==(c);
}

inline bool Color::operator<(const Color& c) const
{
    LIBBOARDGAME_ASSERT(is_initialized());
    LIBBOARDGAME_ASSERT(c.is_initialized());
    return m_i < c.m_i;
}

inline Color Color::get_next(IntType nu_colors) const
{
    IntType i = static_cast<IntType>(m_i + 1);
    if (i == nu_colors)
        return Color(0);
    return Color(i);
}

inline Color Color::get_previous(IntType nu_colors) const
{
    LIBBOARDGAME_ASSERT(nu_colors > 1);
    if (m_i == 0)
        return Color(static_cast<IntType>(nu_colors - 1));
    return Color(static_cast<IntType>(m_i - 1));
}

inline bool Color::is_initialized() const
{
    return m_i < value_uninitialized;
}

inline Color::IntType Color::to_int() const
{
    LIBBOARDGAME_ASSERT(is_initialized());
    return m_i;
}

//-----------------------------------------------------------------------------

/** Output string representation of color.
    The strings "1", "2", ... are used for the colors. */
ostream& operator<<(ostream& out, const Color& c);

/** Read color from input stream.
    Accepts the strings "1", "2", ..., as well as "b", "w" or "black", "white"
    for the first two colors. */
istream& operator>>(istream& in, Color& c);

//-----------------------------------------------------------------------------

/** Unrolled loop over all colors. */
template<class FUNCTION>
inline void for_each_color(FUNCTION f)
{
    static_assert(Color::range == 4, "");
    f(Color(0));
    f(Color(1));
    f(Color(2));
    f(Color(3));
}

//-----------------------------------------------------------------------------

} // namespace libpentobi_base

#endif // LIBPENTOBI_BASE_COLOR_H
