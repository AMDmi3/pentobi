//-----------------------------------------------------------------------------
/** @file libboardgame_util/Assert.h */
//-----------------------------------------------------------------------------

#ifndef LIBBOARDGAME_UTIL_ASSERT_H
#define LIBBOARDGAME_UTIL_ASSERT_H

namespace libboardgame_util {

//-----------------------------------------------------------------------------

class AssertionHandler
{
public:
    /** Construct and register assertion handler. */
    AssertionHandler();

    /** Destruct and unregister assertion handler. */
    virtual ~AssertionHandler() throw();

    virtual void run() = 0;
};

void handle_assertion(const char* expression, const char* file, int line);

//-----------------------------------------------------------------------------

} // namespace libboardgame_util

//-----------------------------------------------------------------------------

/** @def LIBBOARDGAME_ASSERT
    Enhanced assert macro.
    This macro is similar to the assert macro in the standard library, but it
    allows the user to register assertion handlers that are executed before the
    program is aborted. Assertions are only enabled if the macro
    LIBBOARDGAME_DEBUG is true. */
#if LIBBOARDGAME_DEBUG
#define LIBBOARDGAME_ASSERT(expr)                                       \
    ((expr) ? (static_cast<void>(0))                                    \
     : libboardgame_util::handle_assertion(#expr, __FILE__, __LINE__))
#else
#define LIBBOARDGAME_ASSERT(expr) (static_cast<void>(0))
#endif

//-----------------------------------------------------------------------------

#endif // LIBBOARDGAME_UTIL_ASSERT_H
