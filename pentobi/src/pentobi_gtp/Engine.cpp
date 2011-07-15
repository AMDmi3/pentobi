//-----------------------------------------------------------------------------
/** @file libboardgame_gtp/Engine.cpp */
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Engine.h"

#include <boost/format.hpp>
#include "libboardgame_sgf/TreeReader.h"
#include "libpentobi_base/Tree.h"

namespace pentobi_gtp {

using boost::format;
using libboardgame_gtp::Failure;
using libboardgame_mcts::ChildIterator;
using libboardgame_mcts::UctBiasTerm;
using libboardgame_sgf::TreeReader;
using libboardgame_util::log;
using libpentobi_base::Board;
using libpentobi_base::Piece;
using libpentobi_base::Tree;
using libpentobi_mcts::Move;
using libpentobi_mcts::State;
using libpentobi_mcts::ValueType;

//-----------------------------------------------------------------------------

namespace {

bool is_child_better(const libboardgame_mcts::Node<Move>* n1,
                     const libboardgame_mcts::Node<Move>* n2)
{
    ValueType count1 = n1->get_count();
    ValueType count2 = n2->get_count();
    if (count1 != count2)
        return count1 > count2;
    if (count1 > 0)
        return n1->get_value() > n2->get_value();
    return false;
}

} // namespace

//-----------------------------------------------------------------------------

Engine::Engine(GameVariant game_variant, int level, bool use_book)
    : libpentobi_base::Engine(game_variant)
{
    create_player();
    get_mcts_player().set_use_book(use_book);
    get_mcts_player().set_level(level);
    add("gen_playout_move", &Engine::cmd_gen_playout_move);
    add("param", &Engine::cmd_param);
    add("move_values", &Engine::cmd_move_values);
}

Engine::~Engine() throw()
{
}

void Engine::cmd_move_values(Response& response)
{
    const Search& search = get_search();
    const Board& bd = get_board();
    vector<const libboardgame_mcts::Node<Move>*> children;
    for (ChildIterator<Move> i(search.get_tree().get_root()); i; ++i)
        children.push_back(&(*i));
    sort(children.begin(), children.end(), is_child_better);
    response << fixed;
    BOOST_FOREACH(const libboardgame_mcts::Node<Move>* node, children)
    {
        ValueType count = node->get_count();
        response << setprecision(0) << count << ' ';
        if (count > 0)
            response << setprecision(3) << node->get_value();
        else
            response << '-';
        ValueType rave_count = node->get_rave_count();
        response << ' ' << setprecision(0) << rave_count << ' ';
        if (rave_count > 0)
            response << setprecision(3) << node->get_rave_value();
        else
            response << '-';
        response << ' ' << bd.to_string(node->get_move()) << '\n';
    }
}

void Engine::cmd_param(const Arguments& args, Response& response)
{
    Player& p = get_mcts_player();
    Search& s = get_search();
    UctBiasTerm<Move>& b = s.get_exploration_term();
    if (args.get_size() == 0)
        response
            << "avoid_symmetric_draw " << s.get_avoid_symmetric_draw() << '\n'
            << "bias_term_constant " << b.get_bias_term_constant() << '\n'
            << "detect_symmetry " << s.get_detect_symmetry() << '\n'
            << "expand_threshold " << s.get_expand_threshold() << '\n'
            << "fixed_simulations " << p.get_fixed_simulations() << '\n'
            << "level " << p.get_level() << '\n'
            << "score_modification " << s.get_score_modification() << '\n'
            << "use_book " << p.get_use_book() << '\n';
    else
    {
        args.check_size(2);
        string name = args.get(0);
        if (name == "avoid_symmetric_draw")
            s.set_avoid_symmetric_draw(args.get<bool>(1));
        else if (name == "bias_term_constant")
            b.set_bias_term_constant(args.get<ValueType>(1));
        else if (name == "detect_symmetry")
            s.set_detect_symmetry(args.get<bool>(1));
        else if (name == "expand_threshold")
            s.set_expand_threshold(args.get<ValueType>(1));
        else if (name == "fixed_simulations")
            p.set_fixed_simulations(args.get<ValueType>(1));
        else if (name == "level")
            p.set_level(args.get<int>(1));
        else if (name == "score_modification")
            s.set_score_modification(args.get<ValueType>(1));
        else if (name == "use_book")
            p.set_use_book(args.get<bool>(1));
        else
            throw Failure(format("unknown parameter '%1%'") % name);
    }
}

void Engine::create_player()
{
    m_player.reset(new Player(get_board()));
    set_player(*m_player);
}

void Engine::cmd_gen_playout_move(Response& response)
{
    State& state = get_mcts_player().get_search().get_state();
    state.start_search();
    state.start_simulation(0);
    state.start_playout();
    if (! state.gen_and_play_playout_move())
        throw Failure("terminal playout position");
    const Board& bd = get_board();
    response << bd.to_string(state.get_move(state.get_nu_moves() - 1).move);
}

Player& Engine::get_mcts_player()
{
    try
    {
        return dynamic_cast<Player&>(*m_player);
    }
    catch (const bad_cast& e)
    {
        throw Failure("current player is not mcts player");
    }
}

Search& Engine::get_search()
{
    return get_mcts_player().get_search();
}

//-----------------------------------------------------------------------------

} // namespace pentobi_gtp
