import QtQuick 2.0
import QtQuick.Controls.Styles 1.4

QtObject {
    property color busyIndicatorColor: fontColorScore
    property color commentBorderColor: "#b4b3b3"
    property color commentBorderColorFocus: "#0066ff"
    property color messageTextColor: "black"
    property color messageBackgroundColor: "#cac9c9"
    property color backgroundColor: "#efebe7"
    property color fontColorScore: "#666"
    property color fontColorCoordinates: "#64615E"
    property color fontColorPosInfo: "#282625"
    property color colorBlue: "#0077D2"
    property color colorYellow: "#ebc300"
    property color colorRed: "#E63E2C"
    property color colorGreen: "#00C000"
    property color colorPurple: "#a12CCF"
    property color colorOrange: "#F09217"
    property color colorStartingPoint: "#767074"
    property color backgroundButtonPressed: Qt.darker(backgroundColor, 1.1)
    property color selectionColor: "steelblue"
    property color selectedTextColor: "#EEE"
    property color toolBarTextColor: "black"
    property color analyzeBackgroundColor: backgroundColor
    property color analyzeLineColor: "black"
    property color analyzeMiddleLineColor: "grey"
    property real pieceListOpacity: 1
    property real toPlayColorLighter: 0.3

    function getImage(name) { return "themes/light/" + name + ".svg" }
}
