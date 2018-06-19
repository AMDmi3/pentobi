import QtQuick 2.0
import QtQuick.Controls 2.2
import QtQuick.Window 2.0

Dialog {
    property alias computerPlays0: checkBox0.checked
    property alias computerPlays1: checkBox1.checked
    property alias computerPlays2: checkBox2.checked
    property alias computerPlays3: checkBox3.checked

    title: qsTr("Computer Colors")
    standardButtons: Dialog.Ok | Dialog.Cancel
    x: (rootWindow.width - width) / 2
    y: (rootWindow.height - height) / 2
    parent: ApplicationWindow.overlay

    Column {
        spacing: Math.round(Screen.pixelDensity * 1.5)
        width: Math.min(label.height * 15, 0.95 * rootWindow.width)

        Label {
            id: label

            width: parent.width
            text: qsTr("Computer plays:")
        }
        CheckBox {
            id: checkBox0

            enabled: ! isRated
            text: {
                if (gameModel.nuColors == 4 && gameModel.nuPlayers == 2)
                      return qsTr("Blue/Red")
                if (gameModel.gameVariant === "duo")
                    return qsTr("Purple")
                if (gameModel.gameVariant === "junior")
                    return qsTr("Green")
                return qsTr("Blue")
            }
            onClicked:
                if (gameModel.nuColors == 4 && gameModel.nuPlayers == 2)
                    computerPlays2 = checked
        }
        CheckBox {
            id: checkBox1

            enabled: ! isRated
            text: {
                if (gameModel.nuColors == 4 && gameModel.nuPlayers == 2)
                    return qsTr("Yellow/Green")
                if (gameModel.gameVariant === "duo" || gameModel.gameVariant === "junior")
                    return qsTr("Orange")
                if (gameModel.nuColors == 2)
                    return qsTr("Green")
                return qsTr("Yellow")
            }
            onClicked:
                if (gameModel.nuColors == 4 && gameModel.nuPlayers == 2)
                    computerPlays3 = checked
        }
        CheckBox {
            id: checkBox2

            enabled: ! isRated
            text: qsTr("Red")
            visible: gameModel.nuPlayers > 2
        }
        CheckBox {
            id: checkBox3

            enabled: ! isRated
            text: qsTr("Green")
            visible: gameModel.nuPlayers > 3
        }
    }
}
