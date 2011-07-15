/** @mainpage notitle

    @section mainpage_libboardgame LibBoardGame Modules

    The LibBoardGame modules contain code that is not specific to the board
    game Blokus and could be reused for other projects:

    - libboardgame_gtp -
      Implementation of the Go Text Protocol (@ref libboardgame_doc_gtp)
    - libboardgame_sys -
      Platform-dependent functionality
    - libboardgame_util -
      General utilities not specific to board games
    - libboardgame_sgf -
      Implementation of the Smart Game Format (@ref libboardgame_doc_sgf)
    - libboardgame_base -
      Utility classes and functions specific to board games
    - libboardgame_mcts -
      Monte-Carlo tree search
    - libboardgame_test -
      Functionality for unit tests similar to Boost::Test

    @section mainpage_pentobi Pentobi Modules

    The Pentobi modules are specific to the board game Blokus:

    - libpentobi_base -
      General Blokus-specific functionality
    - libpentobi_mcts -
      Blokus player based on Monte-Carlo tree search
    - pentobi_gtp -
      @ref libboardgame_doc_gtp interface to the player in libpentobi_mcts

    All LibBoardGame and Pentobi modules depend on the following
    <a href="http://www.boost.org/">Boost</a> libraries: thread, system,
    filesystem and program_options.

    @section mainpage_gui Pentobi GUI Modules

    The Pentobi GUI modules implement the user interface.
    They have the additional dependency on the following
    <a href="http://qt.nokia.com/">Qt</a> libraries: QtCore
    and QtGui.

    - libpentobi_gui -
      GUI functionality that could be reused for other projects
    - pentobi -
      Main program that provides a GUI for the player in libpentobi_mcts
    - pentobi_thumbnailer -
      Generates file preview thumbnails for the Gnome desktop
*/
