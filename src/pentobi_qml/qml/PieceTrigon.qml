import QtQuick 2.0

// See PieceClassic.qml for comments
Item
{
    id: root

    property var pieceModel
    property string colorName
    property bool isPicked
    property Item parentUnplayed
    property real gridWidth
    property real gridHeight
    property bool isMarked
    property string imageName: theme.getImage("triangle-" + colorName)
    property string imageNameDownward:
        theme.getImage("triangle-down-" + colorName)
    property real pieceAngle: {
        var flX = Math.abs(flipX.angle % 360 - 180) < 90
        var flY = Math.abs(flipY.angle % 360 - 180) < 90
        var angle = rotation
        if (flX && flY) angle += 180
        else if (flX) angle += 120
        else if (flY) angle += 300
        return angle
    }
    property real imageOpacity0: imageOpacity(pieceAngle, 0)
    property real imageOpacity60: imageOpacity(pieceAngle, 60)
    property real imageOpacity120: imageOpacity(pieceAngle, 120)
    property real imageOpacity180: imageOpacity(pieceAngle, 180)
    property real imageOpacity240: imageOpacity(pieceAngle, 240)
    property real imageOpacity300: imageOpacity(pieceAngle, 300)

    z: 1
    transform: [
        Rotation {
            id: flipX

            axis { x: 1; y: 0; z: 0 }
            origin { x: width / 2; y: height / 2 }
        },
        Rotation {
            id: flipY

            axis { x: 0; y: 1; z: 0 }
            origin { x: width / 2; y: height / 2 }
        }
    ]

    function _isDownward(pos) { return (pos.x % 2 == 0) != (pos.y % 2 == 0) }
    function imageOpacity(pieceAngle, imgAngle) {
        var angle = (((pieceAngle - imgAngle) % 360) + 360) % 360
        return (angle >= 60 && angle <= 300 ? 0 : 2 * Math.cos(angle * Math.PI / 180) - 1)
    }

    Repeater {
        model: pieceModel.elements

        Triangle {
            isDownward: _isDownward(modelData)
            width: 2 * gridWidth
            height: gridHeight
            x: (modelData.x - pieceModel.center.x - 0.5) * gridWidth
            y: (modelData.y - pieceModel.center.y) * gridHeight
        }
    }
    Rectangle {
        opacity: isMarked ? 0.5 : 0
        color: colorName == "blue" || colorName == "red" ?
                   "white" : "#333333"
        width: 0.3 * gridHeight
        height: width
        radius: width / 2
        x: (pieceModel.labelPos.x - pieceModel.center.x + 0.5)
           * gridWidth - width / 2
        y: (pieceModel.labelPos.y - pieceModel.center.y
            + (_isDownward(pieceModel.labelPos) ? 1 : 2) / 3)
           * gridHeight - height / 2
        Behavior on opacity { NumberAnimation { duration: 80 } }
    }
    StateGroup {
        state: pieceModel.state

        states: [
            State {
                name: "rot60"
                PropertyChanges { target: root; rotation: 60 }
            },
            State {
                name: "rot120"
                PropertyChanges { target: root; rotation: 120 }
            },
            State {
                name: "rot180"
                PropertyChanges { target: root; rotation: 180 }
            },
            State {
                name: "rot240"
                PropertyChanges { target: root; rotation: 240 }
            },
            State {
                name: "rot300"
                PropertyChanges { target: root; rotation: 300 }
            },
            State {
                name: "flip"
                PropertyChanges { target: flipX; angle: 180 }
            },
            State {
                name: "rot60Flip"
                PropertyChanges { target: root; rotation: 60 }
                PropertyChanges { target: flipX; angle: 180 }
            },
            State {
                name: "rot120Flip"
                PropertyChanges { target: root; rotation: 120 }
                PropertyChanges { target: flipX; angle: 180 }
            },
            State {
                name: "rot180Flip"
                PropertyChanges { target: root; rotation: 180 }
                PropertyChanges { target: flipX; angle: 180 }
            },
            State {
                name: "rot240Flip"
                PropertyChanges { target: root; rotation: 240 }
                PropertyChanges { target: flipX; angle: 180 }
            },
            State {
                name: "rot300Flip"
                PropertyChanges { target: root; rotation: 300 }
                PropertyChanges { target: flipX; angle: 180 }
            }
        ]

        transitions: [
            Transition {
                from: ",rot60,rot120,rot180,rot240,rot300"; to: from
                enabled: enableAnimations

                PieceRotationAnimation { }
            },
            Transition {
                from: "flip,rot60Flip,rot120Flip,rot180Flip,rot240Flip,rot300Flip"; to: from
                enabled: enableAnimations

                PieceRotationAnimation { }
            },
            Transition {
                from: ",flip"; to: from
                enabled: enableAnimations

                PieceFlipAnimation { target: flipX }
            },
            Transition {
                from: "rot60,rot60Flip"; to: from
                enabled: enableAnimations

                PieceFlipAnimation { target: flipX }
            },
            Transition {
                from: "rot120,rot120Flip"; to: from
                enabled: enableAnimations

                PieceFlipAnimation { target: flipX }
            },
            Transition {
                from: "rot180,rot180Flip"; to: from
                enabled: enableAnimations

                PieceFlipAnimation { target: flipX }
            },
            Transition {
                from: "rot240,rot240Flip"; to: from
                enabled: enableAnimations

                PieceFlipAnimation { target: flipX }
            },
            Transition {
                from: "rot300,rot300Flip"; to: from
                enabled: enableAnimations

                PieceFlipAnimation { target: flipX }
            },
            Transition {
                from: ",rot180Flip"; to: from
                enabled: enableAnimations

                SequentialAnimation {
                    PropertyAction { property: "rotation"; value: rotation }
                    PropertyAction {
                        target: flipX; property: "angle"; value: flipX.angle
                    }
                    PieceFlipAnimation { target: flipY; to: 180 }
                    PropertyAction { target: flipY; property: "angle"; value: 0 }
                }
            },
            Transition {
                from: "rot60,rot240Flip"; to: from
                enabled: enableAnimations

                SequentialAnimation {
                    PropertyAction { property: "rotation"; value: rotation }
                    PropertyAction {
                        target: flipX; property: "angle"; value: flipX.angle
                    }
                    PieceFlipAnimation { target: flipY; to: 180 }
                    PropertyAction { target: flipY; property: "angle"; value: 0 }
                }
            },
            Transition {
                from: "rot120,rot300Flip"; to: from
                enabled: enableAnimations

                SequentialAnimation {
                    PropertyAction { property: "rotation"; value: rotation }
                    PropertyAction {
                        target: flipX; property: "angle"; value: flipX.angle
                    }
                    PieceFlipAnimation { target: flipY; to: 180 }
                    PropertyAction { target: flipY; property: "angle"; value: 0 }
                }
            },
            Transition {
                from: "rot180,flip"; to: from
                enabled: enableAnimations

                SequentialAnimation {
                    PropertyAction { property: "rotation"; value: rotation }
                    PropertyAction {
                        target: flipX; property: "angle"; value: flipX.angle
                    }
                    PieceFlipAnimation { target: flipY; to: 180 }
                    PropertyAction { target: flipY; property: "angle"; value: 0 }
                }
            },
            Transition {
                from: "rot240,rot60Flip"; to: from
                enabled: enableAnimations

                SequentialAnimation {
                    PropertyAction { property: "rotation"; value: rotation }
                    PropertyAction {
                        target: flipX; property: "angle"; value: flipX.angle
                    }
                    PieceFlipAnimation { target: flipY; to: 180 }
                    PropertyAction { target: flipY; property: "angle"; value: 0 }
                }
            },
            Transition {
                from: "rot300,rot120Flip"; to: from
                enabled: enableAnimations

                SequentialAnimation {
                    PropertyAction { property: "rotation"; value: rotation }
                    PropertyAction {
                        target: flipX; property: "angle"; value: flipX.angle
                    }
                    PieceFlipAnimation { target: flipY; to: 180 }
                    PropertyAction { target: flipY; property: "angle"; value: 0 }
                }
            }
        ]
    }

    states: [
        State {
            name: "picked"
            when: isPicked

            PropertyChanges {
                target: root
                gridWidth: board.gridWidth
                gridHeight: board.gridHeight
            }
            ParentChange {
                target: root
                parent: pieceManipulator
                x: pieceManipulator.width / 2
                y: pieceManipulator.height / 2
            }
        },
        State {
            name: "played"
            when: pieceModel.isPlayed

            PropertyChanges {
                target: root
                gridWidth: board.gridWidth
                gridHeight: board.gridHeight
            }
            ParentChange {
                target: root
                parent: board
                x: board.mapFromGameX(pieceModel.gameCoord.x)
                y: board.mapFromGameY(pieceModel.gameCoord.y)
            }
        },
        State {
            name: "unplayed"
            when: parentUnplayed != null

            PropertyChanges {
                target: root
                gridWidth: 0.13 * parentUnplayed.width
                gridHeight: Math.sqrt(3) * gridWidth
            }
            ParentChange {
                target: root
                parent: parentUnplayed
                x: parentUnplayed.width / 2
                y: parentUnplayed.height / 2
            }
        }
    ]

    transitions:
        Transition {
            from: "unplayed,picked,played"; to: from
            enabled: enableAnimations

            ParentAnimation {
                via: gameDisplay
                NumberAnimation {
                    properties: "x,y,gridWidth,gridHeight"
                    duration: 300
                    easing.type: Easing.OutSine
                }
            }
    }
}
