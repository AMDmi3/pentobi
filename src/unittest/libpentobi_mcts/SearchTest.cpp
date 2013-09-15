//-----------------------------------------------------------------------------
/** @file unittest/libpentobi_mcts/SearchTest.cpp */
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "libpentobi_mcts/Search.h"

#include "libboardgame_sgf/TreeReader.h"
#include "libboardgame_sgf/Util.h"
#include "libboardgame_test/Test.h"
#include "libboardgame_util/CpuTime.h"
#include "libpentobi_base/BoardUpdater.h"
#include "libpentobi_base/Tree.h"

using namespace std;
using namespace libpentobi_mcts;
using libboardgame_sgf::TreeReader;
using libboardgame_sgf::util::get_last_node;
using libboardgame_util::CpuTime;
using libpentobi_base::BoardUpdater;

//-----------------------------------------------------------------------------

/** Test that state generates a playout move even if no large pieces are
    playable early in the game.
    This tests for a bug that occurred in Pentobi 1.1 with game variant Trigon:
    Because moves that are below a certain piece size are not generated early
    in the game, it could happen in rare cases that no moves were generated,
    which could even cause crashes, because the maximum game length was set
    with the assumption that pass moves are only played if a color has no more
    legal moves. */
LIBBOARDGAME_TEST_CASE(pentobi_mcts_search_no_large_pieces)
{
    istringstream
        in("("
           ";GM[Blokus Trigon Two-Player]"
           ";1[r4,r5,s5,r6,s6,r7]"
           ";2[r12,q13,r13,q14,r14,r15]"
           ";3[k11,l11,m11,n11,j12,k12]"
           ";4[w7,x7,y7,z7,v8,w8]"
           ";1[s8,t8,r9,s9,t9,u9]"
           ";2[n12,o12,m13,n13,o13,o14]"
           ";3[k13,k14,l14,l15,m15,n15]"
           ";4[w9,t10,u10,v10,w10,x10]"
           ";1[n10,o10,p10,q10,r10,r11]"
           ";2[o15,k16,l16,m16,n16,o16]"
           ";3[i15,j15,h16,i16,j16,j17]"
           ";4[u11,s12,t12,u12,v12,v13]"
           ";1[p4,m5,n5,o5,p5,m6]"
           ";2[k17,i18,j18,k18,l18,m18]"
           ";3[l17,m17,n17,o17,p17,o18]"
           ";4[t14,u14,s15,t15,r16,s16]"
           ";1[l8,m8,j9,k9,l9,m9])"
           );
    TreeReader reader;
    reader.read(in);
    unique_ptr<libboardgame_sgf::Node> root =
        reader.get_tree_transfer_ownership();
    libpentobi_base::Tree tree(root);
    unique_ptr<Board> bd(new Board(tree.get_variant()));
    BoardUpdater updater;
    updater.update(*bd, tree, get_last_node(tree.get_root()));
    unsigned nu_threads = 1;
    size_t memory = 10000;
    unique_ptr<Search> search(new Search(bd->get_variant(), nu_threads,
                                         memory));
    Float max_count = 1;
    Float min_simulations = 1;
    double max_time = 0;
    CpuTime time_source;
    Move mv;
    bool res = search->search(mv, *bd, Color(1), max_count, min_simulations,
                              max_time, time_source);
    LIBBOARDGAME_CHECK(res);
    LIBBOARDGAME_CHECK(! mv.is_null());
    LIBBOARDGAME_CHECK(! mv.is_pass());
}

//-----------------------------------------------------------------------------
