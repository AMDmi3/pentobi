//-----------------------------------------------------------------------------
/** @file NullTermList.h */
//-----------------------------------------------------------------------------

#ifndef LIBBOARDGAME_UTIL_NULL_TERM_LIST_H
#define LIBBOARDGAME_UTIL_NULL_TERM_LIST_H

#include "Assert.h"
#include "NullElement.h"

namespace libboardgame_util {

using namespace std;

//-----------------------------------------------------------------------------

/** Array-based list terminated with a null element.
    Fast construction and iteration. */
template<typename T, unsigned int M>
class NullTermList
{
    friend class Init;
    friend class Iterator;

public:
    class Init
    {
    public:
        Init(NullTermList& list);

        void push_back(const T& t);

        bool include(NullTermList& list, const T& t);

        void finish();

    private:
        T* m_t;

#if LIBBOARDGAME_DEBUG
        NullTermList* m_list;
#endif
    };

    class Iterator
    {
    public:
        Iterator(const NullTermList& list);

        const T& operator*() const;

        operator bool() const;

        void operator++();

    private:
        const T* m_t;
    };

    static const unsigned int max_size = M;

    NullTermList();

    unsigned int size() const;

    bool empty() const;

    bool contains(const T& t) const;

private:
    T m_a[max_size + 1];

#if LIBBOARDGAME_DEBUG
    bool m_is_initialized;
#endif
};

template<typename T, unsigned int M>
inline NullTermList<T, M>::Init::Init(NullTermList& list)
    : m_t(list.m_a)
{
#if LIBBOARDGAME_DEBUG
    m_list = &list;
    list.m_is_initialized = false;
#endif
}

template<typename T, unsigned int M>
inline void NullTermList<T, M>::Init::push_back(const T& t)
{
    LIBBOARDGAME_ASSERT(m_t - m_list->m_a < max_size);
    *m_t = t;
    ++m_t;
}

template<typename T, unsigned int M>
inline void NullTermList<T, M>::Init::finish()
{
    set_null(*m_t);
#if LIBBOARDGAME_DEBUG
    m_list->m_is_initialized = true;
#endif
}

template<typename T, unsigned int M>
bool NullTermList<T, M>::Init::include(NullTermList& list, const T& t)
{
    for (T* i = list.m_a; i != m_t; ++i)
        if (*i == t)
            return false;
    push_back(t);
    return true;
}

template<typename T, unsigned int M>
inline NullTermList<T, M>::Iterator::Iterator(const NullTermList& list)
{
    LIBBOARDGAME_ASSERT(list.m_is_initialized);
    m_t = list.m_a;
}

template<typename T, unsigned int M>
inline const T& NullTermList<T, M>::Iterator::operator*() const
{
    LIBBOARDGAME_ASSERT(operator bool());
    return *m_t;
}

template<typename T, unsigned int M>
inline NullTermList<T, M>::Iterator::operator bool() const
{
    return ! is_null(*m_t);
}

template<typename T, unsigned int M>
inline void NullTermList<T, M>::Iterator::operator++()
{
    LIBBOARDGAME_ASSERT(operator bool());
    ++m_t;
}

template<typename T, unsigned int M>
inline NullTermList<T, M>::NullTermList()
{
#if LIBBOARDGAME_DEBUG
    m_is_initialized = false;
#endif
}

template<typename T, unsigned int M>
inline bool NullTermList<T, M>::contains(const T& t) const
{
    LIBBOARDGAME_ASSERT(m_is_initialized);
    for (Iterator i(*this); i; ++i)
        if (*i == t)
            return true;
    return false;
}

template<typename T, unsigned int M>
inline bool NullTermList<T, M>::empty() const
{
    return is_null(m_a[0]);
}

template<typename T, unsigned int M>
inline unsigned int NullTermList<T, M>::size() const
{
    LIBBOARDGAME_ASSERT(m_is_initialized);
    unsigned int n = 0;
    for (Iterator i(*this); i; ++i)
        ++n;
    return n;
}

//-----------------------------------------------------------------------------

} // namespace libboardgame_util

#endif // LIBBOARDGAME_UTIL_NULL_TERM_LIST_H
