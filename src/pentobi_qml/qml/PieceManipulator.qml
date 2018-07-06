import QtQuick 2.0

Item {
    id: root

    property var pieceModel
    // True if piece manipulator is at a board location that is a legal move
    property bool legal

    // Fast move animation to make sure that the next grid cell is reached
    // before the next auto-repeat keyboard command
    property bool fastMove: false

    // Manipulator buttons are smaller on desktop with mouse usage
    property real buttonSize: (isDesktop ? 0.14 : 0.20) * root.width

    property real animationsDuration:
        ! visible || ! gameDisplay.enableAnimations ? 0 : fastMove ? 20 : 300

    signal piecePlayed

    Image {
        anchors.fill: root
        source: isDesktop ? theme.getImage("piece-manipulator-desktop")
                              : theme.getImage("piece-manipulator")
        sourceSize { width: width; height: height }
        opacity: ! legal ? 0.4 : 0
        Behavior on opacity { NumberAnimation { duration: 100 } }
    }
    Image {
        anchors.fill: root
        source: isDesktop ? theme.getImage("piece-manipulator-desktop-legal")
                              : theme.getImage("piece-manipulator-legal")
        sourceSize { width: width; height: height }
        opacity: legal ? 0.4 : 0
        Behavior on opacity { NumberAnimation { duration: 100 } }
    }
    MouseArea {
        id: dragArea

        anchors.fill: root
        drag {
            target: root
            filterChildren: true
            minimumX: -width / 2; maximumX: root.parent.width - width / 2
            minimumY: -height / 2; maximumY: root.parent.height - height / 2
        }
        // Consume mouse hover events in case it is over PieceList
        hoverEnabled: isDesktop

        MouseArea {
            anchors.centerIn: dragArea
            width: 0.5 * root.width; height: width
            onClicked: piecePlayed()
        }
        MouseArea {
            anchors {
                top: dragArea.top
                horizontalCenter: dragArea.horizontalCenter
            }
            width: buttonSize; height: width
            onClicked: pieceModel.rotateRight()
        }
        MouseArea {
            anchors {
                right: dragArea.right
                verticalCenter: dragArea.verticalCenter
            }
            width: buttonSize; height: width
            onClicked: pieceModel.flipAcrossX()
        }
        MouseArea {
            anchors {
                bottom: dragArea.bottom
                horizontalCenter: dragArea.horizontalCenter
            }
            width: buttonSize; height: width
            onClicked: pieceModel.flipAcrossY()
        }
        MouseArea {
            anchors { left: dragArea.left; verticalCenter: dragArea.verticalCenter }
            width: buttonSize; height: width
            onClicked: pieceModel.rotateLeft()
        }
    }

    Behavior on x { NumberAnimation { duration: animationsDuration; easing.type: Easing.InOutSine } }
    Behavior on y { NumberAnimation { duration: animationsDuration; easing.type: Easing.InOutSine } }
}
