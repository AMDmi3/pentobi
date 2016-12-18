//-----------------------------------------------------------------------------
/** @file libpentobi_base/TreeUtil.cpp
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "TreeUtil.h"

#include "NodeUtil.h"
#include "libboardgame_sgf/SgfUtil.h"

namespace libpentobi_base {
namespace tree_util {

using libboardgame_sgf::util::get_move_annotation;
using libboardgame_sgf::util::get_variation_string;

//-----------------------------------------------------------------------------

const SgfNode* get_move_node(const PentobiTree& tree, const SgfNode& node,
                             unsigned n)
{
    auto move_number = get_move_number(tree, node);
    if (n == move_number)
        return &node;
    if (n < move_number)
    {
        auto current = &node;
        do
        {
            if (tree.has_move(*current))
            {
                if (move_number == n)
                    return current;
                --move_number;
            }
            if (libpentobi_base::node_util::has_setup(*current))
                break;
            current = current->get_parent_or_null();
        }
        while (current);
    }
    else
    {
        auto current = node.get_first_child_or_null();
        while (current)
        {
            if (libpentobi_base::node_util::has_setup(*current))
                break;
            if (tree.has_move(*current))
            {
                ++move_number;
                if (move_number == n)
                    return current;
            }
            current = current->get_first_child_or_null();
        }
    }
    return nullptr;
}

unsigned get_move_number(const PentobiTree& tree, const SgfNode& node)
{
    unsigned move_number = 0;
    auto current = &node;
    do
    {
        if (tree.has_move(*current))
            ++move_number;
        if (libpentobi_base::node_util::has_setup(*current))
            break;
        current = current->get_parent_or_null();
    }
    while (current);
    return move_number;
}

unsigned get_moves_left(const PentobiTree& tree, const SgfNode& node)
{
    unsigned moves_left = 0;
    auto current = node.get_first_child_or_null();
    while (current)
    {
        if (libpentobi_base::node_util::has_setup(*current))
            break;
        if (tree.has_move(*current))
            ++moves_left;
        current = current->get_first_child_or_null();
    }
    return moves_left;
}

string get_position_info(const PentobiTree& tree, const SgfNode& node)
{
    auto move = get_move_number(tree, node);
    auto left = get_moves_left(tree, node);
    auto total = move + left;
    auto variation = get_variation_string(node);
    auto annotation = get_move_annotation(tree, node);
    ostringstream s;
    if (left > 0 || move > 0)
        s << move << annotation;
    if (left > 0)
        s << '/' << total;
    if (! variation.empty())
        s << " (" << variation << ')';
    return s.str();
}

//-----------------------------------------------------------------------------

} // namespace tree_util
} // namespace libpentobi_base
