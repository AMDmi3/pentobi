//-----------------------------------------------------------------------------
/** @file unittest/libboardgame_gtp/ArgumentsTest.cpp
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "libboardgame_gtp/Arguments.h"
#include "libboardgame_test/Test.h"

using namespace std;
using namespace libboardgame_gtp;

//-----------------------------------------------------------------------------

LIBBOARDGAME_TEST_CASE(gtp_arguments_arg)
{
    CmdLine line("command arg1   \"arg2 \" arg3 ");
    Arguments args(line);
    LIBBOARDGAME_CHECK_EQUAL("arg1", string(args.get(0)));
    LIBBOARDGAME_CHECK_EQUAL("arg2 ", string(args.get(1)));
    LIBBOARDGAME_CHECK_EQUAL("arg3", string(args.get(2)));
}

LIBBOARDGAME_TEST_CASE(gtp_arguments_to_lower)
{
    CmdLine line("command cAsE");
    Arguments args(line);
    LIBBOARDGAME_CHECK_EQUAL(string("case"), args.get_tolower(0));
}

LIBBOARDGAME_TEST_CASE(gtp_arguments_bool)
{
    {
        CmdLine line("command 0");
        Arguments args(line);
        LIBBOARDGAME_CHECK(! args.parse<bool>(0));
    }
    {
        CmdLine line("command 1");
        Arguments args(line);
        LIBBOARDGAME_CHECK(args.parse<bool>(0));
    }
    {
        CmdLine line("command 2");
        Arguments args(line);
        LIBBOARDGAME_CHECK_THROW(args.parse<bool>(0), Failure);
    }
    {
        CmdLine line("command arg1");
        Arguments args(line);
        LIBBOARDGAME_CHECK_THROW(args.parse<bool>(0), Failure);
    }
    {
        CmdLine line("command");
        Arguments args(line);
        LIBBOARDGAME_CHECK_THROW(args.parse<bool>(0), Failure);
    }
}

LIBBOARDGAME_TEST_CASE(gtp_arguments_float)
{
    CmdLine line("command abc 5.5");
    Arguments args(line);
    LIBBOARDGAME_CHECK_THROW(args.parse<float>(0), Failure);
    LIBBOARDGAME_CHECK_CLOSE(5.5f, args.parse<float>(1), 1e-4);
}

LIBBOARDGAME_TEST_CASE(gtp_arguments_int)
{
    CmdLine line("command 5 arg");
    Arguments args(line);
    LIBBOARDGAME_CHECK_EQUAL(5, args.parse<int>(0));
    LIBBOARDGAME_CHECK_THROW(args.parse<int>(1), Failure);
}

LIBBOARDGAME_TEST_CASE(gtp_arguments_min_int)
{
    CmdLine line("command 5");
    Arguments args(line);
    LIBBOARDGAME_CHECK_EQUAL(5, args.parse_min<int>(0, 3));
    LIBBOARDGAME_CHECK_THROW(args.parse_min<int>(0, 7), Failure);
}

LIBBOARDGAME_TEST_CASE(gtp_arguments_min_max_int)
{
    CmdLine line("command 5");
    Arguments args(line);
    LIBBOARDGAME_CHECK_EQUAL(5, args.parse_min_max<int>(0, 3, 10));
    LIBBOARDGAME_CHECK_THROW(args.parse_min_max<int>(0, 0, 4), Failure);
    LIBBOARDGAME_CHECK_THROW(args.parse_min_max<int>(0, 10, 20), Failure);
}

LIBBOARDGAME_TEST_CASE(gtp_arguments_single_int)
{
    {
        CmdLine line("command 5");
        Arguments args(line);
        LIBBOARDGAME_CHECK_EQUAL(5, args.parse<int>());
    }
    {
        CmdLine line("command 5 10");
        Arguments args(line);
        LIBBOARDGAME_CHECK_THROW(args.parse<int>(), Failure);
    }
}

LIBBOARDGAME_TEST_CASE(gtp_arguments_nu_arg_0)
{
    CmdLine line("1 command");
    Arguments args(line);
    LIBBOARDGAME_CHECK_NO_THROW(args.check_empty());
    LIBBOARDGAME_CHECK_THROW(args.check_size(1), Failure);
    LIBBOARDGAME_CHECK_NO_THROW(args.check_size_less_equal(2));
}

LIBBOARDGAME_TEST_CASE(gtp_arguments_nu_arg_3)
{
    CmdLine line("command arg1 arg2 arg3");
    Arguments args(line);
    LIBBOARDGAME_CHECK_THROW(args.check_empty(), Failure);
    LIBBOARDGAME_CHECK_THROW(args.check_size(2), Failure);
    LIBBOARDGAME_CHECK_NO_THROW(args.check_size(3));
    LIBBOARDGAME_CHECK_THROW(args.check_size(4), Failure);
    LIBBOARDGAME_CHECK_THROW(args.check_size_less_equal(2), Failure);
    LIBBOARDGAME_CHECK_NO_THROW(args.check_size_less_equal(3));
    LIBBOARDGAME_CHECK_NO_THROW(args.check_size_less_equal(4));
}

LIBBOARDGAME_TEST_CASE(gtp_arguments_remaining_arg)
{
    CmdLine line("command arg1 arg2");
    Arguments args(line);
    LIBBOARDGAME_CHECK_EQUAL("arg2", string(args.get_remaining_line(0)));
}

LIBBOARDGAME_TEST_CASE(gtp_arguments_remaining_arg_empty)
{
    CmdLine line("command arg1");
    Arguments args(line);
    LIBBOARDGAME_CHECK_EQUAL("", string(args.get_remaining_line(0)));
}

LIBBOARDGAME_TEST_CASE(gtp_arguments_remaining_line)
{
    CmdLine line("command arg1   \"arg2 \" arg3 ");
    Arguments args(line);
    LIBBOARDGAME_CHECK_EQUAL("\"arg2 \" arg3",
                             string(args.get_remaining_line(0)));
    LIBBOARDGAME_CHECK_EQUAL("arg3", string(args.get_remaining_line(1)));
    LIBBOARDGAME_CHECK_EQUAL("", string(args.get_remaining_line(2)));
}

//-----------------------------------------------------------------------------
