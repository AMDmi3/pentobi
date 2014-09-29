//-----------------------------------------------------------------------------
/** @file pentobi_qml/qml/Message.qml
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

import QtQuick 2.0

Rectangle {
    id: root

    property real fontSize

    property real _margin: 0.5 * fontSize

    signal clicked()

    function clear() {
        opacity = 0
    }
    function show(text) {
        messageTimer.stop()
        messageText.text = text
        opacity = 0.8
    }
    function showTemporary(text, duration) {
        messageText.text = text
        opacity = 0.8
        messageTimer.interval = duration
        messageTimer.restart()
    }

    width: messageText.contentWidth + 2 * _margin
    height: messageText.contentHeight + 2 * _margin
    radius: _margin
    color: theme.backgroundColorMessage
    opacity: 0

    Text {
        id: messageText
        font.pixelSize: root.fontSize
        color: theme.fontColorMessage
        renderType: Text.NativeRendering
        anchors.centerIn: parent
    }
    MouseArea {
        anchors.fill: root
        enabled: root.opacity > 0
        onClicked: root.clicked()
    }
    Timer {
        id: messageTimer
        onTriggered: root.opacity = 0
    }
    Behavior on opacity { NumberAnimation { duration: 400 } }
}
