import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.2
import "." as Pentobi

Pentobi.Dialog {
    id: root

    footer: Pentobi.DialogButtonBox { ButtonClose { } }

    Item {
        implicitWidth: Math.min(column.implicitWidth, 0.9 * rootWindow.width)
        implicitHeight: column.implicitHeight

        Column {
            id: column

            anchors.fill: parent
            spacing: 0.5 * font.pixelSize

            Image {
                source: "qrc:pentobi_icon/pentobi-64.svg"
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Label {
                text: qsTr("Pentobi")
                font {
                    bold: true
                    pixelSize: 1.3 * root.font.pixelSize
                }
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Label {
                text: Qt.application.version
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Label {
                text: qsTr("Computer opponent for the board game Blokus.")
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                width: Math.min(implicitWidth, 0.9 * rootWindow.width)
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Label {
                text: "<a href=\"https://pentobi.sourceforge.io\">pentobi.sourceforge.io</a>"
                elide: Qt.ElideRight
                onLinkActivated: Qt.openUrlExternally(link)
                width: Math.min(implicitWidth, 0.9 * rootWindow.width)
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Label {
                text: qsTr("Copyright © 2011–%1 Markus Enzenberger").arg(2018)
                font.pixelSize: 0.9 * root.font.pixelSize
                opacity: 0.8
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                width: Math.min(implicitWidth, 0.9 * rootWindow.width)
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
}
