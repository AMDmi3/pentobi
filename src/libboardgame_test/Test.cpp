//-----------------------------------------------------------------------------
/** @file libboardgame_test/Test.cpp
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Test.h"

#include <sstream>
#include <map>
#include "libboardgame_util/Assert.h"
#include "libboardgame_util/Log.h"

using libboardgame_util::log;

namespace libboardgame_test {

using namespace std;

//-----------------------------------------------------------------------------

namespace {

map<string, TestFunction>& get_all_tests()
{
    static map<string, TestFunction> all_tests;
    return all_tests;
}

string get_fail_msg(const char* file, int line, const string& s)
{
    ostringstream msg;
    msg << file << ":" << line << ": " << s;
    return msg.str();
}

} // namespace

//-----------------------------------------------------------------------------

TestFail::TestFail(const char* file, int line, const string& s)
    : Exception(get_fail_msg(file, line, s))
{
}

//-----------------------------------------------------------------------------

void add_test(const string& name, TestFunction function)
{
    auto& all_tests = get_all_tests();
    LIBBOARDGAME_ASSERT(all_tests.find(name) == all_tests.end());
    all_tests.insert(make_pair(name, function));
}

bool run_all_tests()
{
    unsigned nu_fail = 0;
    log("Running ", get_all_tests().size(), " tests...");
    for (auto& i : get_all_tests())
    {
        try
        {
            (i.second)();
        }
        catch (const TestFail& e)
        {
            log(e.what());
            ++nu_fail;
        }
    }
    if (nu_fail == 0)
    {
        log("OK");
        return true;
    }
    else
    {
        log(nu_fail, " tests failed.\nFAIL");
        return false;
    }
}

bool run_test(const string& name)
{
    for (auto& i : get_all_tests())
        if (i.first == name)
        {
            log("Running ", name, "...");
            try
            {
                (i.second)();
                log("OK");
                return true;
            }
            catch (const TestFail& e)
            {
                log(e.what(), "\nFAIL");
                return false;
            }
        }
    log("Test not found: ", name);
    return false;
}

int test_main(int argc, char* argv[])
{
    if (argc < 2)
    {
        if (libboardgame_test::run_all_tests())
            return 0;
        else
            return 1;
    }
    else
    {
        int result = 0;
        for (int i = 1; i < argc; ++i)
            if (! libboardgame_test::run_test(argv[i]))
                result = 1;
        return result;
    }
}

//-----------------------------------------------------------------------------

} // namespace libboardgame_test
