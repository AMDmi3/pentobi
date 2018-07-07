//-----------------------------------------------------------------------------
/** @file pentobi_qml/Main.cpp
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <QApplication>
#include <QCommandLineParser>
#include <QTranslator>
#include <QtQml>
#include <QtWebView/QtWebView>
#include "AnalyzeGameModel.h"
#include "GameModel.h"
#include "HelpFileExtractor.h"
#include "PlayerModel.h"
#include "RatingModel.h"
#include "libboardgame_util/Log.h"

using libboardgame_util::RandomGenerator;

//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    libboardgame_util::LogInitializer log_initializer;
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);
    QtWebView::initialize();
    app.setOrganizationName("Pentobi");
    app.setApplicationName("Pentobi");
#ifdef VERSION
    app.setApplicationVersion(VERSION);
#endif
    qmlRegisterType<AnalyzeGameModel>("pentobi", 1, 0, "AnalyzeGameModel");
    qmlRegisterType<GameModel>("pentobi", 1, 0, "GameModel");
    qmlRegisterType<HelpFileExtractor>("pentobi", 1, 0, "HelpFileExtractor");
    qmlRegisterType<PlayerModel>("pentobi", 1, 0, "PlayerModel");
    qmlRegisterType<RatingModel>("pentobi", 1, 0, "RatingModel");
    qmlRegisterInterface<AnalyzeGameElement>("AnalyzeGameElement");
    qmlRegisterInterface<GameMove>("GameModelMove");
    qmlRegisterInterface<PieceModel>("PieceModel");
    QString locale = QLocale::system().name();
    QTranslator translatorPentobi;
    translatorPentobi.load("qml_" + locale, ":qml/i18n");
    app.installTranslator(&translatorPentobi);
    QCommandLineParser parser;
    QCommandLineOption optionNoBook("nobook", "Do not use opening books.");
    parser.addOption(optionNoBook);
    QCommandLineOption optionNoDelay(
                "nodelay", "Do not delay fast computer moves.");
    parser.addOption(optionNoDelay);
    QCommandLineOption optionSeed("seed", "Set random seed to <n>.", "n");
    parser.addOption(optionSeed);
    QCommandLineOption optionThreads("threads", "Use <n> threads (0=auto).",
                                     "n");
    parser.addOption(optionThreads);
#if ! LIBBOARDGAME_DISABLE_LOG
    QCommandLineOption optionVerbose(
                "verbose", "Print logging information to standard error.");
    parser.addOption(optionVerbose);
#endif
    parser.addPositionalArgument("file.blksgf",
                                 "Blokus SGF file to open (optional).");
    parser.addHelpOption();
    parser.process(app);
    try
    {
#if ! LIBBOARDGAME_DISABLE_LOG
        if (! parser.isSet(optionVerbose))
            libboardgame_util::disable_logging();
#endif
        if (parser.isSet(optionNoBook))
            PlayerModel::noBook = true;
        if (parser.isSet(optionNoDelay))
            PlayerModel::noDelay = true;
        bool ok;
        if (parser.isSet(optionSeed))
        {
            auto seed = parser.value(optionSeed).toUInt(&ok);
            if (! ok)
                throw runtime_error("--seed must be a positive number");
            RandomGenerator::set_global_seed(seed);
        }
        if (parser.isSet(optionThreads))
        {
            auto nuThreads = parser.value(optionThreads).toUInt(&ok);
            if (! ok)
                throw runtime_error("--threads must be a positive number");
            PlayerModel::nuThreads = nuThreads;
        }
        QString initialFile;
        auto args = parser.positionalArguments();
        if (! args.empty())
            initialFile = args.at(0);
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("initialFile", initialFile);
        engine.load(QUrl("qrc:///qml/Main.qml"));
        return app.exec();
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}

//-----------------------------------------------------------------------------
