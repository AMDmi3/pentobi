//-----------------------------------------------------------------------------
/** @file libboardgame_mcts/SearchBase.h
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifndef LIBBOARDGAME_MCTS_SEARCH_BASE_H
#define LIBBOARDGAME_MCTS_SEARCH_BASE_H

#include <algorithm>
#include <array>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include "Atomic.h"
#include "BiasTerm.h"
#include "LastGoodReply.h"
#include "PlayerMove.h"
#include "Tree.h"
#include "TreeUtil.h"
#include "libboardgame_util/Abort.h"
#include "libboardgame_util/Barrier.h"
#include "libboardgame_util/IntervalChecker.h"
#include "libboardgame_util/Log.h"
#include "libboardgame_util/Statistics.h"
#include "libboardgame_util/StringUtil.h"
#include "libboardgame_util/TimeIntervalChecker.h"
#include "libboardgame_util/Timer.h"
#include "libboardgame_util/Unused.h"

namespace libboardgame_mcts {

using namespace std;
using libboardgame_mcts::tree_util::find_node;
using libboardgame_util::get_abort;
using libboardgame_util::log;
using libboardgame_util::time_to_string;
using libboardgame_util::to_string;
using libboardgame_util::ArrayList;
using libboardgame_util::Barrier;
using libboardgame_util::IntervalChecker;
using libboardgame_util::StatisticsBase;
using libboardgame_util::StatisticsDirtyLockFree;
using libboardgame_util::StatisticsExt;
using libboardgame_util::Timer;
using libboardgame_util::TimeIntervalChecker;
using libboardgame_util::TimeSource;

//-----------------------------------------------------------------------------

/** Default optional compile-time parameters for Search.
    See description of class Search for more information. */
struct SearchParamConstDefault
{
    /** The floating type used for mean values and counts.
        The default type is @c float for a reduced node size and performance
        gains (especially on 32-bit systems). However, using @c float sets a
        practical limit on the number of simulations before the count and mean
        values go into saturation. This maximum is given by 2^d-1 with d being
        the digits in the mantissa (=23 for IEEE 754 float's). The search will
        terminate when this number is reached. For longer searches, the code
        should be compiled with floating type @c double. */
    typedef float Float;

    /** The maximum number of players. */
    static const PlayerInt max_players = 2;

    /** The maximum length of a game. */
    static const unsigned max_moves = 1000;

    /** Compile with support for multi-threaded search.
        Disabling this slightly increases the performance if support for a
        multi-threaded search is not needed. */
    static const bool multithread = true;

    /** Use RAVE. */
    static const bool rave = false;

    /** Do not update RAVE values if the same move was played first by the
        other player.
        This improves RAVE performance in Go, where moves at the same point
        indicate that something was captured there, so the position has
        changed so significantly that the RAVE value of this move should not be
        updated to earlier positions. Setting it to true causes a slight
        slow-down in the update of the RAVE values. */
    static const bool rave_check_same = false;

    /** Enable distance weighting of RAVE updates.
        The weight decreases linearly from the start to the end of a
        simulation. The distance weight is applied in addition to the normal
        RAVE weight. */
    static const bool rave_dist_weighting = false;

    /** Enable Last-Good-Reply heuristic.
        @see LastGoodReply */
    static const bool use_lgr = false;

    /** See LastGoodReply::hash_table_size.
        Must be greater 0 if use_lgr is true. */
    static const size_t lgr_hash_table_size = 0;

    /** Use virtual loss in multi-threaded mode.
        See Chaslot et al.: Parallel Monte-Carlo Tree Search. 2008. */
    static const bool virtual_loss = false;

    /** Terminate search early if move is unlikely to change.
        See implementation of check_cannot_change(). */
    static const bool use_unlikely_change = true;

    /** The minimum count used in prior knowledge initialization of
        the children of an expanded node. */
    static constexpr Float child_min_count = 0;

    /** An evaluation value representing a 50% winning probability. */
    static constexpr Float tie_value = 0.5f;

    /** Expected simulations per second.
        If the simulations per second vary a lot, it should be a value closer
        to the lower values. This value is used, for example, to determine an
        interval for checking expensive abort conditions in deterministic mode
        (in regular mode, the simulations per second will be measured and the
        interval will be adjusted automatically). That means that in
        deterministic mode, a pessimistic low value will cause more calls to
        the expensive function but an optimistic high value will delay aborting
        the search. */
    static constexpr double expected_sim_per_sec = 100;
};

//-----------------------------------------------------------------------------

/** Game-independent Monte-Carlo tree search.
    Game-dependent functionality is added by implementing some pure virtual
    functions and by template parameters.

    RAVE (see @ref libboardgame_doc_rave) is implemented differently from
    the algorithm described in the original paper: RAVE values are not stored
    separately in the nodes but added to the normal values with a certain
    (constant) weight and up to a maximum visit count of the parent node. This
    saves memory in the tree and speeds up move selection in the in-tree phase.
    It is weaker than the original RAVE at a low number of simulations but
    seems to be equally good or even better at a high number of simulations.

    @tparam S The game-dependent state of a simulation. The state provides
    functions for move generation, evaluation of terminal positions, etc. The
    state should be thread-safe to support multiple states if multi-threading
    is used.
    @tparam M The move type. The type must be convertible to an integer by
    providing M::to_int() and M::range.
    @tparam R Optional compile-time parameters, see SearchParamConstDefault */
template<class S, class M, class R = SearchParamConstDefault>
class SearchBase
{
public:
    typedef S State;

    typedef M Move;

    typedef R SearchParamConst;

    static const bool multithread = SearchParamConst::multithread;

    typedef typename SearchParamConst::Float Float;

    typedef libboardgame_mcts::Node<M, Float, multithread> Node;

    typedef libboardgame_mcts::Tree<Node> Tree;

    typedef libboardgame_mcts::PlayerMove<M> PlayerMove;

    static const PlayerInt max_players = SearchParamConst::max_players;

    static const unsigned max_moves = SearchParamConst::max_moves;

    static const size_t lgr_hash_table_size =
            SearchParamConst::lgr_hash_table_size;

    static_assert(! SearchParamConst::use_lgr || lgr_hash_table_size > 0, "");


    /** Constructor.
        @param nu_threads
        @param memory The memory to be used for (all) the search trees. If
        zero, a default value will be used. */
    SearchBase(unsigned nu_threads, size_t memory);

    virtual ~SearchBase();


    /** @name Pure virtual functions */
    /** @{ */

    /** Create a new game-specific state to be used in a thread of the
        search. */
    virtual unique_ptr<State> create_state() = 0;

    /** Get the current number of players. */
    virtual PlayerInt get_nu_players() const = 0;

    /** Get player to play at root node of the search. */
    virtual PlayerInt get_player() const = 0;

    /** @} */ // @name


    /** @name Virtual functions */
    /** @{ */

    /** Check if the position at the root is a follow-up position of the last
        search.
        In this function, the subclass can store the game state at the root of
        the search, compare it to the the one of the last search, check if
        the current state is a follow-up position and return the move sequence
        leading from the last position to the current one, so that the search
        can check if a subtree of the last search can be reused.
        This function will be called exactly once at the beginning of each
        search. The default implementation returns false.
        The information is also used for deciding whether to clear other
        caches from the last search (e.g. Last-Good-Reply heuristic). */
    virtual bool check_followup(ArrayList<Move, max_moves>& sequence);

    virtual string get_info() const;

    virtual string get_info_ext() const;

    /** @} */ // @name


    /** @name Parameters */
    /** @{ */

    /** Interval in which a full move selection is performed in the in-tree
        phase at high parent counts.
        Takes advantage of the fact that at high parent counts the selected
        move is almost always the same as at the last visit of the node,
        so it will speed up the in-tree phase if we do a full move selection
        only at a certain intervals and play the move from the sequence of
        the last simulation otherwise.
        The default value is 1, which means that full move selection is always
        used.
        @see set_full_select_min. */
    void set_full_select_interval(unsigned n);

    /** The minimum node count to do a full move selection only in a certain
        interval.
        See set_full_select_interval(). */
    void set_full_select_min(Float n);

    /** Minimum count of a node to be expanded. */
    void set_expand_threshold(Float n);

    Float get_expand_threshold() const;

    /** Increase of the expand threshold per in-tree move played. */
    void set_expand_threshold_inc(Float n);

    Float get_expand_threshold_inc() const;

    /** Constant used in UCT bias term.
        @see BiasTerm */
    void set_bias_term_constant(Float c);

    Float get_bias_term_constant() const;

    /** Reuse the subtree from the previous search if the current position is
        a follow-up position of the previous one. */
    void set_reuse_subtree(bool enable);

    bool get_reuse_subtree() const;

    /** Reuse the tree from the previous search if the current position is
        the same position as the previous one. */
    void set_reuse_tree(bool enable);

    bool get_reuse_tree() const;

    /** Maximum parent visit count for applying RAVE. */
    void set_rave_parent_max(Float value);

    Float get_rave_parent_max() const;

    /** Maximum child value count for applying RAVE. */
    void set_rave_child_max(Float value);

    Float get_rave_child_max() const;

    /** Weight used for adding RAVE values to the node value. */
    void set_rave_weight(Float value);

    Float get_rave_weight() const;

    /** The RAVE distance weight at the end of a simulation.
        If RAVE distance weighting is used, the RAVE distance weight decreases
        linearly from 1 at the current position to this value at the end of a
        simulation. */
    void set_rave_dist_final(Float value);

    Float get_rave_dist_final() const;

    /** Value to start the tree pruning with.
        This value should be above typical count initializations if prior
        knowledge initialization is used. */
    void set_prune_count_start(Float n);

    Float get_prune_count_start() const;

    /** Total size of the trees in bytes. */
    void set_tree_memory(size_t memory);

    size_t get_tree_memory() const;

    /** Set deterministic mode.
        Note that using a fixed number of simulations instead of a time limit
        is not enough to make the search fully deterministic because the
        interval in which expensive abort conditions are checked (e.g. if the
        best move cannot change anymore) is adjusted dynamically depending on
        the number of simulations per second. In deterministic mode, a
        fixed interval is used. */
    void set_deterministic();

    /** @} */ // @name


    /** Run a search.
        @param[out] mv
        @param max_count Number of simulations to run. The search might return
        earlier if the best move cannot change anymore or if the count of the
        root node was initialized from an init tree
        @param min_simulations
        @param max_time Maximum search time. Only used if max_count is zero
        @param time_source Time source for time measurement
        @param always_search Always call the search, even if extracting the
        subtree to reuse was aborted due to max_time or util::get_abort(). If
        true, this will call a search with the partially extracted subtree,
        and the search will return immediately (because it also checks max_time
        and get_abort()). This flag should be true for regular searches because
        even a partially extracted subtree can be used for move generation, and
        false for pondering searches because here we don't need a search
        result but want to keep the full tree for reuse in a future search.
        @return @c false if no move could be generated because the position is
        a terminal position. */
    bool search(Move& mv, Float max_count, Float min_simulations,
                double max_time, TimeSource& time_source,
                bool always_search = true);

    const Tree& get_tree() const;

#if LIBBOARDGAME_DEBUG
    string dump() const;
#endif

    /** Number of simulations in the current search in all threads. */
    size_t get_nu_simulations() const;

    /** Select the move to play.
        Uses select_final(). */
    bool select_move(Move& mv) const;

    /** Select the best child of the root node after the search.
        Selects child with highest number of wins; the value is used as a
        tie-breaker for equal counts (important at very low number of
        simulations, e.g. all children have count 1 or 0). */
    const Node* select_final() const;

    State& get_state(unsigned thread_id);

    const State& get_state(unsigned thread_id) const;

    /** Set a callback function that informs the caller about the
        estimated time left.
        The callback function will be called about every 0.1s. The arguments
        of the callback function are: elapsed time, estimated remaining time. */
    void set_callback(function<void(double, double)> callback);

    /** Get evaluation for a player at root node. */
    const StatisticsDirtyLockFree<Float>& get_root_val(PlayerInt player) const;

    /** Get evaluation for get_player() at root node. */
    const StatisticsDirtyLockFree<Float>& get_root_val() const;

    /** The number of times the root node was visited.
        This is equal to the number of simulations plus the visit count
        of a subtree reused from the previous search. */
    Float get_root_visit_count() const;

    /** Create the threads used in the search.
        This cannot be done in the constructor because it uses the virtual
        function create_state(). This function will automatically be called
        before a search if the threads have not been constructed yet, but it
        is advisable to explicitely call it in the constructor of the subclass
        to save some time at the first move generation where the game clock
        might already be running. */
    void create_threads();

protected:
    struct Simulation
    {
        ArrayList<const Node*, max_moves> nodes;

        ArrayList<PlayerMove, max_moves> moves;

        array<Float, max_players> eval;
    };

    virtual void on_start_search(bool is_followup);

    virtual void on_simulation_finished(size_t n, const State& state,
                                        const Simulation& simulation);

    /** Time source for current search.
        Only valid during a search. */
    TimeSource& get_time_source();

private:
#if LIBBOARDGAME_DEBUG
    class AssertionHandler
        : public libboardgame_util::AssertionHandler
    {
    public:
        AssertionHandler(const SearchBase& search);

        ~AssertionHandler();

        void run() override;

    private:
        const SearchBase& m_search;
    };
#endif

    /** Thread-specific search state. */
    struct ThreadState
    {
        unsigned thread_id;

        unique_ptr<State> state;

        unsigned full_select_counter;

        /** Was the search in this thread terminated because the search tree
            was full? */
        bool is_out_of_mem;

        /** Number of simulations in the current search in this thread. */
        size_t nu_simulations;

        Simulation simulation;

        StatisticsExt<> stat_len;

        /** Statistics about in-tree length where the full child select was
            skipped.
            @see Search::set_full_select_interval() */
        StatisticsExt<> stat_fs_len;

        StatisticsExt<> stat_in_tree_len;

        /** Local variable for update_rave().
            Stores if a move was played for each player.
            Reused for efficiency. */
        array<array<bool, Move::range>, max_players> was_played;

        /** Local variable for update_rave().
            Stores the first time a move was played for each player.
            Elements are only defined if was_played is true.
            Reused for efficiency. */
        array<array<unsigned, Move::range>, max_players> first_play;

        ~ThreadState();
    };

    /** Thread in the parallel search.
        The thread waits for a call to start_search(), then runs the given
        search loop function (see Search::search_loop()) with the
        thread-specific search state. After start_search(),
        wait_search_finished() needs to called before calling start_search()
        again or destructing this object. */
    class Thread
    {
    public:
        typedef function<void(ThreadState&)> SearchFunc;

        ThreadState thread_state;

        Thread(SearchFunc& search_func);

        ~Thread();

        void run();

        void start_search();

        void wait_search_finished();

    private:
        SearchFunc m_search_func;

        bool m_quit = false;

        bool m_start_search_flag = false;

        bool m_search_finished_flag = false;

        Barrier m_thread_ready{2};

        mutex m_start_search_mutex;

        mutex m_search_finished_mutex;

        condition_variable m_start_search_cond;

        condition_variable m_search_finished_cond;

        unique_lock<mutex> m_search_finished_lock{m_search_finished_mutex,
                                                  defer_lock};

        thread m_thread;

        void thread_main();
    };

    unsigned m_nu_threads;

    /** See set_full_select_interval(). */
    unsigned m_full_select_interval = 1;

    /** See set_full_select_min(). */
    Float m_full_select_min = numeric_limits<Float>::max();

    Float m_expand_threshold = 0;

    Float m_expand_threshold_inc = 0;

    bool m_deterministic = false;

    bool m_reuse_subtree = true;

    bool m_reuse_tree = false;

    /** Player to play at the root node of the search. */
    PlayerInt m_player;

    /** Cached return value of get_nu_players() that stays constant during
        a search. */
    PlayerInt m_nu_players;

    /** Time of last search. */
    double m_last_time;

    Float m_prune_count_start = 16;

    Float m_rave_parent_max = 50000;

    Float m_rave_child_max = 2000;

    Float m_rave_weight = 0.3f;

    Float m_rave_dist_final = 0;

    /** Minimum simulations to perform in the current search.
        This does not include the count of simulations reused from a subtree of
        a previous search. */
    Float m_min_simulations;

    /** Maximum simulations of current search.
        This include the count of simulations reused from a subtree of a
        previous search. */
    Float m_max_count;

    size_t m_tree_memory;

    size_t m_max_nodes;

    /** Maximum time of current search. */
    double m_max_time;

    TimeSource* m_time_source;

    BiasTerm<Float> m_bias_term;

    Timer m_timer;

    vector<unique_ptr<Thread>> m_threads;

    Tree m_tmp_tree;

#if LIBBOARDGAME_DEBUG
    AssertionHandler m_assertion_handler;
#endif


    /** @name Members that are used concurrently by all threads during the
        lock-free multi-threaded search */
    /** @{ */

    Tree m_tree;

    /** See get_root_val(). */
    array<StatisticsDirtyLockFree<Float>, max_players> m_root_val;

    LastGoodReply<Move, max_players, lgr_hash_table_size, multithread> m_lgr;

    /** See get_nu_simulations(). */
    Atomic<size_t, multithread> m_nu_simulations;

    /** Mutex used in log_thread() */
    mutable mutex m_log_mutex;

    /** @} */ // @name


    function<void(double, double)> m_callback;

    ArrayList<Move, max_moves> m_followup_sequence;

    static size_t get_max_nodes(size_t memory);

    bool check_abort(const ThreadState& thread_state, Float root_count) const;

    bool check_abort_expensive(ThreadState& thread_state) const;

    void check_create_threads();

    bool check_cannot_change(ThreadState& thread_state, Float remaining) const;

    bool estimate_reused_root_val(Tree& tree, const Node& root, Float& value,
                                  Float& count);

    bool expand_node(ThreadState& thread_state, const Node& node,
                     const Node*& best_child);

    void log_thread(const ThreadState& thread_state, const string& s) const;

    void playout(ThreadState& thread_state);

    void play_in_tree(ThreadState& thread_state);

    bool prune(TimeSource& time_source, double time, double max_time,
               Float prune_min_count, Float& new_prune_min_count);

    void search_loop(ThreadState& thread_state);

    const Node* select_child(const Node& node);

    void update_lgr(ThreadState& thread_state);

    void update_rave(ThreadState& thread_state);

    void update_values(ThreadState& thread_state);
};


template<class S, class M, class R>
SearchBase<S, M, R>::ThreadState::~ThreadState()
{
}

template<class S, class M, class R>
SearchBase<S, M, R>::Thread::Thread(SearchFunc& search_func)
    : m_search_func(search_func)
{ }

template<class S, class M, class R>
SearchBase<S, M, R>::Thread::~Thread()
{
    if (! m_thread.joinable())
        return;
    m_quit = true;
    {
        lock_guard<mutex> lock(m_start_search_mutex);
        m_start_search_flag = true;
    }
    m_start_search_cond.notify_one();
    m_thread.join();
}

template<class S, class M, class R>
void SearchBase<S, M, R>::Thread::run()
{
    m_thread = thread(bind(&Thread::thread_main, this));
    m_thread_ready.wait();
}

template<class S, class M, class R>
void SearchBase<S, M, R>::Thread::start_search()
{
    LIBBOARDGAME_ASSERT(m_thread.joinable());
    m_search_finished_lock.lock();
    {
        lock_guard<mutex> lock(m_start_search_mutex);
        m_start_search_flag = true;
    }
    m_start_search_cond.notify_one();
}

template<class S, class M, class R>
void SearchBase<S, M, R>::Thread::thread_main()
{
    unique_lock<mutex> lock(m_start_search_mutex);
    m_thread_ready.wait();
    while (true)
    {
        while (! m_start_search_flag)
            m_start_search_cond.wait(lock);
        m_start_search_flag = false;
        if (m_quit)
            break;
        m_search_func(thread_state);
        {
            lock_guard<mutex> lock(m_search_finished_mutex);
            m_search_finished_flag = true;
        }
        m_search_finished_cond.notify_one();
    }
}

template<class S, class M, class R>
void SearchBase<S, M, R>::Thread::wait_search_finished()
{
    LIBBOARDGAME_ASSERT(m_thread.joinable());
    while (! m_search_finished_flag)
        m_search_finished_cond.wait(m_search_finished_lock);
    m_search_finished_flag = false;
    m_search_finished_lock.unlock();
}


#if LIBBOARDGAME_DEBUG
template<class S, class M, class R>
SearchBase<S, M, R>::AssertionHandler::AssertionHandler(const SearchBase& search)
    : m_search(search)
{
}

template<class S, class M, class R>
SearchBase<S, M, R>::AssertionHandler::~AssertionHandler()
{
}

template<class S, class M, class R>
void SearchBase<S, M, R>::AssertionHandler::run()
{
    log(m_search.dump());
}
#endif // LIBBOARDGAME_DEBUG


template<class S, class M, class R>
SearchBase<S, M, R>::SearchBase(unsigned nu_threads, size_t memory)
    : m_nu_threads(nu_threads),
      m_tree_memory(memory == 0 ? 256000000 : memory),
      m_max_nodes(get_max_nodes(m_tree_memory)),
      m_bias_term(0),
      m_tmp_tree(m_max_nodes, m_nu_threads),
#if LIBBOARDGAME_DEBUG
      m_assertion_handler(*this),
#endif
      m_tree(m_max_nodes, m_nu_threads)
{
}

template<class S, class M, class R>
SearchBase<S, M, R>::~SearchBase()
{
}

template<class S, class M, class R>
bool SearchBase<S, M, R>::check_abort(const ThreadState& thread_state,
                                      Float root_count) const
{
    if (m_max_count > 0 && root_count >= m_max_count)
    {
        log_thread(thread_state, "Maximum count reached");
        return true;
    }
    return false;
}

template<class S, class M, class R>
bool SearchBase<S, M, R>::check_abort_expensive(
        ThreadState& thread_state) const
{
    if (get_abort())
    {
        log_thread(thread_state, "Search aborted");
        return true;
    }
    auto time = m_timer();
    if (! m_deterministic && time < 0.1)
        // Simulations per second might be inaccurate for very small times
        return false;
    double simulations_per_sec;
    if (time == 0)
        simulations_per_sec = SearchParamConst::expected_sim_per_sec;
    else
    {
        size_t nu_simulations = m_nu_simulations.load(memory_order_relaxed);
        simulations_per_sec = double(nu_simulations) / time;
    }
    double remaining_time;
    Float remaining_simulations;
    if (m_max_count == 0)
    {
        // Search uses time limit
        if (time > m_max_time)
        {
            log_thread(thread_state, "Maximum time reached");
            return true;
        }
        remaining_time = m_max_time - time;
        remaining_simulations = Float(remaining_time * simulations_per_sec);
    }
    else
    {
        // Search uses count limit
        auto count = m_tree.get_root().get_visit_count();
        remaining_simulations = m_max_count - count;
        remaining_time = remaining_simulations / simulations_per_sec;
    }
    if (thread_state.thread_id == 0 && m_callback)
        m_callback(time, remaining_time);
    if (check_cannot_change(thread_state, remaining_simulations))
        return true;
    return false;
}

template<class S, class M, class R>
bool SearchBase<S, M, R>::check_cannot_change(ThreadState& thread_state,
                                              Float remaining) const
{
    // select_final() selects move with highest number of wins.
    Float max_wins = 0;
    Float second_max = 0;
    for (auto& i : m_tree.get_root_children())
    {
        Float wins = i.get_value() * i.get_value_count();
        if (wins > max_wins)
        {
            second_max = max_wins;
            max_wins = wins;
        }
    }
    Float diff = max_wins - second_max;
    if (SearchParamConst::use_unlikely_change)
    {
        // Weight remaining number of simulations with current global win rate,
        // but not less than 10%
        auto& root_val = m_root_val[m_player];
        Float win_rate;
        if (root_val.get_count() > 100)
        {
            win_rate = root_val.get_mean();
            if (win_rate < 0.1f)
                win_rate = 0.1f;
        }
        else
            win_rate = 1; // Not enough statistics
        if (diff < win_rate * remaining)
            return false;
    }
    else if (diff < remaining)
        return false;
    log_thread(thread_state, "Move will not change");
    return true;
}

template<class S, class M, class R>
void SearchBase<S, M, R>::check_create_threads()
{
    if (m_nu_threads != m_threads.size())
        create_threads();
}

template<class S, class M, class R>
bool SearchBase<S, M, R>::check_followup(ArrayList<Move, max_moves>& sequence)
{
    LIBBOARDGAME_UNUSED(sequence);
    return false;
}

template<class S, class M, class R>
void SearchBase<S, M, R>::create_threads()
{
    if (! multithread && m_nu_threads > 1)
        throw runtime_error("libboardgame_mcts::Search was compiled"
                            " without support for multithreading");
    log("Creating ", m_nu_threads, " threads");
    m_threads.clear();
    m_threads.reserve(m_nu_threads);
    auto search_func =
        static_cast<typename Thread::SearchFunc>(
                          bind(&SearchBase::search_loop, this, placeholders::_1));
    for (unsigned i = 0; i < m_nu_threads; ++i)
    {
        unique_ptr<Thread> t(new Thread(search_func));
        auto& thread_state = t->thread_state;
        thread_state.thread_id = i;
        thread_state.state = create_state();
        for (PlayerInt j = 0; j < max_players; ++j)
            thread_state.was_played[j].fill(false);
        if (i > 0)
            t->run();
        m_threads.push_back(move(t));
    }
}

#if LIBBOARDGAME_DEBUG
template<class S, class M, class R>
string SearchBase<S, M, R>::dump() const
{
    ostringstream s;
    for (unsigned i = 0; i < m_nu_threads; ++i)
    {
        s << "Thread state " << i << ":\n"
          << get_state(i).dump();
    }
    return s.str();
}
#endif

template<class S, class M, class R>
bool SearchBase<S, M, R>::expand_node(ThreadState& thread_state,
                                      const Node& node,
                                      const Node*& best_child)
{
    auto& state = *thread_state.state;
    // SearchParamConst::child_min_count is currently declared as unsigned int
    // because MSVC does not support constexpr Float yet
    Float min_count = static_cast<Float>(SearchParamConst::child_min_count);
    auto thread_id = thread_state.thread_id;
    typename Tree::NodeExpander expander(thread_id, m_tree, min_count);
    state.gen_children(expander, m_root_val[state.get_player()].get_mean());
    if (! expander.is_tree_full())
    {
        expander.link_children(m_tree, node);
        best_child = expander.get_best_child();
        return true;
    }
    return false;
}

template<class S, class M, class R>
inline auto SearchBase<S, M, R>::get_bias_term_constant() const -> Float
{
    return m_bias_term.get_bias_term_constant();
}

template<class S, class M, class R>
inline auto SearchBase<S, M, R>::get_expand_threshold() const -> Float
{
    return m_expand_threshold;
}

template<class S, class M, class R>
inline auto SearchBase<S, M, R>::get_expand_threshold_inc() const -> Float
{
    return m_expand_threshold_inc;
}

template<class S, class M, class R>
size_t SearchBase<S, M, R>::get_max_nodes(size_t memory)
{
    // Memory is used for 2 trees (m_tree and m_tmp_tree)
    size_t max_nodes = memory / sizeof(Node) / 2;
    // It doesn't make sense to set max_nodes higher than what can be accessed
    // with NodeIdx
    max_nodes =
        min(max_nodes, static_cast<size_t>(numeric_limits<NodeIdx>::max()));
    log("Search tree size: 2 x ", max_nodes, " nodes");
    return max_nodes;
}

template<class S, class M, class R>
inline size_t SearchBase<S, M, R>::get_nu_simulations() const
{
    return m_nu_simulations;
}

template<class S, class M, class R>
inline auto SearchBase<S, M, R>::get_root_val(PlayerInt player) const
-> const StatisticsDirtyLockFree<Float>&
{
    LIBBOARDGAME_ASSERT(player < m_nu_players);
    return m_root_val[player];
}

template<class S, class M, class R>
inline auto SearchBase<S, M, R>::get_root_val() const
-> const StatisticsDirtyLockFree<Float>&
{
    return get_root_val(get_player());
}

template<class S, class M, class R>
inline auto SearchBase<S, M, R>::get_root_visit_count() const -> Float
{
    return m_tree.get_root().get_visit_count();
}

template<class S, class M, class R>
inline auto SearchBase<S, M, R>::get_prune_count_start() const -> Float
{
    return m_prune_count_start;
}

template<class S, class M, class R>
inline auto SearchBase<S, M, R>::get_rave_dist_final() const -> Float
{
    return m_rave_dist_final;
}

template<class S, class M, class R>
inline auto SearchBase<S, M, R>::get_rave_parent_max() const -> Float
{
    return m_rave_parent_max;
}

template<class S, class M, class R>
inline auto SearchBase<S, M, R>::get_rave_child_max() const -> Float
{
    return m_rave_child_max;
}

template<class S, class M, class R>
inline auto SearchBase<S, M, R>::get_rave_weight() const -> Float
{
    return m_rave_weight;
}

template<class S, class M, class R>
inline bool SearchBase<S, M, R>::get_reuse_subtree() const
{
    return m_reuse_subtree;
}

template<class S, class M, class R>
inline bool SearchBase<S, M, R>::get_reuse_tree() const
{
    return m_reuse_tree;
}

template<class S, class M, class R>
inline S& SearchBase<S, M, R>::get_state(unsigned thread_id)
{
    LIBBOARDGAME_ASSERT(thread_id < m_threads.size());
    return *m_threads[thread_id]->thread_state.state;
}

template<class S, class M, class R>
inline const S& SearchBase<S, M, R>::get_state(unsigned thread_id) const
{
    LIBBOARDGAME_ASSERT(thread_id < m_threads.size());
    return *m_threads[thread_id]->thread_state.state;
}

template<class S, class M, class R>
inline TimeSource& SearchBase<S, M, R>::get_time_source()
{
    LIBBOARDGAME_ASSERT(m_time_source != 0);
    return *m_time_source;
}

template<class S, class M, class R>
size_t SearchBase<S, M, R>::get_tree_memory() const
{
    return m_tree_memory;
}

template<class S, class M, class R>
inline auto SearchBase<S, M, R>::get_tree() const -> const Tree&
{
    return m_tree;
}

template<class S, class M, class R>
void SearchBase<S, M, R>::log_thread(const ThreadState& thread_state,
                                 const string& s) const
{
    lock_guard<mutex> lock(m_log_mutex);
    log('[', thread_state.thread_id, "] ", s);
}

template<class S, class M, class R>
void SearchBase<S, M, R>::on_simulation_finished(size_t n, const State& state,
                                                 const Simulation& simulation)
{
    LIBBOARDGAME_UNUSED(n);
    LIBBOARDGAME_UNUSED(state);
    LIBBOARDGAME_UNUSED(simulation);
    // Default implementation does nothing
}

template<class S, class M, class R>
void SearchBase<S, M, R>::on_start_search(bool is_followup)
{
    // Default implementation does nothing
    LIBBOARDGAME_UNUSED(is_followup);
}

template<class S, class M, class R>
void SearchBase<S, M, R>::playout(ThreadState& thread_state)
{
    auto& state = *thread_state.state;
    state.start_playout();
    auto& simulation = thread_state.simulation;
    auto& moves = simulation.moves;
    auto nu_moves = moves.size();
    Move last = nu_moves > 0 ? moves[nu_moves - 1].move : Move::null();
    Move second_last = nu_moves > 1 ? moves[nu_moves - 2].move : Move::null();
    while (true)
    {
        PlayerMove mv;
        if (! state.gen_playout_move(m_lgr, last, second_last, mv))
            break;
        simulation.moves.push_back(mv);
        state.play_playout(mv.move);
        second_last = last;
        last = mv.move;
    }
}

template<class S, class M, class R>
void SearchBase<S, M, R>::play_in_tree(ThreadState& thread_state)
{
    auto& state = *thread_state.state;
    auto& simulation = thread_state.simulation;
    auto& root = m_tree.get_root();
    auto node = &root;
    m_tree.inc_visit_count(*node);
    Float expand_threshold = m_expand_threshold;
    if (thread_state.full_select_counter > 0)
    {
        // Don't do a full move select but play same sequence as last time
        // as long as the node count is above m_full_select_min.
        --thread_state.full_select_counter;
        unsigned depth = 0;
        while (node->has_children())
        {
            if (node->get_visit_count() <= m_full_select_min
                    || depth + 1 >= simulation.nodes.size())
                break;
            node = simulation.nodes[depth + 1];
            m_tree.inc_visit_count(*node);
            if (multithread && SearchParamConst::virtual_loss)
                m_tree.add_value(*node, 0);
            state.play_in_tree(node->get_move());
            ++depth;
            expand_threshold += m_expand_threshold_inc;
        }
        simulation.nodes.resize(depth + 1);
        simulation.moves.resize(depth);
    }
    else
    {
        simulation.nodes.assign(node);
        simulation.moves.clear();
        thread_state.full_select_counter = m_full_select_interval;
    }
    thread_state.stat_fs_len.add(double(simulation.moves.size()));
    while (node->has_children())
    {
        node = select_child(*node);
        m_tree.inc_visit_count(*node);
        if (multithread && SearchParamConst::virtual_loss)
            m_tree.add_value(*node, 0);
        simulation.nodes.push_back(node);
        Move mv = node->get_move();
        simulation.moves.push_back(PlayerMove(state.get_player(), mv));
        state.play_in_tree(mv);
        expand_threshold += m_expand_threshold_inc;
    }
    state.finish_in_tree();
    if (node->get_visit_count() > expand_threshold)
    {
        if (! expand_node(thread_state, *node, node))
            thread_state.is_out_of_mem = true;
        else if (node)
        {
            simulation.nodes.push_back(node);
            Move mv = node->get_move();
            simulation.moves.push_back(PlayerMove(state.get_player(), mv));
            state.play_expanded_child(mv);
        }
    }
    thread_state.stat_in_tree_len.add(double(simulation.moves.size()));
}

template<class S, class M, class R>
string SearchBase<S, M, R>::get_info() const
{
    auto& root = m_tree.get_root();
    if (m_threads.empty())
        return string();
    auto& thread_state = m_threads[0]->thread_state;
    ostringstream s;
    if (thread_state.nu_simulations == 0)
    {
        s << "No simulations in thread 0\n";
        return s.str();
    }
    s << fixed << setprecision(2) << "Val: " << get_root_val().get_mean()
      << setprecision(0) << ", ValCnt: " << get_root_val().get_count()
      << ", VstCnt: " << get_root_visit_count()
      << ", Sim: " << m_nu_simulations;
    auto child = select_final();
    if (child && root.get_visit_count() > 0)
        s << setprecision(1) << ", Chld: "
          << (100 * child->get_visit_count() / root.get_visit_count())
          << '%';
    s << "\nNds: " << m_tree.get_nu_nodes()
      << ", Tm: " << time_to_string(m_last_time)
      << setprecision(0) << ", Sim/s: "
      << (double(m_nu_simulations) / m_last_time)
      << ", Len: " << thread_state.stat_len.to_string(true, 1, true)
      << "\nDp: " << thread_state.stat_in_tree_len.to_string(true, 1, true)
      << ", FS: " << thread_state.stat_fs_len.to_string(true, 1, true)
      << "\n";
    return s.str();
}

template<class S, class M, class R>
string SearchBase<S, M, R>::get_info_ext() const
{
    return string();
}

template<class S, class M, class R>
bool SearchBase<S, M, R>::prune(TimeSource& time_source, double time,
                            double max_time, Float prune_min_count,
                            Float& new_prune_min_count)
{
    Timer timer(time_source);
    TimeIntervalChecker interval_checker(time_source, max_time);
    if (m_deterministic)
        interval_checker.set_deterministic(1000000);
    m_tmp_tree.clear(m_tree.get_root().get_value());
    if (! m_tree.copy_subtree(m_tmp_tree, m_tmp_tree.get_root(),
                              m_tree.get_root(), prune_min_count, true,
                              &interval_checker))
    {
        log("Pruning aborted");
        return false;
    }
    int percent = int(m_tmp_tree.get_nu_nodes() * 100 / m_tree.get_nu_nodes());
    log("Pruning MinCnt: ", prune_min_count, ", AtTm: ", time, ", Nds: ",
        m_tmp_tree.get_nu_nodes(), " (", percent, "%), Tm: ", timer());
    m_tree.swap(m_tmp_tree);
    if (percent > 50)
    {
        if (prune_min_count >= 0.5 * numeric_limits<Float>::max())
            return false;
        new_prune_min_count = prune_min_count * 2;
        return true;
    }
    else
    {
        new_prune_min_count = prune_min_count;
        return true;
    }
}

/** Estimate the value and count of a root node from its children.
    After reusing a subtree, we don't know the value of the root because nodes
    only store the value of moves. To estimate the root value, we use the child
    with the highest visit count. */
template<class S, class M, class R>
bool SearchBase<S, M, R>::estimate_reused_root_val(Tree& tree,
                                                   const Node& root,
                                                   Float& value, Float& count)
{
    const Node* best = nullptr;
    Float max_count = 0;
    for (auto& i : tree.get_children(root))
        if (i.get_visit_count() > max_count)
        {
            best = &i;
            max_count = i.get_visit_count();
        }
    if (! best)
        return false;
    value = best->get_value();
    count = best->get_value_count();
    return count > 0;
}

template<class S, class M, class R>
bool SearchBase<S, M, R>::search(Move& mv, Float max_count,
                                 Float min_simulations, double max_time,
                                 TimeSource& time_source, bool always_search)
{
    check_create_threads();
    bool is_followup = check_followup(m_followup_sequence);
    on_start_search(is_followup);
    if (max_count > 0)
        // A fixed number of simulations means that no time limit is used, but
        // max_time is still used at some places in the code, so we set it to
        // infinity
        max_time = numeric_limits<double>::max();
    m_player = get_player();
    m_nu_players = get_nu_players();
    bool clear_tree = true;
    bool is_same = false;
    if (is_followup && m_followup_sequence.empty())
    {
        is_same = true;
        is_followup = false;
    }
    if (is_same || (is_followup && m_followup_sequence.size() <= m_nu_players))
    {
        // Use root_val from last search but with a count of max. 100
        for (PlayerInt i = 0; i < m_nu_players; ++i)
            if (m_root_val[i].get_count() > 100)
                m_root_val[i].init(m_root_val[i].get_mean(), 100);
    }
    else
        for (PlayerInt i = 0; i < m_nu_players; ++i)
            m_root_val[i].init(SearchParamConst::tie_value, 1);
    if ((m_reuse_subtree && is_followup) || (m_reuse_tree && is_same))
    {
        size_t tree_nodes = m_tree.get_nu_nodes();
        if (m_followup_sequence.empty())
        {
            if (tree_nodes > 1)
                log("Reusing all ", tree_nodes, "nodes (count=",
                    m_tree.get_root().get_visit_count(), ")");
        }
        else
        {
            Timer timer(time_source);
            m_tmp_tree.clear(SearchParamConst::tie_value);
            auto node = find_node(m_tree, m_followup_sequence);
            if (node)
            {
                TimeIntervalChecker interval_checker(time_source, max_time);
                if (m_deterministic)
                    interval_checker.set_deterministic(1000000);
                bool aborted =
                    ! m_tree.extract_subtree(m_tmp_tree, *node, true,
                                             &interval_checker);
                auto& tmp_tree_root = m_tmp_tree.get_root();
                if (! is_same)
                {
                    Float value, count;
                    if (estimate_reused_root_val(m_tmp_tree, tmp_tree_root,
                                                 value, count))
                        m_root_val[m_player].add(value, count);
                }
                if (aborted && ! always_search)
                    return false;
                size_t tmp_tree_nodes = m_tmp_tree.get_nu_nodes();
                if (tree_nodes > 1 && tmp_tree_nodes > 1)
                {
                    double time = timer();
                    double reuse = double(tmp_tree_nodes) / double(tree_nodes);
                    double percent = 100 * reuse;
                    log("Reusing ", tmp_tree_nodes, " nodes (", std::fixed,
                        setprecision(1), percent, "% tm=", setprecision(4),
                        time, ")");
                    m_tree.swap(m_tmp_tree);
                    clear_tree = false;
                    max_time -= time;
                    if (max_time < 0)
                        max_time = 0;
                }
            }
        }
    }
    if (clear_tree)
        m_tree.clear(SearchParamConst::tie_value);

    m_timer.reset(time_source);
    m_time_source = &time_source;
    if (SearchParamConst::use_lgr && ! is_followup)
        m_lgr.init(m_nu_players);
    for (auto& i : m_threads)
    {
        auto& thread_state = i->thread_state;
        thread_state.nu_simulations = 0;
        thread_state.full_select_counter = 0;
        thread_state.stat_len.clear();
        thread_state.stat_in_tree_len.clear();
        thread_state.stat_fs_len.clear();
        thread_state.state->start_search();
    }
    m_max_count = max_count;
    m_min_simulations = min_simulations;
    m_max_time = max_time;
    m_nu_simulations.store(0);
    Float prune_min_count = get_prune_count_start();

    // Don't use multi-threading for very short searches (less than 0.5s).
    // There are too many lost updates at the beginning (e.g. if all threads
    // expand the root node and only the children of the last thread are used)
    auto reused_count = m_tree.get_root().get_visit_count();
    unsigned nu_threads = m_nu_threads;
    double expected_time;
    if (max_count > 0)
        expected_time =
                (max_count - reused_count)
                / SearchParamConst::expected_sim_per_sec;
    else
        expected_time = max_time;
    if (nu_threads > 1 && expected_time < 0.5)
    {
        log("Using single-threading for very short search");
        nu_threads = 1;
    }

    auto& thread_state_0 = m_threads[0]->thread_state;
    auto& root = m_tree.get_root();
    if (! root.has_children())
    {
        const Node* best_child;
        thread_state_0.state->start_simulation(0);
        thread_state_0.state->finish_in_tree();
        expand_node(thread_state_0, root, best_child);
    }

    if (root.get_nu_children() == 0)
        log("No legal moves at root");
    else if (root.get_nu_children() == 1 && min_simulations == 0)
        log("Root has only one child");
    else
        while (true)
        {
            for (unsigned i = 1; i < nu_threads; ++i)
                m_threads[i]->start_search();
            search_loop(thread_state_0);
            for (unsigned i = 1; i < nu_threads; ++i)
                m_threads[i]->wait_search_finished();
            bool is_out_of_mem = false;
            for (unsigned i = 0; i < nu_threads; ++i)
                if (m_threads[i]->thread_state.is_out_of_mem)
                {
                    is_out_of_mem = true;
                    break;
                }
            if (! is_out_of_mem)
                break;
            double time = m_timer();
            if (! prune(time_source, time, max_time - time, prune_min_count,
                        prune_min_count))
            {
                log("Aborting search because pruning failed.");
                break;
            }
        }
    static_assert(numeric_limits<Float>::radix == 2, "");
    if (m_tree.get_root().get_visit_count()
            >= (size_t(1) << numeric_limits<Float>::digits) - 1)
    {
        log("Warning: Maximum count supported by floating type exceeded");
        return true;
    }

    m_last_time = m_timer();
    log(get_info());
    bool result = select_move(mv);
    m_time_source = nullptr;
    return result;
}

template<class S, class M, class R>
void SearchBase<S, M, R>::search_loop(ThreadState& thread_state)
{
    auto& state = *thread_state.state;
    auto& simulation = thread_state.simulation;
    simulation.nodes.clear();
    simulation.moves.clear();
    double time_interval = 0.1;
    if (m_max_count == 0 && m_max_time < 1)
        time_interval = 0.1 * m_max_time;
    IntervalChecker expensive_abort_checker(
                *m_time_source, time_interval,
                bind(&SearchBase::check_abort_expensive, this,
                     ref(thread_state)));
    if (m_deterministic)
    {
        unsigned interval =
            static_cast<unsigned>(
                    max(1.0, SearchParamConst::expected_sim_per_sec / 5.0));
        expensive_abort_checker.set_deterministic(interval);
    }
    while (true)
    {
        thread_state.is_out_of_mem = false;
        auto count = m_tree.get_root().get_visit_count();
        if ((check_abort(thread_state, count) || expensive_abort_checker())
                && m_nu_simulations >= m_min_simulations)
            break;
        auto nu_simulations = m_nu_simulations.fetch_add(1);
        ++thread_state.nu_simulations;
        state.start_simulation(nu_simulations);
        play_in_tree(thread_state);
        if (thread_state.is_out_of_mem)
            break;
        playout(thread_state);
        state.evaluate_playout(simulation.eval);
        thread_state.stat_len.add(double(simulation.moves.size()));
        update_values(thread_state);
        if (SearchParamConst::rave)
            update_rave(thread_state);
        if (SearchParamConst::use_lgr)
            update_lgr(thread_state);
        on_simulation_finished(nu_simulations, state, simulation);
    }
}

template<class S, class M, class R>
inline auto SearchBase<S, M, R>::select_child(const Node& node) -> const Node*
{
    auto children = m_tree.get_children(node);
    LIBBOARDGAME_ASSERT(! children.empty());
    m_bias_term.start_iteration(node.get_visit_count());
    // SearchParamConst::child_min_count is currently declared as unsigned int
    // because MSVC does not support constexpr Float yet
    auto min_count = static_cast<Float>(SearchParamConst::child_min_count);
    auto bias_upper_limit = m_bias_term.get(min_count);
    auto i = children.begin();
    auto value = i->get_value();
    value += m_bias_term.get(i->get_value_count());
    auto best_value = value;
    auto best_child = i;
    auto limit = best_value - bias_upper_limit;
    while (++i != children.end())
    {
        value = i->get_value();
        if (value < limit)
            continue;
        value += m_bias_term.get(i->get_value_count());
        if (value > best_value)
        {
            best_value = value;
            best_child = i;
            limit = best_value - bias_upper_limit;
        }
    }
    return best_child;
}

template<class S, class M, class R>
auto SearchBase<S, M, R>::select_final() const-> const Node*
{
    // Select the child with the highest number of wins
    auto children = m_tree.get_children(m_tree.get_root());
    if (children.empty())
        return nullptr;
    auto i = children.begin();
    auto best_child = i;
    auto max_wins = i->get_value_count() * i->get_value();
    while (++i != children.end())
    {
        auto wins = i->get_value_count() * i->get_value();
        if (wins > max_wins)
        {
            max_wins = wins;
            best_child = i;
        }
    }
    return best_child;
}

template<class S, class M, class R>
bool SearchBase<S, M, R>::select_move(Move& mv) const
{
    auto child = select_final();
    if (child)
    {
        mv = child->get_move();
        return true;
    }
    else
        return false;
}

template<class S, class M, class R>
void SearchBase<S, M, R>::set_bias_term_constant(Float c)
{
    m_bias_term.set_bias_term_constant(c);
}

template<class S, class M, class R>
void SearchBase<S, M, R>::set_callback(function<void(double, double)> callback)
{
    m_callback = callback;
}

template<class S, class M, class R>
void SearchBase<S, M, R>::set_deterministic()
{
    m_deterministic = true;
}

template<class S, class M, class R>
void SearchBase<S, M, R>::set_expand_threshold(Float n)
{
    m_expand_threshold = n;
}

template<class S, class M, class R>
void SearchBase<S, M, R>::set_expand_threshold_inc(Float n)
{
    m_expand_threshold_inc = n;
}

template<class S, class M, class R>
void SearchBase<S, M, R>::set_full_select_interval(unsigned n)
{
    m_full_select_interval = n;
}

template<class S, class M, class R>
void SearchBase<S, M, R>::set_full_select_min(Float n)
{
    m_full_select_min = n;
}

template<class S, class M, class R>
void SearchBase<S, M, R>::set_prune_count_start(Float n)
{
    m_prune_count_start = n;
}

template<class S, class M, class R>
void SearchBase<S, M, R>::set_rave_dist_final(Float v)
{
    m_rave_dist_final = v;
}

template<class S, class M, class R>
void SearchBase<S, M, R>::set_rave_parent_max(Float n)
{
    m_rave_parent_max = n;
}

template<class S, class M, class R>
void SearchBase<S, M, R>::set_rave_child_max(Float n)
{
    m_rave_child_max = n;
}

template<class S, class M, class R>
void SearchBase<S, M, R>::set_rave_weight(Float v)
{
    m_rave_weight = v;
}

template<class S, class M, class R>
void SearchBase<S, M, R>::set_reuse_subtree(bool enable)
{
    m_reuse_subtree = enable;
}

template<class S, class M, class R>
void SearchBase<S, M, R>::set_reuse_tree(bool enable)
{
    m_reuse_tree = enable;
}

template<class S, class M, class R>
void SearchBase<S, M, R>::set_tree_memory(size_t memory)
{
    m_tree_memory = memory;
    m_max_nodes = get_max_nodes(memory);
    m_tree.set_max_nodes(m_max_nodes);
    m_tmp_tree.set_max_nodes(m_max_nodes);
}

template<class S, class M, class R>
void SearchBase<S, M, R>::update_lgr(ThreadState& thread_state)
{
    const auto& simulation = thread_state.simulation;
    auto& eval = simulation.eval;
    auto max_eval = eval[0];
    for (PlayerInt i = 1; i < m_nu_players; ++i)
        max_eval = max(eval[i], max_eval);
    array<bool,max_players> is_winner;
    for (PlayerInt i = 0; i < m_nu_players; ++i)
        // Note: this handles a draw as a win. Without additional information
        // we cannot make a good decision how to handle draws and some
        // experiments in Blokus Duo showed (with low confidence) that treating
        // them as a win for both players is slightly better than treating them
        // as a loss for both.
        is_winner[i] = (eval[i] == max_eval);
    auto& moves = simulation.moves;
    auto nu_moves = moves.size();
    Move last = moves.get_unchecked(0).move;
    Move second_last = Move::null();
    for (unsigned i = 1; i < nu_moves; ++i)
    {
        PlayerMove reply = moves[i];
        PlayerInt player = reply.player;
        Move mv = reply.move;
        if (is_winner[player])
            m_lgr.store(player, last, second_last, mv);
        else
            m_lgr.forget(player, last, second_last, mv);
        second_last = last;
        last = mv;
    }
}

template<class S, class M, class R>
void SearchBase<S, M, R>::update_rave(ThreadState& thread_state)
{
    const auto& state = *thread_state.state;
    auto& moves = thread_state.simulation.moves;
    auto nu_moves = static_cast<unsigned>(moves.size());
    if (nu_moves == 0)
        return;
    auto& was_played = thread_state.was_played;
    auto& first_play = thread_state.first_play;
    auto& nodes = thread_state.simulation.nodes;
    unsigned nu_nodes = static_cast<unsigned>(nodes.size());
    unsigned i = nu_moves - 1;
    // nu_nodes is at least 2 (including root) because the case of no legal
    // moves at the root is already handled before running any simulations.
    LIBBOARDGAME_ASSERT(nu_nodes > 1);

    // Fill was_played and first_play with information from playout moves
    for ( ; i >= nu_nodes - 1; --i)
    {
        auto mv = moves[i];
        if (! state.skip_rave(mv.move))
        {
            was_played[mv.player][mv.move.to_int()] = true;
            first_play[mv.player][mv.move.to_int()] = i;
        }
    }

    // Add RAVE values to children of nodes of current simulation
    while (true)
    {
        const auto node = nodes[i];
        if (node->get_visit_count() > m_rave_parent_max)
            break;
        auto mv = moves[i];
        auto player = mv.player;
        Float dist_weight_factor;
        if (SearchParamConst::rave_dist_weighting)
            dist_weight_factor = (1 - m_rave_dist_final) / Float(nu_moves - i);
        auto children = m_tree.get_children(*node);
        LIBBOARDGAME_ASSERT(! children.empty());
        auto it = children.begin();
        do
        {
            auto mv = it->get_move();
            if (! was_played[player][mv.to_int()]
                    || it->get_value_count() > m_rave_child_max)
                continue;
            auto first = first_play[player][mv.to_int()];
            LIBBOARDGAME_ASSERT(first > i);
            if (SearchParamConst::rave_check_same)
            {
                bool other_played_same = false;
                for (PlayerInt j = 0; j < m_nu_players; ++j)
                    if (j != player && was_played[j][mv.to_int()])
                    {
                        auto first_other = first_play[j][mv.to_int()];
                        if (first_other >= i && first_other <= first)
                        {
                            other_played_same = true;
                            break;
                        }
                    }
                if (other_played_same)
                    continue;
            }
            Float weight = m_rave_weight;
            if (SearchParamConst::rave_dist_weighting)
                weight *= 1 - Float(first - i) * dist_weight_factor;
            m_tree.add_value(*it, thread_state.simulation.eval[player], weight);
        }
        while (++it != children.end());
        if (i == 0)
            break;
        if (! state.skip_rave(mv.move))
        {
            was_played[player][mv.move.to_int()] = true;
            first_play[player][mv.move.to_int()] = i;
        }
        --i;
    }

    // Reset was_played
    while (true)
    {
        ++i;
        if (i >= nu_moves)
            break;
        auto mv = moves[i];
        was_played[mv.player][mv.move.to_int()] = false;
    }
}

template<class S, class M, class R>
void SearchBase<S, M, R>::update_values(ThreadState& thread_state)
{
    const auto& simulation = thread_state.simulation;
    auto& nodes = simulation.nodes;
    auto& eval = simulation.eval;
    unsigned nu_nodes = static_cast<unsigned>(nodes.size());
    for (unsigned i = 1; i < nu_nodes; ++i)
    {
        auto& node = *nodes[i];
        auto mv = simulation.moves[i - 1];
        if (multithread && SearchParamConst::virtual_loss)
            // Note that this could become problematic if the number of threads
            // is large. The lock-free algorithm intentionally ignores lost or
            // partial updates to run faster. But the probability that adding
            // a virtual loss is lost is not the same as that its removal is
            // lost because the removal is done in this function with many
            // calls to add_value() but the adding is done in play_in_tree().
            // This could introduce a systematic error.
            m_tree.add_value_remove_loss(node, eval[mv.player]);
        else
            m_tree.add_value(node, eval[mv.player]);
    }
    for (PlayerInt i = 0; i < m_nu_players; ++i)
        m_root_val[i].add(eval[i]);
}

//-----------------------------------------------------------------------------

} // namespace libboardgame_mcts

#endif // LIBBOARDGAME_MCTS_SEARCH_BASE_H
