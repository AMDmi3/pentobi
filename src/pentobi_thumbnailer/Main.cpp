//-----------------------------------------------------------------------------
/** @file pentobi_thumbnailer/Main.cpp */
//-----------------------------------------------------------------------------

#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/program_options.hpp>
#include "libboardgame_base/RectGeometry.h"
#include "libboardgame_base/TrigonGeometry.h"
#include "libboardgame_sgf/TreeReader.h"
#include "libboardgame_sgf/Util.h"
#include "libpentobi_base/Tree.h"
#include "libpentobi_gui/BoardPainter.h"

using namespace std;
using boost::is_any_of;
using boost::split;
using boost::trim_copy;
using boost::algorithm::to_lower_copy;
using boost::program_options::command_line_parser;
using boost::program_options::options_description;
using boost::program_options::notify;
using boost::program_options::positional_options_description;
using boost::program_options::store;
using boost::program_options::value;
using boost::program_options::variables_map;
using libboardgame_base::RectGeometry;
using libboardgame_base::TrigonGeometry;
using libboardgame_sgf::Node;
using libboardgame_sgf::TreeReader;
using libpentobi_base::variant_classic;
using libpentobi_base::variant_classic_2;
using libpentobi_base::variant_duo;
using libpentobi_base::variant_junior;
using libpentobi_base::variant_trigon;
using libpentobi_base::variant_trigon_2;
using libpentobi_base::variant_trigon_3;
using libpentobi_base::Variant;
using libpentobi_base::Geometry;
using libpentobi_base::Grid;
using libpentobi_base::PointState;

//-----------------------------------------------------------------------------

namespace {

/** Helper function for getFinalPosition() */
void handleSetup(const char* id, Color c, const Node& node,
                 const Geometry& geometry, Grid<PointState>& pointState)
{
    vector<string> values = node.get_multi_property(id);
    BOOST_FOREACH(const string& s, values)
    {
        if (trim_copy(s).empty())
            continue;
        vector<string> v;
        split(v, s, is_any_of(","));
        BOOST_FOREACH(const string& p_str, v)
        {
            try
            {
                Point p = Point::from_string(p_str);
                if (geometry.is_onboard(p))
                    pointState[p] = c;
            }
            catch (const Point::InvalidString&)
            {
                continue;
            }
        }
    }
}

/** Helper function for getFinalPosition() */
void handleSetupEmpty(const Node& node, const Geometry& geometry,
                      Grid<PointState>& pointState)
{
    vector<string> values = node.get_multi_property("AE");
    BOOST_FOREACH(const string& s, values)
    {
        if (trim_copy(s).empty())
            continue;
        vector<string> v;
        split(v, s, is_any_of(","));
        BOOST_FOREACH(const string& p_str, v)
        {
            try
            {
                Point p = Point::from_string(p_str);
                if (geometry.is_onboard(p))
                    pointState[p] = PointState::empty();
            }
            catch (const Point::InvalidString&)
            {
                continue;
            }
        }
    }
}

/** Get the board state of the final position of the main variation.
    Avoids constructing an instance of a Tree or Game, which would do a costly
    initialization of BoardConst and slow down the thumbnailer unnecessarily. */
bool getFinalPosition(const Node& root, Variant& variant,
                      Grid<PointState>& pointState)
{
    if (! parse_variant(root.get_property("GM", ""), variant))
        return false;
    const Geometry* geometry;
    switch (variant)
    {
    case variant_duo:
    case variant_junior:
        geometry = RectGeometry<Point>::get(14, 14);
        break;
    case variant_classic:
    case variant_classic_2:
        geometry = RectGeometry<Point>::get(20, 20);
        break;
    case variant_trigon:
    case variant_trigon_2:
        geometry = TrigonGeometry<Point>::get(9);
        break;
    case variant_trigon_3:
        geometry = TrigonGeometry<Point>::get(8);
        break;
    default:
        LIBBOARDGAME_ASSERT(false);
        return false;
    }
    pointState.init(*geometry, PointState::empty());
    const Node* node = &root;
    while (node != 0)
    {
        if (libpentobi_base::Tree::has_setup(*node))
        {
            handleSetup("AB", Color(0), *node, *geometry, pointState);
            handleSetup("AW", Color(1), *node, *geometry, pointState);
            handleSetup("A1", Color(0), *node, *geometry, pointState);
            handleSetup("A2", Color(1), *node, *geometry, pointState);
            handleSetup("A3", Color(2), *node, *geometry, pointState);
            handleSetup("A4", Color(3), *node, *geometry, pointState);
            handleSetupEmpty(*node, *geometry, pointState);
            if (node == &root)
                // If the file starts with a setup (e.g. a puzzle), we use this
                // position for the thumbnail.
                break;
        }
        Color c;
        MovePoints points;
        if (libpentobi_base::Tree::get_move(*node, variant, c, points))
        {
            BOOST_FOREACH(Point p, points)
            {
                if (geometry->is_onboard(p))
                    pointState[p] = c;
            }
        }
        node = node->get_first_child_or_null();
    }
    return true;
}

int mainFunction(int argc, char* argv[])
{
    int size = 128;
    vector<string> files;
    options_description normal_options("Options");
    normal_options.add_options()
        ("size,s", value<int>(&size), "Image size");
    options_description hidden_options;
    hidden_options.add_options()
        ("files", value<vector<string>>(&files), "input-file output-file");
    options_description all_options;
    all_options.add(normal_options).add(hidden_options);
    positional_options_description positional_options;
    positional_options.add("files", -1);
    variables_map vm;
    store(command_line_parser(argc, argv).options(all_options).
          positional(positional_options).run(), vm);
    notify(vm);
    if (size <= 0)
    {
        cerr << "Invalid image size\n";
        return 1;
    }
    if (files.size() > 2)
    {
        cerr << "Too many file arguments\n";
        return 1;
    }
    if (files.size() < 2)
    {
        cerr << "Need input and output file argument\n";
        return 1;
    }

    TreeReader reader;
    reader.set_read_only_main_variation(true);
    reader.read(files[0]);
    Variant variant =
        variant_classic; // Initialize to avoid compiler warning
    Grid<PointState> pointState;
    if (! getFinalPosition(reader.get_tree(), variant, pointState))
    {
        cerr << "Not a valid Blokus SGF file\n";
        return 1;
    }

    BoardPainter boardPainter;
    QImage image(size, size, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter;
    painter.begin(&image);
    boardPainter.paintEmptyBoard(painter, size, size, variant,
                                 pointState.get_geometry());
    boardPainter.paintPieces(painter, pointState);
    painter.end();
    QImageWriter writer(QString::fromLocal8Bit(files[1].c_str()));
    writer.setFormat("png");
    if (! writer.write(image))
    {
        cerr << writer.errorString().toStdString() << '\n';
        return 1;
    }
    return 0;
}

} //namespace

//-----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    try
    {
        return mainFunction(argc, argv);
    }
    catch (const exception& e)
    {
        cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}

//-----------------------------------------------------------------------------
