//-----------------------------------------------------------------------------
/** @file unittest/libboardgame_sgf/SgfUtilTest.cpp
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "libboardgame_sgf/SgfUtil.h"

#include "libboardgame_test/Test.h"

using namespace std;
using namespace libboardgame_sgf;
using namespace libboardgame_sgf::util;

//-----------------------------------------------------------------------------

LIBBOARDGAME_TEST_CASE(sgf_util_get_path_from_root)
{
    auto root = make_unique<SgfNode>();
    auto& child = root->create_new_child();
    vector<const SgfNode*> path;
    get_path_from_root(child, path);
    LIBBOARDGAME_CHECK_EQUAL(path.size(), 2u);
    LIBBOARDGAME_CHECK_EQUAL(path[0], root.get());
    LIBBOARDGAME_CHECK_EQUAL(path[1], &child);
}

//-----------------------------------------------------------------------------
