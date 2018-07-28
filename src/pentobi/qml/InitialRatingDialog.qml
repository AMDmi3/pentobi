//-----------------------------------------------------------------------------
/** @file pentobi/qml/InitialRatingDialog.qml
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

import QtQuick 2.0
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.0
import QtQuick.Window 2.0
import "." as Pentobi
import "Main.js" as Logic

Pentobi.Dialog {
    footer: DialogButtonBoxOkCancel { }
    onAccepted: {
        ratingModel.setInitialRating(Math.round(slider.value))
        Logic.ratedGameNoVerify()
    }

    ColumnLayout
    {
        Item {
            implicitWidth: {
                // Wrap long text
                var w = font.pixelSize * 25
                w = Math.min(w, 0.9 * rootWindow.width)
                return w
            }
            implicitHeight: textLabel.implicitHeight
            Layout.fillWidth: true

            Label {
                id: textLabel

                anchors.fill: parent
                text: qsTr("Estimate your playing strength to initialize your rating for this game variant.")
                wrapMode: Text.Wrap
            }
        }
        RowLayout {
            Label {
                text: qsTr("Your initial rating:")
            }
            Label {
                text: Math.round(slider.value)
                font.bold: true
            }
        }
        Slider {
            id: slider

            value: 800
            from: 800; to: 2000; stepSize: 100
            Layout.fillWidth: true
        }
        RowLayout {
            Layout.fillWidth: true

            Label { text: qsTr("Beginner") }
            Item { Layout.fillWidth: true }
            Label { text: qsTr("Expert") }
        }
    }
}
