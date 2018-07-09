//-----------------------------------------------------------------------------
/** @file pentobi_qml/GameModel.cpp
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#include "GameModel.h"

#include <cerrno>
#include <cmath>
#include <cstring>
#include <fstream>
#include <QClipboard>
#include <QGuiApplication>
#include <QDebug>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QTextCodec>
#include "libboardgame_sgf/SgfUtil.h"
#include "libboardgame_sgf/TreeReader.h"
#include "libpentobi_base/MoveMarker.h"
#include "libpentobi_base/NodeUtil.h"
#include "libpentobi_base/PentobiTreeWriter.h"
#include "libpentobi_base/TreeUtil.h"

#ifdef Q_OS_ANDROID
#include <QtAndroidExtras/QtAndroid>
#include <QtAndroidExtras/QAndroidJniObject>
#endif

using namespace std;
using libboardgame_sgf::SgfError;
using libboardgame_sgf::TreeReader;
using libboardgame_sgf::back_to_main_variation;
using libboardgame_sgf::beginning_of_branch;
using libboardgame_sgf::find_next_comment;
using libboardgame_sgf::get_last_node;
using libboardgame_sgf::get_move_annotation;
using libboardgame_sgf::has_comment;
using libboardgame_sgf::has_earlier_variation;
using libboardgame_sgf::is_main_variation;
using libboardgame_util::ArrayList;
using libboardgame_util::get_letter_coord;
using libpentobi_base::get_piece_set;
using libpentobi_base::to_string_id;
using libpentobi_base::BoardType;
using libpentobi_base::Color;
using libpentobi_base::ColorMap;
using libpentobi_base::ColorMove;
using libpentobi_base::CoordPoint;
using libpentobi_base::MovePoints;
using libpentobi_base::PentobiTree;
using libpentobi_base::PentobiTreeWriter;
using libpentobi_base::Piece;
using libpentobi_base::PieceInfo;
using libpentobi_base::PiecePoints;
using libpentobi_base::PieceSet;
using libpentobi_base::Point;
using libpentobi_base::has_setup;
using libpentobi_base::get_move_number;
using libpentobi_base::get_moves_left;
using libpentobi_base::get_move_node;
using libpentobi_base::get_position_info;

//-----------------------------------------------------------------------------

namespace {

// Game coordinates are fractional because they refer to the center of a piece.
// This function is used to compare game coordinates of moves with the same
// piece, so we could even compare the rounded values (?), but comparing
// against epsilon is also safe.
bool compareGameCoord(const QPointF& p1, const QPointF& p2)
{
    return (p1 - p2).manhattanLength() < 0.01f;
}

bool compareTransform(const PieceInfo& pieceInfo, const Transform* t1,
                      const Transform* t2)
{
    return pieceInfo.get_equivalent_transform(t1) ==
            pieceInfo.get_equivalent_transform(t2);
}

QPointF getGameCoord(const Board& bd, Move mv)
{
    auto& geo = bd.get_geometry();
    PiecePoints movePoints;
    for (Point p : bd.get_move_points(mv))
        movePoints.push_back(CoordPoint(geo.get_x(p), geo.get_y(p)));
    return PieceModel::findCenter(bd, movePoints, false);
}

/** Simple heuristic used for sorting the list used in GameModel::findMove().
    Prefers larger pieces, and moves of the same piece (in that order). */
double getHeuristic(const Board& bd, Move mv)
{
    auto piece = bd.get_move_piece(mv);
    auto points = bd.get_piece_info(piece).get_score_points();
    return Piece::max_pieces * points - piece.to_int();
}

/** Get the index of a variation.
    This ignores child nodes without moves so that the moves are still labeled
    1a, 1b, 1c, etc. even if this does not correspond to the child node
    index. (Note that this is a different convention from variation strings
    which does not use move number and child move index, but node depth and
    child node index) */
bool getVariationIndex(const PentobiTree& tree, const SgfNode& node,
                       unsigned& moveIndex)
{
    auto parent = node.get_parent_or_null();
    if (! parent || parent->has_single_child())
        return false;
    unsigned nuSiblingMoves = 0;
    moveIndex = 0;
    for (auto& i : parent->get_children())
    {
        if (! tree.has_move(i))
            continue;
        if (&i == &node)
            moveIndex = nuSiblingMoves;
        ++nuSiblingMoves;
    }
    if (nuSiblingMoves == 1)
        return false;
    return true;
}

} //namespace

//-----------------------------------------------------------------------------

GameMove::GameMove(QObject* parent, ColorMove mv)
    : QObject(parent),
      m_move(mv)
{
}

//-----------------------------------------------------------------------------

GameModel::GameModel(QObject* parent)
    : QObject(parent),
      m_game(getInitialGameVariant()),
      m_gameVariant(to_string_id(m_game.get_variant())),
      m_nuColors(getBoard().get_nu_colors()),
      m_nuPlayers(getBoard().get_nu_players())
{
    loadRecentFiles();
    initGame(m_game.get_variant());
    createPieceModels();
    updateProperties();
}

GameModel::~GameModel() = default;

GameMove* GameModel::addEmpty(const QPoint& pos)
{
    if (! checkSetupAllowed())
        return nullptr;
    auto& bd = getBoard();
    auto& geo = bd.get_geometry();
    if (pos.x() < 0 || pos.y() < 0)
        return nullptr;
    auto x = static_cast<unsigned>(pos.x());
    auto y = static_cast<unsigned>(pos.y());
    if (! geo.is_onboard(x, y))
        return nullptr;
    auto p = geo.get_point(x, y);
    auto s = bd.get_point_state(p);
    if (s.is_empty())
        return nullptr;
    auto c = s.to_color();
    auto mv = bd.get_move_at(p);
    LIBBOARDGAME_ASSERT(bd.get_setup().placements[c].contains(mv));
    preparePositionChange();
    m_game.remove_setup(c, mv);
    setSetupPlayer();
    updateProperties();
    return new GameMove(this, ColorMove(c, mv));
}

void GameModel::addRecentFile(const QString& file)
{
    m_recentFiles.removeAll(file);
    m_recentFiles.prepend(file);
    while (m_recentFiles.length() > maxRecentFiles)
        m_recentFiles.removeLast();
    QSettings settings;
    settings.setValue("recentFiles", m_recentFiles);
    emit recentFilesChanged();
}

void GameModel::addSetup(PieceModel* pieceModel, QPointF coord)
{
    if (! checkSetupAllowed())
        return;
    Color c(static_cast<Color::IntType>(pieceModel->color()));
    Move mv;
    if (! findMove(*pieceModel, pieceModel->state(), coord, mv))
    {
        qWarning("GameModel: move not found");
        return;
    }
    preparePositionChange();
    preparePieceGameCoord(pieceModel, mv);
    pieceModel->setIsPlayed(true);
    preparePieceTransform(pieceModel, mv);
    try
    {
        m_game.add_setup(c, mv);
    }
    catch (const SgfError& error)
    {
        m_lastInputOutputError = error.what();
        emit invalidSgfFile();
    }
    setSetupPlayer();
    updateProperties();
}

/** Request the Android media scanner to scan a file.
    This ensures that the file will be visible via MTP. */
void GameModel::androidScanFile(const QString& pathname)
{
#ifdef Q_OS_ANDROID
    // Corresponding Java code:
    //   sendBroadcast(new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE,
    //                       Uri.fromFile(File(pathname).getCanonicalFile())));
    auto ACTION_MEDIA_SCANNER_SCAN_FILE =
            QAndroidJniObject::getStaticObjectField<jstring>(
                "android/content/Intent", "ACTION_MEDIA_SCANNER_SCAN_FILE");
    if (! ACTION_MEDIA_SCANNER_SCAN_FILE.isValid())
        return;
    auto pathnameString = QAndroidJniObject::fromString(pathname);
    QAndroidJniObject file("java/io/File", "(Ljava/lang/String;)V",
                           pathnameString.object<jstring>());
    if (! file.isValid())
        return;
    auto absoluteFile = file.callObjectMethod(
                "getCanonicalFile", "()Ljava/io/File;", file.object());
    if (! absoluteFile.isValid())
        return;
    auto uri = QAndroidJniObject::callStaticObjectMethod(
                "android/net/Uri", "fromFile",
                "(Ljava/io/File;)Landroid/net/Uri;", absoluteFile.object());
    if (! uri.isValid())
        return;
    QAndroidJniObject intent("android/content/Intent",
                             "(Ljava/lang/String;Landroid/net/Uri;)V",
                             ACTION_MEDIA_SCANNER_SCAN_FILE.object<jstring>(),
                             uri.object());
    if (! intent.isValid())
        return;
    auto activity = QtAndroid::androidActivity();
    activity.callMethod<void>("sendBroadcast", "(Landroid/content/Intent;)V",
                              intent.object());
#else
    LIBBOARDGAME_UNUSED(pathname);
#endif
}

void GameModel::autoSave()
{
    auto& tree = m_game.get_tree();
    QSettings settings;
    settings.setValue("variant", to_string_id(m_game.get_variant()));
    settings.setValue("autosave", getSgf());

    settings.setValue("file", m_file);
    settings.setValue("fileDate", m_fileDate);
    settings.setValue("isModified", m_isModified);

    QVariantList location;
    uint depth = 0;
    auto node = &m_game.get_current();
    while (node != &tree.get_root())
    {
        auto& parent = node->get_parent();
        if (parent.get_nu_children() > 1)
            location.prepend(parent.get_child_index(*node));
        node = &parent;
        ++depth;
    }
    location.prepend(depth);
    settings.setValue("autosaveLocation", location);
}

void GameModel::backToMainVar()
{
    gotoNode(back_to_main_variation(m_game.get_current()));
}

void GameModel::changeGameVariant(const QString& gameVariant)
{
    Variant variant;
    if (! parse_variant_id(gameVariant.toLocal8Bit().constData(), variant))
    {
        qWarning("GameModel: invalid game variant");
        return;
    }
    initGameVariant(variant);
    setIsModified(false);
    setFile("");
}

bool GameModel::checkFileDeletedOutside()
{
    if (m_file.isEmpty() || ! m_fileDate.isValid())
        return false;
    QFileInfo fileInfo(m_file);
    return ! fileInfo.exists();
}

bool GameModel::checkFileExists(const QString& file)
{
    return QFileInfo::exists(file);
}

bool GameModel::checkFileModifiedOutside()
{
    if (m_file.isEmpty() || ! m_fileDate.isValid())
        return false;
    QFileInfo fileInfo(m_file);
    if (! fileInfo.exists())
        // We handle deleted files as not modified, no reason to ask the user
        // what to do if they want to save a file in this case.
        return false;
    return fileInfo.lastModified() != m_fileDate;
}

/** Check if setup is allowed in the current position.
    Currently, we supprt setup mode only if no moves have been played. It
    should also work in inner nodes but this might be confusing for users and
    violate some assumptions in the user interface (e.g. node depth is equal to
    move number).*/
bool GameModel::checkSetupAllowed() const
{
    if (m_canGoBackward || m_canGoForward || m_moveNumber > 0)
    {
        qWarning("GameModel: setup only supported in root node");
        return false;
    }
    return true;
}

void GameModel::clearLegalMoves()
{
    if (! m_legalMoves)
        return;
    m_legalMoves->clear();
    m_legalMoveIndex = 0;
}

void GameModel::createPieceModels()
{
    createPieceModels(Color(0), m_pieceModels0);
    createPieceModels(Color(1), m_pieceModels1);
    if (m_nuColors > 2)
        createPieceModels(Color(2), m_pieceModels2);
    else
        m_pieceModels2.clear();
    if (m_nuColors > 3)
        createPieceModels(Color(3), m_pieceModels3);
    else
        m_pieceModels3.clear();
}

void GameModel::createPieceModels(Color c, QList<PieceModel*>& pieceModels)
{
    auto& bd = getBoard();
    auto nuPieces = bd.get_nu_uniq_pieces();
    pieceModels.clear();
    pieceModels.reserve(nuPieces);
    for (Piece::IntType i = 0; i < nuPieces; ++i)
    {
        Piece piece(i);
        for (unsigned j = 0; j < bd.get_piece_info(piece).get_nu_instances();
             ++j)
            pieceModels.append(new PieceModel(this, bd, piece, c));
    }
}

QString GameModel::decode(const string& s) const
{
    return m_textCodec->toUnicode(s.c_str());
}

void GameModel::deleteAllVar()
{
    if (! is_main_variation(m_game.get_current()))
        preparePositionChange();
    m_game.delete_all_variations();
    updateProperties();
}


QByteArray GameModel::encode(const QString& s) const
{
    return m_textCodec->fromUnicode(s);
}

GameMove* GameModel::findMove()
{
    auto& bd = getBoard();
    auto c = bd.get_to_play();
    if (bd.is_game_over())
        return new GameMove(this, ColorMove(c, Move::null()));
    if (! m_legalMoves)
        m_legalMoves.reset(new MoveList);
    if (m_legalMoves->empty())
    {
        if (! m_marker)
            m_marker.reset(new MoveMarker);
        bd.gen_moves(c, *m_marker, *m_legalMoves);
        m_marker->clear(*m_legalMoves);
        sort(m_legalMoves->begin(), m_legalMoves->end(),
             [&](Move mv1, Move mv2) {
                 return getHeuristic(bd, mv1) > getHeuristic(bd, mv2);
             });
    }
    if (m_legalMoves->empty())
        return new GameMove(this, ColorMove(c, Move::null()));
    if (m_legalMoveIndex >= m_legalMoves->size())
        m_legalMoveIndex = 0;
    auto mv = (*m_legalMoves)[m_legalMoveIndex++];
    return new GameMove(this, ColorMove(c, mv));
}

bool GameModel::findMove(const PieceModel& pieceModel, const QString& state,
                         QPointF coord, Move& mv) const
{
    auto piece = pieceModel.getPiece();
    auto& bd = getBoard();
    if (piece.to_int() >= bd.get_nu_uniq_pieces())
    {
        qWarning("GameModel::findMove: pieceModel invalid in game variant");
        return false;
    }
    auto transform = pieceModel.getTransform(state);
    if (! transform)
    {
        qWarning("GameModel::findMove: transform not found");
        return false;
    }
    auto& info = bd.get_piece_info(piece);
    PiecePoints piecePoints = info.get_points();
    transform->transform(piecePoints.begin(), piecePoints.end());
    QPointF center(PieceModel::findCenter(bd, piecePoints, false));
    // Round y of center to a multiple of 0.5, works better in Trigon
    center.setY(round(2 * center.y()) / 2);
    auto pointType = transform->get_point_type();
    auto dx = coord.x() - center.x();
    auto dy = coord.y() - center.y();
    int offX;
    if (bd.get_piece_set() == PieceSet::gembloq)
    {
        // In GembloQ, every piece has at least one full square, so we can use
        // half the x resolution, which makes positioning easier for the user.
        if (pointType == 0 || pointType == 2)
            offX = static_cast<int>(round(dx * 0.5f)) * 2;
        else
            offX = static_cast<int>(round((dx - 1) * 0.5f)) * 2 + 1;
    }
    else
        offX = static_cast<int>(round(dx));
    int offY = static_cast<int>(round(dy));
    auto& geo = bd.get_geometry();
    if (geo.get_point_type(offX, offY) != pointType)
        return false;
    MovePoints points;
    for (auto& p : piecePoints)
    {
        int x = p.x + offX;
        int y = p.y + offY;
        if (! geo.is_onboard(x, y))
            return false;
        points.push_back(geo.get_point(x, y));
    }
    return bd.find_move(points, piece, mv);
}

bool GameModel::findNextComment()
{
    auto node = find_next_comment(m_game.get_current());
    if (! node)
        return false;
    gotoNode(*node);
    return true;
}

bool GameModel::findNextCommentContinueFromRoot()
{
    auto node = &m_game.get_root();
    if (! has_comment(*node))
        node = find_next_comment(*node);
    if (! node)
        return false;
    gotoNode(*node);
    return true;
}

PieceModel* GameModel::findPieceModel(Color c, Piece piece)
{
    for (auto pieceModel : getPieceModels(c))
        if (pieceModel->getPiece() == piece)
            return pieceModel;
    return nullptr;
}

QUrl GameModel::getDefaultFolder() const
{
#ifdef Q_OS_ANDROID
    return QUrl("file:///sdcard");
#else
    return QUrl::fromLocalFile(
                QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
#endif
}

QString GameModel::getPlayerString(int player)
{
    auto variant = m_game.get_variant();
    bool isMulticolor = (m_nuColors > m_nuPlayers && variant != Variant::classic_3);
    switch (player) {
    case 0:
        if (isMulticolor)
            return tr("Blue/Red");
        else if (variant == Variant::duo)
            return tr("Purple");
        else if (variant == Variant::junior)
            return tr("Green");
        else
            return tr("Blue");
    case 1:
        if (isMulticolor)
            return tr("Yellow/Green");
        else if (variant == Variant::duo || variant == Variant::junior)
            return tr("Orange");
        else if (m_nuColors == 2)
            return tr("Green");
        else
            return tr("Yellow");
    case 2:
        return tr("Red");
    case 3:
        return tr("Green");
    }
    return "";
}

Variant GameModel::getInitialGameVariant()
{
    QSettings settings;
    auto variantString = settings.value("variant", "").toString();
    Variant variant;
    if (! parse_variant_id(variantString.toLocal8Bit().constData(), variant))
        variant = Variant::duo;
    return variant;
}

QList<PieceModel*>& GameModel::getPieceModels(Color c)
{
    if (c == Color(0))
        return m_pieceModels0;
    else if (c == Color(1))
        return m_pieceModels1;
    else if (c == Color(2))
        return m_pieceModels2;
    else
        return m_pieceModels3;
}

QString GameModel::getResultMessage()
{
    auto& bd = getBoard();
    auto variant = bd.get_variant();
    if (variant == Variant::duo)
    {
        auto score = m_points0 - m_points1;
        if (score == 1)
            return tr("Purple wins with 1 point.");
        if (score > 0)
            return tr("Purple wins with %L1 points.").arg(score);
        if (score == -1)
            return tr("Orange wins with 1 point.");
        if (score < 0)
            return tr("Orange wins with %L1 points.").arg(-score);
        return tr("Game ends in a tie.");
    }
    if (variant == Variant::junior)
    {
        auto score = m_points0 - m_points1;
        if (score == 1)
            return tr("Green wins with 1 point.");
        if (score > 0)
            return tr("Green wins with %L1 points.").arg(score);
        if (score == -1)
            return tr("Orange wins with 1 point.");
        if (score < 0)
            return tr("Orange wins with %L1 points.").arg(-score);
        return tr("Game ends in a tie.");
    }
    bool breakTies = (bd.get_piece_set() == PieceSet::callisto);
    if (m_nuColors == 2)
    {
        auto score = m_points0 - m_points1;
        if (score == 1)
            return tr("Blue wins with 1 point.");
        if (score > 0)
            return tr("Blue wins with %L1 points.").arg(score);
        if (score == -1)
            return tr("Green wins with 1 point.");
        if (score < 0)
            return tr("Green wins with %L1 points.").arg(-score);
        if (breakTies)
            //: Game variant with tie-breaker rule made later player win.
            return tr("Green wins (tie resolved).");
        return tr("Game ends in a tie.");
    }
    if (m_nuColors == 4 && m_nuPlayers == 2)
    {
        auto score = m_points0 + m_points2 - m_points1 - m_points3;
        if (score == 1)
            return tr("Blue/Red wins with 1 point.");
        if (score > 0)
            return tr("Blue/Red wins with %L1 points.").arg(score);
        if (score == -1)
            return tr("Yellow/Green wins with 1 point.");
        if (score < 0)
            return tr("Yellow/Green wins with %L1 points.").arg(-score);
        if (breakTies)
            //: Game variant with tie-breaker rule made later player win.
            return tr("Yellow/Green wins (tie resolved).");
        return tr("Game ends in a tie.");
    }
    if (m_nuPlayers == 3)
    {
        auto maxPoints = max(max(m_points0, m_points1), m_points2);
        unsigned nuWinners = 0;
        if (m_points0 == maxPoints)
            ++nuWinners;
        if (m_points1 == maxPoints)
            ++nuWinners;
        if (m_points2 == maxPoints)
            ++nuWinners;
        if (m_points0 == maxPoints && nuWinners == 1)
            return tr("Blue wins.");
        if (m_points1 == maxPoints && nuWinners == 1)
            return tr("Yellow wins.");
        if (m_points2 == maxPoints && nuWinners == 1)
            return tr("Red wins.");
        if (m_points2 == maxPoints && breakTies)
            //: Game variant with tie-breaker rule made later player win.
            return tr("Red wins (tie resolved).");
        if (m_points1 == maxPoints && breakTies)
            //: Game variant with tie-breaker rule made later player win.
            return tr("Yellow wins (tie resolved).");
        if (m_points0 == maxPoints && m_points1 == maxPoints && nuWinners == 2)
            return tr("Game ends in a tie between Blue and Yellow.");
        if (m_points0 == maxPoints && m_points2 == maxPoints && nuWinners == 2)
            return tr("Game ends in a tie between Blue and Red.");
        if (nuWinners == 2)
            return tr("Game ends in a tie between Yellow and Red.");
        return tr("Game ends in a tie between all players.");
    }
    auto maxPoints = max(max(m_points0, m_points1), max(m_points2, m_points3));
    unsigned nuWinners = 0;
    if (m_points0 == maxPoints)
        ++nuWinners;
    if (m_points1 == maxPoints)
        ++nuWinners;
    if (m_points2 == maxPoints)
        ++nuWinners;
    if (m_points3 == maxPoints)
        ++nuWinners;
    if (m_points0 == maxPoints && nuWinners == 1)
        return tr("Blue wins.");
    if (m_points1 == maxPoints && nuWinners == 1)
        return tr("Yellow wins.");
    if (m_points2 == maxPoints && nuWinners == 1)
        return tr("Red wins.");
    if (m_points3 == maxPoints && nuWinners == 1)
        return tr("Green wins.");
    if (m_points3 == maxPoints && breakTies)
        //: Game variant with tie-breaker rule made later player win.
        return tr("Green wins (tie resolved).");
    if (m_points2 == maxPoints && breakTies)
        //: Game variant with tie-breaker rule made later player win.
        return tr("Red wins (tie resolved).");
    if (m_points1 == maxPoints && breakTies)
        //: Game variant with tie-breaker rule made later player win.
        return tr("Yellow wins (tie resolved).");
    if (m_points0 == maxPoints && m_points1 == maxPoints
            && m_points2 == maxPoints && nuWinners == 3)
        return tr("Game ends in a tie between Blue, Yellow and Red.");
    if (m_points0 == maxPoints && m_points1 == maxPoints
            && m_points3 == maxPoints && nuWinners == 3)
        return tr("Game ends in a tie between Blue, Yellow and Green.");
    if (m_points0 == maxPoints && m_points2 == maxPoints
            && m_points3 == maxPoints && nuWinners == 3)
        return tr("Game ends in a tie between Blue, Red and Green.");
    if (nuWinners == 3)
        return tr("Game ends in a tie between Yellow, Red and Green.");
    if (m_points0 == maxPoints && m_points1 == maxPoints && nuWinners == 2)
        return tr("Game ends in a tie between Blue and Yellow.");
    if (m_points0 == maxPoints && m_points2 == maxPoints && nuWinners == 2)
        return tr("Game ends in a tie between Blue and Red.");
    if (nuWinners == 2)
        return tr("Game ends in a tie between Yellow and Red.");
    return tr("Game ends in a tie between all players.");
}

QByteArray GameModel::getSgf() const
{
    auto& tree = m_game.get_tree();
    ostringstream s;
    PentobiTreeWriter writer(s, tree);
    writer.set_indent(-1);
    writer.write();
    return QByteArray(s.str().c_str());
}

QString GameModel::getVariationInfo() const
{
    auto moveNumber = getBoard().get_nu_moves();
    QString s = QString::number(moveNumber);
    unsigned moveIndex;
    if (getVariationIndex(m_game.get_tree(), m_game.get_current(), moveIndex))
        s.append(get_letter_coord(moveIndex).c_str());
    return s;
}

void GameModel::goBackward()
{
    gotoNode(m_game.get_current().get_parent_or_null());
}

void GameModel::goBeginning()
{
    gotoNode(m_game.get_root());
}

void GameModel::goEnd()
{
    gotoNode(get_last_node(m_game.get_current()));
}

void GameModel::goForward()
{
    gotoNode(m_game.get_current().get_first_child_or_null());
}

void GameModel::goNextVar()
{
    gotoNode(m_game.get_current().get_sibling());
}

void GameModel::goPrevVar()
{
    gotoNode(m_game.get_current().get_previous_sibling());
}

void GameModel::gotoBeginningOfBranch()
{
    gotoNode(beginning_of_branch(m_game.get_current()));
}

void GameModel::gotoMove(int n)
{
    if (n > 0)
        gotoNode(get_move_node(m_game.get_tree(), m_game.get_current(), n));
}

void GameModel::gotoNode(const SgfNode& node)
{
    if (&node == &m_game.get_current())
        return;
    preparePositionChange();
    try
    {
        m_game.goto_node(node);
    }
    catch (const SgfError& error)
    {
        m_lastInputOutputError = error.what();
        emit invalidSgfFile();
    }
    updateProperties();
}

void GameModel::gotoNode(const SgfNode* node)
{
    if (node)
        gotoNode(*node);
}

void GameModel::initGame(Variant variant)
{
    m_game.init(variant);
#ifdef VERSION
    m_game.set_application("Pentobi", VERSION);
#else
    m_game.set_application("Pentobi");
#endif
    m_game.set_date_today();
    setUtf8();
    updateGameInfo();
}

void GameModel::initGameVariant(Variant variant)
{
    if (m_game.get_variant() != variant)
        initGame(variant);
    auto& bd = getBoard();
    set(m_nuColors, static_cast<int>(bd.get_nu_colors()), &GameModel::nuColorsChanged);
    set(m_nuPlayers, static_cast<int>(bd.get_nu_players()), &GameModel::nuPlayersChanged);
    m_lastMovePieceModel = nullptr;
    createPieceModels();
    m_gameVariant = to_string_id(variant);
    emit gameVariantChanged();
    updateProperties();
}

bool GameModel::isLegalPos(PieceModel* pieceModel, const QString& state,
                           QPointF coord) const
{
    Move mv;
    if (! findMove(*pieceModel, state, coord, mv))
        return false;
    Color c(static_cast<Color::IntType>(pieceModel->color()));
    return getBoard().is_legal(c, mv);
}

bool GameModel::isLegalSetupPos(PieceModel* pieceModel, const QString& state,
                                QPointF coord) const
{
    Move mv;
    if (! findMove(*pieceModel, state, coord, mv))
        return false;
    auto& bd = getBoard();
    for (auto p : bd.get_move_points(mv))
        if (! bd.get_point_state(p).is_empty())
            return false;
    return true;
}

void GameModel::keepOnlyPosition()
{
    m_game.keep_only_position();
    updateProperties();
}

void GameModel::keepOnlySubtree()
{
    m_game.keep_only_subtree();
    updateProperties();
}

bool GameModel::loadAutoSave()
{
    QSettings settings;
    if (! openByteArray(settings.value("autosave", "").toByteArray()))
        return false;
    setFile(settings.value("file").toString());
    m_fileDate = settings.value("fileDate").toDateTime();
    setIsModified(settings.value("isModified").toBool());
    restoreAutoSaveLocation();
    updateProperties();
    return true;
}

void GameModel::loadRecentFiles()
{
    QSettings settings;
    m_recentFiles = settings.value("recentFiles", "").toStringList();
    QMutableListIterator<QString> i(m_recentFiles);
    while (i.hasNext()) {
        auto file = i.next();
        if (file.isEmpty() || ! QFileInfo::exists(file))
            i.remove();
    }
    while (m_recentFiles.length() > maxRecentFiles)
        m_recentFiles.removeLast();
    emit recentFilesChanged();
}

bool GameModel::openByteArray(const QByteArray& byteArray)
{
    istringstream in(byteArray.constData());
    setFile("");
    if (! openStream(in))
        return false;
    goEnd();
    return true;
}

void GameModel::makeMainVar()
{
    m_game.make_main_variation();
    updateProperties();
}

void GameModel::moveDownVar()
{
    m_game.move_down_variation();
    updateProperties();
}

void GameModel::moveUpVar()
{
    m_game.move_up_variation();
    updateProperties();
}

void GameModel::nextColor()
{
    preparePositionChange();
    auto& bd = getBoard();
    m_game.set_to_play(bd.get_next(bd.get_to_play()));
    setSetupPlayer();
    updateProperties();
}

PieceModel* GameModel::nextPiece(PieceModel* currentPickedPiece)
{
    auto& bd = getBoard();
    auto c = bd.get_to_play();
    if (bd.get_pieces_left(c).empty())
        return nullptr;
    auto nuUniqPieces = bd.get_nu_uniq_pieces();
    Piece::IntType i;
    if (currentPickedPiece != nullptr)
        i = static_cast<Piece::IntType>(
                    currentPickedPiece->getPiece().to_int() + 1);
    else
        i = 0;
    while (true)
    {
        if (i >= nuUniqPieces)
            i = 0;
        if (bd.is_piece_left(c, Piece(i)))
            break;
        ++i;
    }
    return findPieceModel(c, Piece(i));
}

void GameModel::newGame()
{
    preparePositionChange();
    initGame(m_game.get_variant());
    setIsModified(false);
    setFile("");
    for (auto pieceModel : m_pieceModels0)
        pieceModel->setDefaultState();
    for (auto pieceModel : m_pieceModels1)
        pieceModel->setDefaultState();
    for (auto pieceModel : m_pieceModels2)
        pieceModel->setDefaultState();
    for (auto pieceModel : m_pieceModels3)
        pieceModel->setDefaultState();
    updateProperties();
}

bool GameModel::openStream(istream& in)
{
    bool result = true;
    try
    {
        preparePositionChange();
        TreeReader reader;
        reader.read(in);
        auto root = reader.get_tree_transfer_ownership();
        m_game.init(root);
    }
    catch (const runtime_error& e)
    {
        m_lastInputOutputError =
                QString(tr("Invalid Blokus SGF file. (%1)"))
                .arg(QString::fromLocal8Bit(e.what()));
        result = false;
    }
    auto charSet = m_game.get_charset();
    if (charSet.empty())
        m_textCodec = QTextCodec::codecForName("ISO 8859-1");
    else
        m_textCodec = QTextCodec::codecForName(m_game.get_charset().c_str());
    if (! m_textCodec)
    {
        m_textCodec = QTextCodec::codecForName("ISO 8859-1");
        m_lastInputOutputError = QString(tr("File has unsupported character set."));
        result = false;
    }
    if (! result)
        m_game.init();
    auto variant = to_string_id(m_game.get_variant());
    if (variant != m_gameVariant)
        initGameVariant(m_game.get_variant());
    setIsModified(false);
    updateProperties();
    return result;
}

bool GameModel::openFile(const QString& file)
{
    ifstream in(file.toLocal8Bit().constData());
    if (! in)
    {
        m_lastInputOutputError = QString::fromLocal8Bit(strerror(errno));
        return false;
    }
    if (openStream(in))
    {
        updateFileInfo(file);
        addRecentFile(file);
        auto& root = m_game.get_root();
        // Show end of game position by default unless the root node has
        // setup stones or comments, because then it might be a puzzle and
        // we don't want to show the solution.
        if (! has_setup(root) && ! has_comment(root) && root.has_children())
            goEnd();
        return true;
    }
    setFile("");
    return false;
}

bool GameModel::openClipboard()
{
    auto text = QGuiApplication::clipboard()->text();
    if (text.isEmpty())
    {
        m_lastInputOutputError = tr("Clipboard is empty.");
        return false;
    }
    istringstream in(text.toLocal8Bit().constData());
    if (openStream(in))
    {
        auto& root = m_game.get_root();
        if (! has_setup(root) && root.has_children())
            goEnd();
        return true;
    }
    setFile("");
    return false;
}

PieceModel* GameModel::pickNamedPiece(const QString& name,
                                      PieceModel* currentPickedPiece)
{
    string nameStr(name.toLocal8Bit().constData());
    auto& bd = getBoard();
    auto c = bd.get_to_play();
    Board::PiecesLeftList pieces;
    for (Piece::IntType i = 0; i < bd.get_nu_uniq_pieces(); ++i)
    {
        Piece piece(i);
        if (bd.is_piece_left(c, piece)
                && bd.get_piece_info(piece).get_name().find(nameStr) == 0)
            pieces.push_back(piece);
    }
    if (pieces.empty())
        return nullptr;
    Piece piece;
    if (currentPickedPiece == nullptr)
        piece = pieces[0];
    else
    {
        piece = currentPickedPiece->getPiece();
        auto pos = std::find(pieces.begin(), pieces.end(), piece);
        if (pos == pieces.end())
            piece = pieces[0];
        else
        {
            ++pos;
            if (pos == pieces.end())
                piece = pieces[0];
            else
                piece = *pos;
        }
    }
    return findPieceModel(c, piece);
}

QQmlListProperty<PieceModel> GameModel::pieceModels0()
{
    return QQmlListProperty<PieceModel>(this, m_pieceModels0);
}

QQmlListProperty<PieceModel> GameModel::pieceModels1()
{
    return QQmlListProperty<PieceModel>(this, m_pieceModels1);
}

QQmlListProperty<PieceModel> GameModel::pieceModels2()
{
    return QQmlListProperty<PieceModel>(this, m_pieceModels2);
}

QQmlListProperty<PieceModel> GameModel::pieceModels3()
{
    return QQmlListProperty<PieceModel>(this, m_pieceModels3);
}

void GameModel::playMove(GameMove* move)
{
    auto mv = move->get();
    if (mv.is_null())
        return;
    preparePositionChange();
    m_game.play(mv, false);
    updateProperties();
}

void GameModel::playPiece(PieceModel* pieceModel, QPointF coord)
{
    Color c(static_cast<Color::IntType>(pieceModel->color()));
    Move mv;
    if (! findMove(*pieceModel, pieceModel->state(), coord, mv))
    {
        qWarning("GameModel::play: illegal move");
        return;
    }
    preparePositionChange();
    preparePieceGameCoord(pieceModel, mv);
    pieceModel->setIsPlayed(true);
    preparePieceTransform(pieceModel, mv);
    m_game.play(c, mv, false);
    updateProperties();
}

PieceModel* GameModel::preparePiece(GameMove* move)
{
    if (move == nullptr)
        return nullptr;
    auto mv = move->get();
    if (mv.is_null())
        return nullptr;
    auto piece = getBoard().get_move_piece(mv.move);
    for (auto pieceModel : getPieceModels(mv.color))
        if (pieceModel->getPiece() == piece && ! pieceModel->isPlayed())
        {
            preparePieceTransform(pieceModel, mv.move);
            preparePieceGameCoord(pieceModel, mv.move);
            return pieceModel;
        }
    return nullptr;
}

void GameModel::preparePieceGameCoord(PieceModel* pieceModel, Move mv)
{
    pieceModel->setGameCoord(getGameCoord(getBoard(), mv));
}

void GameModel::preparePieceTransform(PieceModel* pieceModel, Move mv)
{
    auto& bd = getBoard();
    auto transform = bd.find_transform(mv);
    auto& pieceInfo = bd.get_piece_info(bd.get_move_piece(mv));
    if (! compareTransform(pieceInfo, pieceModel->getTransform(), transform))
        pieceModel->setTransform(transform);
}

void GameModel::preparePositionChange()
{
    clearLegalMoves();
    emit positionAboutToChange();
}

PieceModel* GameModel::previousPiece(PieceModel* currentPickedPiece)
{
    auto& bd = getBoard();
    auto c = bd.get_to_play();
    if (bd.get_pieces_left(c).empty())
        return nullptr;
    auto nuUniqPieces = bd.get_nu_uniq_pieces();
    Piece::IntType i;
    if (currentPickedPiece != nullptr)
        i = static_cast<Piece::IntType>(currentPickedPiece->getPiece().to_int());
    else
        i = 0;
    while (true)
    {
        if (i == 0)
            i = static_cast<Piece::IntType>(nuUniqPieces - 1);
        else
            --i;
        if (bd.is_piece_left(c, Piece(i)))
            break;
    }
    return findPieceModel(c, Piece(i));
}

bool GameModel::restoreAutoSaveLocation()
{
    QSettings settings;
    auto location = settings.value("autosaveLocation").value<QVariantList>();
    if (location.empty())
        return false;
    int index = 0;
    bool ok;
    auto depth = location[index++].toUInt(&ok);
    if (! ok)
        return false;
    auto node = &m_game.get_root();
    while (depth > 0)
    {
        auto nuChildren = node->get_nu_children();
        if (nuChildren == 0)
            return false;
        if (nuChildren == 1)
            node = &node->get_first_child();
        else
        {
            if (index >= location.size())
                return false;
            auto child = location[index++].toUInt(&ok);
            if (! ok || child >= nuChildren)
                return false;
            node = &node->get_child(child);
        }
        --depth;
    }
    gotoNode(*node);
    return true;
}

bool GameModel::save(const QString& file)
{
    {
        ofstream out(file.toLocal8Bit().constData());
        PentobiTreeWriter writer(out, m_game.get_tree());
        writer.set_indent(1);
        writer.write();
        if (! out)
        {
            m_lastInputOutputError = QString::fromLocal8Bit(strerror(errno));
            return false;
        }
    }
    androidScanFile(file);
    updateFileInfo(file);
    setIsModified(false);
    addRecentFile(file);
    return true;
}

bool GameModel::saveAsciiArt(const QString& file)
{
    ofstream out(file.toLocal8Bit().constData());
    getBoard().write(out, false);
    if (! out)
    {
        m_lastInputOutputError = QString::fromLocal8Bit(strerror(errno));
        return false;
    }
    androidScanFile(file);
    return true;
}

template<typename T>
bool GameModel::set(T& target, const T& value,
                    void (GameModel::*changedSignal)())
{
    if (target != value)
    {
        target = value;
        emit (this->*changedSignal)();
        return true;
    }
    return false;
}

void GameModel::setComment(const QString& comment)
{
    if (comment == m_comment)
        return;
    m_game.set_comment(encode(comment).constData());
    m_comment = comment;
    emit commentChanged();
    updateIsGameEmpty();
    updateIsModified();
}

void GameModel::setDate(const QString& date)
{
    if (date == m_date)
        return;
    m_date = date;
    m_game.set_date(encode(date).constData());
    emit dateChanged();
    // updateIsGameEmpty() not necessary, see
    // libboardgame_sgf::is_empty()
    updateIsModified();
}

void GameModel::setEvent(const QString& event)
{
    if (event == m_event)
        return;
    m_event = event;
    m_game.set_event(encode(event).constData());
    emit eventChanged();
    updateIsGameEmpty();
    updateIsModified();
}

void GameModel::setFile(const QString& file)
{
    if (file == m_file)
        return;
    m_file = file;
    emit fileChanged();
}

void GameModel::setIsModified(bool isModified)
{
    m_game.set_modified(isModified);
    updateIsModified();
}

void GameModel::setMoveAnnotation(const QString& annotation)
{
    m_game.remove_move_annotation();
    if (annotation == "!")
        m_game.set_good_move();
    else if (annotation == "!!")
        m_game.set_good_move(2);
    else if (annotation == "?")
        m_game.set_bad_move();
    else if (annotation == "??")
        m_game.set_bad_move(2);
    else if (annotation == "!?")
        m_game.set_interesting_move();
    else if (annotation == "?!")
        m_game.set_doubtful_move();
    updateMoveAnnotation();
    updatePositionInfo();
    updatePieces();
}

void GameModel::setPlayerName0(const QString& name)
{
    if (name == m_playerName0)
        return;
    m_playerName0 = name;
    m_game.set_player_name(Color(0), encode(name).constData());
    emit playerName0Changed();
    updateIsGameEmpty();
    updateIsModified();
}

void GameModel::setPlayerName1(const QString& name)
{
    if (name == m_playerName1)
        return;
    m_playerName1 = name;
    m_game.set_player_name(Color(1), encode(name).constData());
    emit playerName1Changed();
    updateIsGameEmpty();
    updateIsModified();
}

void GameModel::setPlayerName2(const QString& name)
{
    if (name == m_playerName2)
        return;
    m_playerName2 = name;
    m_game.set_player_name(Color(2), encode(name).constData());
    emit playerName2Changed();
    updateIsGameEmpty();
    updateIsModified();
}

void GameModel::setPlayerName3(const QString& name)
{
    if (name == m_playerName3)
        return;
    m_playerName3 = name;
    m_game.set_player_name(Color(3), encode(name).constData());
    emit playerName3Changed();
    updateIsGameEmpty();
    updateIsModified();
}

void GameModel::setRound(const QString& round)
{
    if (round == m_round)
        return;
    m_round = round;
    m_game.set_round(encode(round).constData());
    emit roundChanged();
    updateIsGameEmpty();
    updateIsModified();
}

void GameModel::setShowVariations(bool showVariations)
{
    if (set(m_showVariations, showVariations, &GameModel::showVariationsChanged))
        updatePieces();
}

void GameModel::setSetupPlayer()
{
    if (! m_game.has_setup())
        m_game.remove_player();
    else
        m_game.set_player(getBoard().get_to_play());
}

void GameModel::setTime(const QString& time)
{
    if (time == m_time)
        return;
    m_time = time;
    m_game.set_time(encode(time).constData());
    emit playerName3Changed();
    updateIsGameEmpty();
    updateIsModified();
}

void GameModel::setUtf8()
{
    m_game.set_charset("UTF-8");
    m_textCodec = QTextCodec::codecForName("UTF-8");
}

void GameModel::truncate()
{
    if (! m_game.get_current().has_parent())
        return;
    preparePositionChange();
    m_game.truncate();
    updateProperties();
}

void GameModel::truncateChildren()
{
    m_game.truncate_children();
    updateProperties();
}

void GameModel::undo()
{
    if (! m_canUndo)
        return;
    preparePositionChange();
    m_game.undo();
    updateProperties();
}

void GameModel::updateFileInfo(const QString& file)
{
    setFile(file);
    QFileInfo fileInfo(file);
    m_fileDate = fileInfo.lastModified();
}

void GameModel::updateGameInfo()
{
    static_assert(Color::range == 4, "");
    setPlayerName0(decode(m_game.get_player_name(Color(0))));
    setPlayerName1(decode(m_game.get_player_name(Color(1))));
    if (m_nuPlayers > 2)
        setPlayerName2(decode(m_game.get_player_name(Color(2))));
    if (m_nuPlayers > 3)
        setPlayerName3(decode(m_game.get_player_name(Color(3))));
    setDate(decode(m_game.get_date()));
    setTime(decode(m_game.get_time()));
    setEvent(decode(m_game.get_event()));
    setRound(decode(m_game.get_round()));
}

void GameModel::updateIsGameEmpty()
{
    set(m_isGameEmpty, libboardgame_sgf::is_empty(m_game.get_tree()),
        &GameModel::isGameEmptyChanged);
}

void GameModel::updateIsModified()
{
    // Don't consider modified game tree as modified if it is empty and no
    // file is associated.
    bool isModified =
            m_game.is_modified()
            && (! libboardgame_sgf::is_empty(m_game.get_tree())
                || ! m_file.isEmpty());
    set(m_isModified, isModified, &GameModel::isModifiedChanged);
}

void GameModel::updateMoveAnnotation()
{
    QString moveAnnotation =
            get_move_annotation(m_game.get_tree(), m_game.get_current());
    set(m_moveAnnotation, moveAnnotation, &GameModel::moveAnnotationChanged);
}

PieceModel* GameModel::updatePiece(Color c, Move mv,
                                   array<bool, Board::max_pieces>& isPlayed)
{
    auto& bd = getBoard();
    Piece piece = bd.get_move_piece(mv);
    auto& pieceInfo = bd.get_piece_info(piece);
    auto gameCoord = getGameCoord(bd, mv);
    auto transform = bd.find_transform(mv);
    auto& pieceModels = getPieceModels(c);
    // Prefer piece models already played with the given gameCoord and
    // transform because class Board doesn't make a distinction between
    // instances of the same piece (in Junior) and we want to avoid
    // unwanted piece movement animations to switch instances.
    for (int i = 0; i < pieceModels.length(); ++i)
        if (pieceModels[i]->getPiece() == piece
                && pieceModels[i]->isPlayed()
                && compareGameCoord(pieceModels[i]->gameCoord(), gameCoord)
                && compareTransform(pieceInfo, pieceModels[i]->getTransform(),
                                    transform))
        {
            isPlayed[i] = true;
            return pieceModels[i];
        }
    for (int i = 0; i < pieceModels.length(); ++i)
        if (pieceModels[i]->getPiece() == piece && ! isPlayed[i])
        {
            isPlayed[i] = true;
            // Set PieceModel.isPlayed temporarily to false, such that there is
            // always a state transition animation (e.g. if the piece stays
            // on the board but changes its coordinates when navigating through
            // move variations).
            pieceModels[i]->setIsPlayed(false);
            // Set gameCoord before isPlayed because the animation needs it.
            pieceModels[i]->setGameCoord(gameCoord);
            pieceModels[i]->setIsPlayed(true);
            pieceModels[i]->setTransform(transform);
            return pieceModels[i];
        }
    LIBBOARDGAME_ASSERT(false);
    return nullptr;
}

void GameModel::updatePieces()
{
    auto& bd = getBoard();
    ColorMap<array<bool, Board::max_pieces>> isPlayed;

    // Update pieces of setup
    for (Color c : bd.get_colors())
    {
        isPlayed[c].fill(false);
        for (Move mv : bd.get_setup().placements[c])
        {
            auto pieceModel = updatePiece(c, mv, isPlayed[c]);
            pieceModel->setMoveLabel("");
        }
    }

    // Update pieces of moves played after last setup or root
    auto& tree = m_game.get_tree();
    // We need to loop forward through the moves to ensure the persistence of
    // the GUI pieces, see comment in updatePiece()
    ArrayList<const SgfNode*, Board::max_moves> nodes;
    auto node = &m_game.get_current();
    do
    {
        if (tree.has_move(*node))
            nodes.push_back(node);
        if (has_setup(*node) || nodes.size() == nodes.max_size)
            break;
        node = node->get_parent_or_null();
    }
    while (node);
    PieceModel* pieceModel = nullptr;
    int moveNumber = 1;
    for (auto i = nodes.size(); i > 0; --i)
    {
        auto& node = *nodes[i - 1];
        auto mv = tree.get_move(node);
        auto c = mv.color;
        pieceModel = updatePiece(c, mv.move, isPlayed[c]);
        QString label = QString::number(moveNumber);
        ++moveNumber;
        unsigned moveIndex;
        if (m_showVariations && getVariationIndex(tree, node, moveIndex))
            label.append(get_letter_coord(moveIndex).c_str());
        label.append(get_move_annotation(tree, node));
        pieceModel->setMoveLabel(label);
    }
    if (pieceModel != m_lastMovePieceModel)
    {
        if (m_lastMovePieceModel != nullptr)
            m_lastMovePieceModel->setIsLastMove(false);
        if (pieceModel != nullptr)
            pieceModel->setIsLastMove(true);
        m_lastMovePieceModel = pieceModel;
    }

    // Update pieces not on board
    for (Color c : bd.get_colors())
    {
        auto& pieceModels = getPieceModels(c);
        for (int i = 0; i < pieceModels.length(); ++i)
            if (! isPlayed[c][i] && pieceModels[i]->isPlayed())
            {
                pieceModels[i]->setDefaultState();
                pieceModels[i]->setIsPlayed(false);
                pieceModels[i]->setMoveLabel("");
            }
    }
}

void GameModel::updatePositionInfo()
{
    auto& tree = m_game.get_tree();
    auto& current = m_game.get_current();
    auto& bd = m_game.get_board();
    auto positionInfo
            = QString::fromLocal8Bit(get_position_info(tree, current).c_str());
    auto positionInfoShort = positionInfo;
    if (positionInfo.isEmpty())
    {
        positionInfo = bd.has_setup() ? tr("(Setup)") : tr("(No moves)");
        positionInfoShort = bd.has_setup() ? tr("(Setup)") : "";
    }
    else
    {
        //: The argument is the current move number.
        positionInfo = tr("Move %1").arg(positionInfo);
        if (bd.get_nu_moves() == 0 && bd.has_setup())
        {
            positionInfo.append(' ');
            positionInfo.append(tr("(Setup)"));
            positionInfoShort.append(' ');
            positionInfoShort.append(tr("(Setup)"));
        }
    }
    set(m_positionInfo, positionInfo, &GameModel::positionInfoChanged);
    set(m_positionInfoShort, positionInfoShort,
        &GameModel::positionInfoShortChanged);
}

/** Update all properties that might change when changing the current
    position in the game tree. */
void GameModel::updateProperties()
{
    auto& bd = getBoard();
    auto& geo = bd.get_geometry();
    auto& tree = m_game.get_tree();
    bool isTrigon = (bd.get_piece_set() == PieceSet::trigon);
    set(m_points0, bd.get_points(Color(0)), &GameModel::points0Changed);
    set(m_points1, bd.get_points(Color(1)), &GameModel::points1Changed);
    set(m_bonus0, bd.get_bonus(Color(0)), &GameModel::bonus0Changed);
    set(m_bonus1, bd.get_bonus(Color(1)), &GameModel::bonus1Changed);
    set(m_hasMoves0, bd.has_moves(Color(0)), &GameModel::hasMoves0Changed);
    set(m_hasMoves1, bd.has_moves(Color(1)), &GameModel::hasMoves1Changed);
    bool isFirstPieceAny = false;
    if (m_nuColors > 2)
    {
        set(m_points2, bd.get_points(Color(2)), &GameModel::points2Changed);
        set(m_bonus2, bd.get_bonus(Color(2)), &GameModel::bonus2Changed);
        set(m_hasMoves2, bd.has_moves(Color(2)), &GameModel::hasMoves2Changed);
    }
    if (m_nuColors > 3)
    {
        set(m_points3, bd.get_points(Color(3)), &GameModel::points3Changed);
        set(m_bonus3, bd.get_bonus(Color(3)), &GameModel::bonus3Changed);
        set(m_hasMoves3, bd.has_moves(Color(3)), &GameModel::hasMoves3Changed);
    }
    m_tmpPoints.clear();
    if (bd.is_first_piece(Color(0)))
    {
        isFirstPieceAny = true;
        if (! isTrigon)
            for (Point p : bd.get_starting_points(Color(0)))
                m_tmpPoints.append(QPointF(geo.get_x(p), geo.get_y(p)));
    }
    set(m_startingPoints0, m_tmpPoints, &GameModel::startingPoints0Changed);
    m_tmpPoints.clear();
    if (bd.is_first_piece(Color(1)))
    {
        isFirstPieceAny = true;
        if (! isTrigon)
            for (Point p : bd.get_starting_points(Color(1)))
                m_tmpPoints.append(QPointF(geo.get_x(p), geo.get_y(p)));
    }
    set(m_startingPoints1, m_tmpPoints, &GameModel::startingPoints1Changed);
    m_tmpPoints.clear();
    if (m_nuColors > 2 && bd.is_first_piece(Color(2)))
    {
        isFirstPieceAny = true;
        if (! isTrigon)
            for (Point p : bd.get_starting_points(Color(2)))
                m_tmpPoints.append(QPointF(geo.get_x(p), geo.get_y(p)));
    }
    set(m_startingPoints2, m_tmpPoints, &GameModel::startingPoints2Changed);
    m_tmpPoints.clear();
    if (m_nuColors > 3 && bd.is_first_piece(Color(3)))
    {
        isFirstPieceAny = true;
        if (! isTrigon)
            for (Point p : bd.get_starting_points(Color(3)))
                m_tmpPoints.append(QPointF(geo.get_x(p), geo.get_y(p)));
    }
    set(m_startingPoints3, m_tmpPoints, &GameModel::startingPoints3Changed);
    m_tmpPoints.clear();
    if (isTrigon && isFirstPieceAny)
        for (Point p : bd.get_starting_points(Color(0)))
            m_tmpPoints.append(QPointF(geo.get_x(p), geo.get_y(p)));
    set(m_startingPointsAll, m_tmpPoints,
        &GameModel::startingPointsAllChanged);
    auto& current = m_game.get_current();
    set(m_canUndo,
           ! current.has_children() && tree.has_move(current)
           && current.has_parent(),
           &GameModel::canUndoChanged);
    set(m_canGoForward, current.has_children(),
        &GameModel::canGoForwardChanged);
    set(m_canGoBackward, current.has_parent(),
        &GameModel::canGoBackwardChanged);
    set(m_hasPrevVar, (current.get_previous_sibling() != nullptr),
        &GameModel::hasPrevVarChanged);
    set(m_hasNextVar, (current.get_sibling() != nullptr),
        &GameModel::hasNextVarChanged);
    set(m_hasVariations, tree.has_variations(),
        &GameModel::hasVariationsChanged);
    set(m_hasEarlierVar, has_earlier_variation(current),
        &GameModel::hasEarlierVarChanged);
    set(m_isMainVar, is_main_variation(current), &GameModel::isMainVarChanged);
    set(m_moveNumber, static_cast<int>(get_move_number(tree, current)),
        &GameModel::moveNumberChanged);
    set(m_movesLeft, static_cast<int>(get_moves_left(tree, current)),
        &GameModel::movesLeftChanged);
    updatePositionInfo();
    bool isGameOver = true;
    for (Color c : bd.get_colors())
        if (bd.has_moves(c))
        {
            isGameOver = false;
            break;
        }
    set(m_isBoardEmpty, bd.get_nu_onboard_pieces() == 0,
        &GameModel::isBoardEmptyChanged);
    set(m_isGameOver, isGameOver, &GameModel::isGameOverChanged);
    updateIsGameEmpty();
    updateIsModified();
    updateMoveAnnotation();
    updatePieces();
    set(m_comment, decode(m_game.get_comment()), &GameModel::commentChanged);
    set(m_toPlay, m_isGameOver ? 0 : bd.get_effective_to_play().to_int(),
        &GameModel::toPlayChanged);
    set(m_altPlayer,
        bd.get_variant() == Variant::classic_3 ? bd.get_alt_player() : 0,
        &GameModel::altPlayerChanged);

    emit positionChanged();
}

//-----------------------------------------------------------------------------
