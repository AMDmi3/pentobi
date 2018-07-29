//-----------------------------------------------------------------------------
/** @file pentobi/qml/ToolBar.qml
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0
import "." as Pentobi
import "Main.js" as Logic

RowLayout {
    id: root

    property real buttonPadding: Math.round(1.5 * Screen.pixelDensity)

    function clickMenuButton() {
        menuButton.checked = true
        menuButton.onClicked()
        menu.item.currentIndex = 0
    }
    // Automatic width in Pentobi.Menu doesn't work with dynamic menus and even
    // if we used a fixed width for the recent files menu, there is a bug
    // (tested with Qt 5.11.1) that makes item texts not vertically centered
    // after insertItem(). So we should call this function to enforce a new
    // menu creation whenever gameModel.recentFiles has changed.
    function destroyMenu() {
        menu.sourceComponent = null
    }

    spacing: 0

    Label {
        // Like the label used for desktop after the toolbuttons, but with
        // shorter text for small smartphone screens and a message box instead
        // of a tooltip for showing the full file path because tooltips are
        // not wrapped if larger than screen (last tested with Qt 5.11.1)
        visible: ! isDesktop
        Layout.fillWidth: true
        Layout.leftMargin: root.height / 10
        color: theme.colorText
        elide: Text.ElideRight
        text: {
            if (gameDisplay.setupMode) return qsTr("Setup")
            if (isRated) return qsTr("Rated")
            return Logic.getFileLabel(gameModel.file, gameModel.isModified)
        }
        MouseArea {
            anchors.fill: parent
            onClicked:
                if (gameDisplay.setupMode) {
                    gameDisplay.setupMode = false
                    Logic.setComputerNone()
                }
                else if (gameModel.file !== "")
                    Logic.showInfo(Logic.getFileInfo(gameModel.file, gameModel.isModified))
        }
    }
    Pentobi.Button {
        padding: buttonPadding
        icon.source: theme.getImage("pentobi-newgame")
        action: actions.actionNew
        visible: isDesktop || enabled
        ToolTip.text: qsTr("Start a new game")
    }
    Pentobi.Button {
        visible: isDesktop
        padding: buttonPadding
        icon.source: theme.getImage("pentobi-rated-game")
        action: actions.actionNewRated
        ToolTip.text: qsTr("Start a rated game")
    }
    Pentobi.Button {
        padding: buttonPadding
        icon.source: theme.getImage("pentobi-undo")
        action: actions.actionUndo
        visible: isDesktop || enabled
        ToolTip.text: action.text.replace("&", "")
    }
    Pentobi.Button {
        padding: buttonPadding
        icon.source: theme.getImage("pentobi-computer-colors")
        action: actions.actionComputerSettings
        visible: isDesktop || enabled
        ToolTip.text: qsTr("Set the colors played by the computer")
    }
    Pentobi.Button {
        padding: buttonPadding
        icon.source: theme.getImage("pentobi-play")
        action: actions.actionPlay
        visible: isDesktop || enabled
        ToolTip.text: {
            var toPlay = gameModel.toPlay
            if (gameModel.gameVariant === "classic_3" && toPlay === 3)
                toPlay = gameModel.altPlayer
            if ((computerPlays0 && toPlay === 0)
                    || (computerPlays1 && toPlay === 1)
                    || (computerPlays2 && toPlay === 2)
                    || (computerPlays3 && toPlay === 3))
                return qsTr("Make the computer continue to play the current color")
            return qsTr("Make the computer play the current color")
        }
    }
    Pentobi.Button {
        visible: isDesktop
        padding: buttonPadding
        icon.source: theme.getImage("pentobi-beginning")
        action: actions.actionBeginning
        ToolTip.text: qsTr("Go to beginning of game")
    }
    Pentobi.Button {
        visible: isDesktop
        padding: buttonPadding
        icon.source: theme.getImage("pentobi-backward10")
        action: actions.actionBackward10
        autoRepeat: true
        ToolTip.text: qsTr("Go ten moves backward")
    }
    Pentobi.Button {
        visible: isDesktop
        padding: buttonPadding
        icon.source: theme.getImage("pentobi-backward")
        action: actions.actionBackward
        autoRepeat: true
        ToolTip.text: qsTr("Go one move backward")
    }
    Pentobi.Button {
        visible: isDesktop
        padding: buttonPadding
        icon.source: theme.getImage("pentobi-forward")
        action: actions.actionForward
        autoRepeat: true
        ToolTip.text: qsTr("Go one move forward")
    }
    Pentobi.Button {
        visible: isDesktop
        padding: buttonPadding
        icon.source: theme.getImage("pentobi-forward10")
        action: actions.actionForward10
        autoRepeat: true
        ToolTip.text: qsTr("Go ten moves forward")
    }
    Pentobi.Button {
        visible: isDesktop
        padding: buttonPadding
        icon.source: theme.getImage("pentobi-end")
        action: actions.actionEnd
        ToolTip.text: qsTr("Go to end of moves")
    }
    Pentobi.Button {
        visible: isDesktop
        padding: buttonPadding
        icon.source: theme.getImage("pentobi-previous-variation")
        action: actions.actionPrevVar
        ToolTip.text: qsTr("Go to previous variation")
    }
    Pentobi.Button {
        visible: isDesktop
        padding: buttonPadding
        icon.source: theme.getImage("pentobi-next-variation")
        action: actions.actionNextVar
        ToolTip.text: qsTr("Go to next variation")
    }
    Item {
        visible: isDesktop
        Layout.preferredWidth: parent.height / 2
    }
    Label {
        visible: isDesktop
        text: {
            if (gameDisplay.setupMode) return qsTr("Setup mode")
            if (isRated) return qsTr("Rated game")
            return Logic.getFileLabel(gameModel.file, gameModel.isModified)
        }
        color: theme.colorText
        elide: Text.ElideRight
        Layout.fillWidth: true

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            ToolTip.text: Logic.getFileInfo(gameModel.file, gameModel.isModified)
            ToolTip.visible: containsMouse && gameModel.file !== "" && ! gameDisplay.setupMode
            ToolTip.delay: 700
            ToolTip.timeout: 9000
        }
    }
    // We want to open/close the menu by button clicks and support auto-closing
    // with escape/outside clicks, which is suprisingly difficult because
    // auto-close events close the menu before the button signal handlers are
    // called. The current code almost works, except for the use case of
    // opening the menu with a click and closing it with escape, which leaves
    // the button checked and needs two clicks to open the menu again.
    Pentobi.Button {
        id: menuButton

        padding: buttonPadding
        icon.source: theme.getImage("menu")
        checkable: true
        hoverEnabled: isDesktop
        onHoveredChanged: checked = (menu.item && menu.item.opened)
        down: pressed || (menu.item && menu.item.opened)
        onClicked: {
            if (! menu.item)
                menu.sourceComponent = menuComponent
            if (checked) {
                if (isAndroid)
                    menu.item.popup()
                else
                    menu.item.popup(0, height)
            }
        }

        Loader {
            id: menu

            Component {
                id: menuComponent

                Pentobi.Menu {
                    onClosed:
                        if (! menuButton.hovered)
                            menuButton.checked = false

                    MenuGame { }
                    MenuGo { }
                    MenuEdit { }
                    MenuView { }
                    MenuComputer { }
                    MenuTools { }
                    MenuHelp { }
                }
            }
        }
    }
}
