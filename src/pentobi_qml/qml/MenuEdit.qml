import QtQuick 2.0
import QtQuick.Controls 2.2
import "Main.js" as Logic
import "." as Pentobi

Pentobi.Menu {
    title: Logic.removeShortcut(qsTr("&Edit"))

    Pentobi.Menu {
        title: Logic.removeShortcut(qsTr("Move &Annotation"))
        enabled: gameModel.moveNumber > 0

        MenuItem {
            text: Logic.removeShortcut(qsTr("&None"))
            checkable: true
            autoExclusive: true
            checked: gameModel.moveAnnotation === ""
            onTriggered: gameModel.moveAnnotation = ""
        }
        MenuItem {
            text: Logic.removeShortcut(qsTr("&Very good"))
            checkable: true
            autoExclusive: true
            checked: gameModel.moveAnnotation === "!!"
            onTriggered: gameModel.moveAnnotation = "!!"
        }
        MenuItem {
            text: Logic.removeShortcut(qsTr("&Good"))
            checkable: true
            autoExclusive: true
            checked: gameModel.moveAnnotation === "!"
            onTriggered: gameModel.moveAnnotation = "!"
        }
        MenuItem {
            text: Logic.removeShortcut(qsTr("Int&eresting"))
            checkable: true
            autoExclusive: true
            checked: gameModel.moveAnnotation === "!?"
            onTriggered: gameModel.moveAnnotation = "!?"
        }
        MenuItem {
            text: Logic.removeShortcut(qsTr("&Doubtful"))
            checkable: true
            autoExclusive: true
            checked: gameModel.moveAnnotation === "?!"
            onTriggered: gameModel.moveAnnotation = "?!"
        }
        MenuItem {
            text: Logic.removeShortcut(qsTr("&Bad"))
            checkable: true
            autoExclusive: true
            checked: gameModel.moveAnnotation === "?"
            onTriggered: gameModel.moveAnnotation = "?"
        }
        MenuItem {
            text: Logic.removeShortcut(qsTr("Very B&ad"))
            checkable: true
            autoExclusive: true
            checked: gameModel.moveAnnotation === "??"
            onTriggered: gameModel.moveAnnotation = "??"
        }
    }
    MenuSeparator { }
    MenuItem {
        text: Logic.removeShortcut(qsTr("Make &Main Variation"))
        enabled: ! gameModel.isMainVar
        onTriggered: {
            gameModel.makeMainVar()
            Logic.showTemporaryMessage(qsTr("Made main variation."))
        }
    }
    MenuItem {
        text: Logic.removeShortcut(qsTr("Move Variation &Up"))
        enabled: gameModel.hasPrevVar
        onTriggered: Logic.moveUpVar()
    }
    MenuItem {
        text: Logic.removeShortcut(qsTr("Move Variation &Down"))
        enabled: gameModel.hasNextVar
        onTriggered: Logic.moveDownVar()
    }
    MenuSeparator { }
    MenuItem {
        text: Logic.removeShortcut(qsTr("&Delete All Variations"))
        enabled: gameModel.hasVariations
        onTriggered: Logic.deleteAllVar()
    }
    MenuItem {
        text: Logic.removeShortcut(qsTr("&Truncate"))
        enabled: gameModel.canGoBackward
        onTriggered: Logic.truncate()
    }
    MenuItem {
        text: Logic.removeShortcut(qsTr("Truncate &Children"))
        enabled: gameModel.canGoForward
        onTriggered: Logic.truncateChildren()
    }
    MenuItem {
        text: Logic.removeShortcut(qsTr("&Keep Only Position"))
        enabled: ! gameModel.isBoardEmpty && (gameModel.canGoBackward || gameModel.canGoForward)
        onTriggered: Logic.keepOnlyPosition()
    }
    MenuItem {
        text: Logic.removeShortcut(qsTr("Keep Only &Subtree"))
        enabled: gameModel.canGoBackward && gameModel.canGoForward
        onTriggered: Logic.keepOnlySubtree()
    }
    MenuSeparator { }
    MenuItem {
        text: Logic.removeShortcut(qsTr("S&etup Mode"))
        checkable: true
        enabled: ! gameModel.canGoBackward && ! gameModel.canGoForward
                 && gameModel.moveNumber === 0
        checked: gameDisplay.setupMode
        onTriggered: {
            gameDisplay.setupMode = checked
            if (checked)
                gameDisplay.showPieces()
            else
                Logic.setComputerNone()
        }
    }
    MenuItem {
        text: Logic.removeShortcut(qsTr("&Next Color"))
        onTriggered: gameModel.nextColor()
    }
}
