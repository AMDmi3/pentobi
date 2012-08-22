//-----------------------------------------------------------------------------
/** @file libpentobi_base/Game.h */
//-----------------------------------------------------------------------------

#ifndef LIBPENTOBI_BASE_GAME_H
#define LIBPENTOBI_BASE_GAME_H

#include "Board.h"
#include "BoardUpdater.h"
#include "NodeUtil.h"
#include "Tree.h"

namespace libpentobi_base {

//-----------------------------------------------------------------------------

class Game
{
public:
    Game(Variant variant);

    Game(unique_ptr<Node>& root);

    void init(Variant variant);

    void init();

    /** Initialize game from a SGF tree.
        @note If the tree contains invalid properties, future calls to
        goto_node() might throw an exception.
        @param root The root node of the SGF tree; the ownership is transfered
        to this class.
        @throws InvalidTree, if the root node contains invalid
        properties */
    void init(unique_ptr<Node>& root);

    const Board& get_board() const;

    Variant get_variant() const;

    const Node& get_current() const;

    const Node& get_root() const;

    const Tree& get_tree() const;

    /** Get the current color to play.
        This takes not into account if the current color to play still has
        moves available. */
    Color get_to_play() const;

    /** Get the next color to play that still has moves.
        The colors are tested in playing order starting with get_to_play(). */
    Color get_effective_to_play() const;

    /** @param mv
        @param always_create_new_node Always create a new child of the current
        node even if a child with the move already exists. */
    void play(ColorMove mv, bool always_create_new_node);

    void play(Color c, Move mv, bool always_create_new_node);

    /** Update game state to a node in the tree.
        @throws InvalidTree, if the game was constructed with an
        external SGF tree and the tree contained invalid property values
        (syntactically or sematically, like moves on occupied points). If an
        exception is thrown, the current node is not changed. */
    void goto_node(const Node& node);

    /** Undo the current move and go to parent node.
        @pre ! get_current().get_move().is_null()
        @pre get_current()->has_parent()
        @note Even if the implementation of this function calls goto_node(),
        it cannot throw an InvalidPropertyValue because the class Game ensures
        that the current node is always reachable via a path of nodes with
        valid move properties. */
    void undo();

    ColorMove get_move() const;

    /** See libpentobi_base::Tree::get_move_ignore_invalid() */
    ColorMove get_move_ignore_invalid() const;

    /** Add final score to root node if the current node is in the main
        variation. */
    void set_result(int score);

    void set_charset(const string& charset);

    void remove_move_annotation();

    double get_bad_move() const;

    double get_good_move() const;

    bool is_doubtful_move() const;

    bool is_interesting_move() const;

    void set_bad_move(double value = 1);

    void set_good_move(double value = 1);

    void set_doubtful_move();

    void set_interesting_move();

    string get_comment() const;

    void set_comment(const string& s);

    /** Delete the current node and its subtree and go to the parent node.
        @pre get_current().has_parent() */
    void truncate();

    void truncate_children();

    /** Replace the game tree by a new one that has the current position
        as a setup in its root node. */
    void keep_only_position();

    /** Like keep_only_position() but does not delete the children of the
        current node. */
    void keep_only_subtree();

    void make_main_variation();

    void move_up_variation();

    void move_down_variation();

    /** Delete all variations but the main variation.
        If the current node is not in the main variation it will be changed
        to the node as in libboardgame_sgf::util::back_to_main_variation() */
    void delete_all_variations();

    /** Make the current node the first child of its parent. */
    void make_first_child();

    void set_modified(bool modified);

    bool get_modified() const;

    /** Set the AP property at the root node. */
    void set_application(const string& name, const string& version = "");

    string get_player_name(Color c) const;

    void set_player_name(Color c, const string& name);

    string get_date() const;

    void set_date(const string& date);

    void set_date_today();

    bool has_setup() const;

    void add_setup(Color c, Move mv);

    void remove_setup(Color c, Move mv);

    /** See libpentobi_base::Tree::set_player() */
    void set_player(Color c);

    /** See libpentobi_base::Tree::remove_player() */
    void remove_player();

private:
    const Node* m_current;

    unique_ptr<Board> m_bd;

    Tree m_tree;

    BoardUpdater m_updater;
};

inline double Game::get_bad_move() const
{
    return m_tree.get_bad_move(*m_current);
}

inline const Board& Game::get_board() const
{
    return *m_bd;
}

inline string Game::get_comment() const
{
    return m_tree.get_comment(*m_current);
}

inline string Game::get_date() const
{
    return m_tree.get_date();
}

inline Color Game::get_effective_to_play() const
{
    return m_bd->get_effective_to_play();
}

inline const Node& Game::get_current() const
{
    return *m_current;
}

inline double Game::get_good_move() const
{
    return m_tree.get_good_move(*m_current);
}

inline bool Game::get_modified() const
{
    return m_tree.get_modified();
}

inline ColorMove Game::get_move() const
{
    return m_tree.get_move(*m_current);
}

inline ColorMove Game::get_move_ignore_invalid() const
{
    return m_tree.get_move_ignore_invalid(*m_current);
}

inline string Game::get_player_name(Color c) const
{
    return m_tree.get_player_name(c);
}

inline Color Game::get_to_play() const
{
    return m_bd->get_to_play();
}

inline const Node& Game::get_root() const
{
    return m_tree.get_root();
}

inline const Tree& Game::get_tree() const
{
    return m_tree;
}

inline bool Game::has_setup() const
{
    return libpentobi_base::node_util::has_setup(*m_current);
}

inline Variant Game::get_variant() const
{
    return m_bd->get_variant();
}

inline void Game::init()
{
    init(m_bd->get_variant());
}

inline bool Game::is_doubtful_move() const
{
    return m_tree.is_doubtful_move(*m_current);
}

inline bool Game::is_interesting_move() const
{
    return m_tree.is_interesting_move(*m_current);
}

inline void Game::make_first_child()
{
    m_tree.make_first_child(*m_current);
}

inline void Game::make_main_variation()
{
    m_tree.make_main_variation(*m_current);
}

inline void Game::move_down_variation()
{
    m_tree.move_down(*m_current);
}

inline void Game::move_up_variation()
{
    m_tree.move_up(*m_current);
}

inline void Game::play(Color c, Move mv, bool always_create_new_node)
{
    play(ColorMove(c, mv), always_create_new_node);
}

inline void Game::remove_move_annotation()
{
    m_tree.remove_move_annotation(*m_current);
}

inline void Game::set_application(const string& name, const string& version)
{
    m_tree.set_application(name, version);
}

inline void Game::set_bad_move(double value)
{
    m_tree.set_bad_move(*m_current, value);
}

inline void Game::set_charset(const string& charset)
{
    m_tree.set_charset(charset);
}

inline void Game::set_comment(const string& s)
{
    m_tree.set_comment(*m_current, s);
}

inline void Game::set_date(const string& date)
{
    m_tree.set_date(date);
}

inline void Game::set_date_today()
{
    m_tree.set_date_today();
}

inline void Game::set_doubtful_move()
{
    m_tree.set_doubtful_move(*m_current);
}

inline void Game::set_good_move(double value)
{
    m_tree.set_good_move(*m_current, value);
}

inline void Game::set_interesting_move()
{
    m_tree.set_interesting_move(*m_current);
}

inline void Game::set_modified(bool modified)
{
    m_tree.set_modified(modified);
}

inline void Game::set_player_name(Color c, const string& name)
{
    m_tree.set_player_name(c, name);
}

inline void Game::truncate_children()
{
    m_tree.remove_children(*m_current);
}

//-----------------------------------------------------------------------------

} // namespace libpentobi_base

#endif // LIBPENTOBI_BASE_GAME_H
