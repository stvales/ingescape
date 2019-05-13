/*
 *	IngeScape Editor
 *
 *  Copyright © 2017 Ingenuity i/o. All rights reserved.
 *
 *	See license terms for the rights and conditions
 *	defined by copyright holders.
 *
 *
 *	Contributors:
 *      Vincent Peyruqueou  <peyruqueou@ingenuity.io>
 *
 */

import QtQuick 2.8
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
//import QtQuick.Controls 2.0 as Controls2

import I2Quick 1.0

import INGESCAPE 1.0

import "../theme" as Theme;


Rectangle {
    id: rootItem


    //--------------------------------
    //
    // Properties
    //
    //--------------------------------
    // Controller associated to our view
    property var controller : null;

    // Model associated to our QML item
    property ActionInMappingVM actionInMappingVM: null;

    property ActionM actionM: actionInMappingVM ? actionInMappingVM.action : null;

    property LinkInputVM linkInput: actionInMappingVM ? actionInMappingVM.linkInput : null;
    property LinkOutputVM linkOutput: actionInMappingVM ? actionInMappingVM.linkOutput : null;

    // false if the agent is dropping and the drop is not available, true otherwise
    //property bool dropEnabled: true

    // Flag indicating if the mouse is hover our agent
    property bool actionItemIsHovered: mouseArea.containsMouse

    // To check if our item is selected or not
    property bool _isSelected: (controller && rootItem.actionInMappingVM && (controller.selectedAction === rootItem.actionInMappingVM))

    width: 150
    height: 100
    radius: 50 // 75


    // Init position of our agent
    x: (actionInMappingVM && actionInMappingVM.position) ? actionInMappingVM.position.x : 0
    y: (actionInMappingVM && actionInMappingVM.position) ? actionInMappingVM.position.y : 0


    color: mouseArea.pressed ? IngeScapeTheme.darkGreyColor2
                             : IngeScapeTheme.darkBlueGreyColor


    border {
        color: IngeScapeTheme.selectionColor
        width: rootItem._isSelected ? 1 : 0
    }


    //--------------------------------
    //
    // Behaviors
    //
    //--------------------------------

    //
    // Bindings to save the position of our agent when we move it (drag-n-drop)
    //
    Binding {
        target: rootItem.actionInMappingVM
        property: "position"
        value: Qt.point(rootItem.x, rootItem.y)
    }


    // Animate our border when our item is selected / unselected
    Behavior on border.width {
        NumberAnimation {}
    }


    //--------------------------------
    //
    // Signals
    //
    //--------------------------------



    //--------------------------------
    //
    // Functions
    //
    //--------------------------------



    //--------------------------------
    //
    // Content
    //
    //--------------------------------

    MouseArea {
        id: mouseArea

        anchors.fill: parent

        hoverEnabled: true

        drag.target: parent
        // Disable smoothed so that the Item pixel from where we started the drag remains under the mouse cursor
        drag.smoothed: false


        onPressed: {
            // bring our agent to front
            parent.z = rootItem.parent.maxZ++;
        }

        onClicked: {
            if (controller && actionInMappingVM) {
                if (controller.selectedAction === actionInMappingVM)
                {
                    controller.selectedAction = null;
                }
                else {
                    controller.selectedAction = actionInMappingVM;
                }
            }
        }


        //------------------------------------------
        //
        // Input slot
        //
        //------------------------------------------
        Item {
            id: inputSlotItem

            height: 22
            anchors {
                left: parent.left
                right: parent.right
                verticalCenter: parent.verticalCenter
            }

            Rectangle {
                id: draggablePointFROM

                parent: draggablePointFROM.dragActive ? rootItem.parent  : linkPoint

                height: linkPoint.height
                width: height
                radius: height/2

                property bool dragActive: mouseAreaPointFROM.drag.active;
                property var actionInMappingVMOfInput: rootItem.actionInMappingVM;
                property var inputSlotModel: rootItem.linkInput

                Drag.active: mouseAreaPointFROM.drag.active;
                Drag.hotSpot.x: width/2
                Drag.hotSpot.y: height/2
                Drag.keys: ["InputSlotItem"]

                color: draggablePointFROM.dragActive ? IngeScapeTheme.blackColor : "transparent"
                border {
                    width: 1
                    color: draggablePointFROM.dragActive ? linkPoint.color : "transparent"
                }

                MouseArea {
                    id: mouseAreaPointFROM

                    anchors.fill: parent

                    drag.target: parent
                    // Disable smoothed so that the Item pixel from where we started
                    // the drag remains under the mouse cursor
                    drag.smoothed: false

                    hoverEnabled: true

                    cursorShape: draggablePointFROM.dragActive ? Qt.ClosedHandCursor : Qt.PointingHandCursor

                    onPressed: {
                        draggablePointFROM.z = rootItem.parent.maxZ++;
                        linkDraggablePoint.z = rootItem.parent.maxZ++;
                    }

                    onReleased: {
                        // Drop our item
                        draggablePointFROM.Drag.drop();

                        // replace the draggablePointTO
                        draggablePointFROM.x = 0
                        draggablePointFROM.y = 0
                    }
                }

                Link {
                    id: linkDraggablePoint

                    parent: rootItem.parent

                    strokeDashArray: "5, 5"
                    visible: draggablePointFROM.dragActive

                    secondPoint: Qt.point(rootItem.linkInput.position.x, rootItem.linkInput.position.y)
                    firstPoint: Qt.point(draggablePointFROM.x + draggablePointFROM.width, draggablePointFROM.y + draggablePointFROM.height/2)

                    defaultColor: linkPoint.color
                }
            }


            Rectangle {
                id: linkPoint

                anchors {
                    horizontalCenter: parent.left
                    verticalCenter: parent.verticalCenter
                }

                height: 15
                width: height
                radius: height/2

                border {
                    width: 0
                    color: IngeScapeTheme.whiteColor
                }
                color: IngeScapeEditorTheme.purpleColor
            }


            DropArea {
                id: inputDropArea

                anchors {
                    fill: linkPoint
                    margins: -3
                }

                // Only accept drag events from output slot items
                keys: ["OutputSlotItem"]

                onEntered: {
                    if (drag.source !== null)
                    {
                        var dragItem = drag.source;

                        if ((typeof dragItem.dragActive !== 'undefined') && dragItem.outputSlotModel
                                && dragItem.outputSlotModel.canLinkWith(rootItem.linkInput))
                        {
                            dragItem.color = dragItem.border.color;
                            linkPoint.border.width = 2;
                            linkPoint.scale = 1.2;
                        }
                        else
                        {
                            console.log("(action) inputDropArea: can NOT Link " + dragItem.outputSlotModel + " and " + rootItem.linkInput)
                        }
                    }
                    else
                    {
                        console.log("(action) inputDropArea: no source " + drag.source)
                    }
                }


                onExited: {
                    var dragItem = drag.source;

                    if (typeof dragItem.dragActive !== 'undefined')
                    {
                        dragItem.color = "transparent";
                        linkPoint.border.width = 0;
                        linkPoint.scale = 1;
                    }
                }


                onDropped: {
                    var dragItem = drag.source;

                    if (dragItem && (typeof dragItem.outputSlotModel !== 'undefined')
                            && controller && rootItem.actionInMappingVM && rootItem.linkInput)
                    {
                        dragItem.color = "transparent";
                        linkPoint.border.width = 0;
                        linkPoint.scale = 1;

                        // Action
                        if (dragItem.actionInMappingVMOfOutput)
                        {
                            //console.log("(action) inputDropArea: create a link from " + dragItem.actionInMappingVMOfOutput + "." + dragItem.outputSlotModel + " to " + rootItem.linkInput);
                            controller.dropLinkBetweenTwoActions(dragItem.actionInMappingVMOfOutput, dragItem.outputSlotModel, rootItem.actionInMappingVM, rootItem.linkInput);
                        }
                        // Agent
                        else if (dragItem.agentInMappingVMOfOutput)
                        {
                            //console.log("(action) inputDropArea: create a link from " + dragItem.agentInMappingVMOfOutput + "." + dragItem.outputSlotModel + " to " + rootItem.linkInput);
                            controller.dropLinkFromAgentToAction(dragItem.agentInMappingVMOfOutput, dragItem.outputSlotModel, rootItem.actionInMappingVM, rootItem.linkInput);
                        }
                    }
                }

            }



            //
            // Bindings to save the anchor point of our input slot
            // i.e. the point used to draw a link
            //
            Binding {
                target: rootItem.linkInput

                property: "position"

                value: Qt.point(rootItem.x + inputSlotItem.x + linkPoint.x + linkPoint.width/2,
                                rootItem.y + inputSlotItem.y + linkPoint.y + linkPoint.height/2)
            }
        }


        //------------------------------------------
        //
        // Output slot
        //
        //------------------------------------------
        Item {
            id: outputSlotItem

            height: 22
            anchors {
                left: parent.left
                right: parent.right
                verticalCenter: parent.verticalCenter
            }

            Rectangle {
                id: draggablePointTO

                parent: draggablePointTO.dragActive ? rootItem.parent : linkPointOut

                height: linkPointOut.height
                width: height
                radius: height/2

                color: draggablePointTO.dragActive ? IngeScapeEditorTheme.agentsMappingBackgroundColor : "transparent"
                border {
                    width: 1
                    color: draggablePointTO.dragActive ? linkPointOut.color : "transparent"
                }

                property bool dragActive : mouseAreaPointTO.drag.active;
                property var actionInMappingVMOfOutput : rootItem.actionInMappingVM
                property var outputSlotModel: rootItem.linkOutput

                Drag.active: draggablePointTO.dragActive;
                Drag.hotSpot.x: width/2
                Drag.hotSpot.y: height/2
                Drag.keys: ["OutputSlotItem"]


                MouseArea {
                    id: mouseAreaPointTO

                    anchors.fill: parent

                    drag.target: parent
                    // Disable smoothed so that the Item pixel from where we started
                    // the drag remains under the mouse cursor
                    drag.smoothed: false

                    hoverEnabled: true

                    cursorShape: draggablePointTO.dragActive ? Qt.ClosedHandCursor : Qt.PointingHandCursor

                    onPressed: {
                        draggablePointTO.z = rootItem.parent.maxZ++;
                        linkDraggablePointTO.z = rootItem.parent.maxZ++;
                    }

                    onReleased: {
                        // Drop our item
                        draggablePointTO.Drag.drop();

                        // replace the draggablePointTO
                        draggablePointTO.x = 0
                        draggablePointTO.y = 0
                    }
                }



                Link {
                    id: linkDraggablePointTO

                    parent: rootItem.parent

                    strokeDashArray: "5, 5"
                    visible: draggablePointTO.dragActive

                    firstPoint: Qt.point(rootItem.linkOutput.position.x, rootItem.linkOutput.position.y)
                    secondPoint: Qt.point(draggablePointTO.x, draggablePointTO.y + draggablePointTO.height/2)

                    defaultColor: linkPointOut.color
                }
            }



            Rectangle {
                id: linkPointOut

                anchors {
                    horizontalCenter: parent.right
                    verticalCenter: parent.verticalCenter
                }

                height: 15
                width: height
                radius: height/2

                border {
                    width: 0
                    color: IngeScapeTheme.whiteColor
                }
                color: IngeScapeEditorTheme.purpleColor
            }



            DropArea {
                id: outputDropArea

                anchors {
                    fill: linkPointOut
                    margins: -3
                }

                // Only accept drag events from input slot items
                keys: ["InputSlotItem"]

                onEntered: {
                    if (drag.source !== null)
                    {
                        var dragItem = drag.source;

                        if ((typeof dragItem.dragActive !== 'undefined') && rootItem.linkOutput
                                && rootItem.linkOutput.canLinkWith(dragItem.inputSlotModel))
                        {
                            dragItem.color = dragItem.border.color;
                            linkPointOut.border.width = 2;
                            linkPointOut.scale = 1.2;
                        }
                        else
                        {
                            console.log("(action) outputDropArea: can NOT Link " + rootItem.linkOutput + " and " + dragItem.inputSlotModel)
                        }
                    }
                    else
                    {
                        console.log("(action) outputDropArea: no source " + drag.source)
                    }
                }


                onExited: {
                    var dragItem = drag.source;

                    if (dragItem && (typeof dragItem.dragActive !== 'undefined'))
                    {
                        dragItem.color = "transparent";
                        linkPointOut.border.width = 0;
                        linkPointOut.scale = 1;
                    }
                }

                onDropped: {
                    var dragItem = drag.source;

                    if (dragItem && (typeof dragItem.inputSlotModel !== 'undefined')
                            && controller && rootItem.actionInMappingVM && rootItem.linkOutput)
                    {
                        dragItem.color = "transparent";
                        linkPointOut.border.width = 0;
                        linkPointOut.scale = 1;

                        // Action
                        if (dragItem.actionInMappingVMOfInput)
                        {
                            //console.log("(action) outputDropArea: create a link from " + rootItem.linkOutput + " to " + dragItem.actionInMappingVMOfInput + "." + dragItem.inputSlotModel);
                            controller.dropLinkBetweenTwoActions(rootItem.actionInMappingVM, rootItem.linkOutput, dragItem.actionInMappingVMOfInput, dragItem.inputSlotModel);
                        }
                        // Agent
                        else if (dragItem.agentInMappingVMOfInput)
                        {
                            //console.log("(action) outputDropArea: create a link from " + rootItem.linkOutput + " to " + dragItem.agentInMappingVMOfInput + "." + dragItem.inputSlotModel);
                            controller.dropLinkFromActionToAgent(rootItem.actionInMappingVM, rootItem.linkOutput, dragItem.agentInMappingVMOfInput, dragItem.inputSlotModel);
                        }
                    }
                }
            }



            //
            // Bindings to save the anchor point of our input slot
            // i.e. the point used to draw a link
            //
            Binding {
                target: rootItem.linkOutput

                property: "position"

                value: Qt.point(rootItem.x + outputSlotItem.x + linkPointOut.x + linkPointOut.width/2,
                                rootItem.y + outputSlotItem.y + linkPointOut.y + linkPointOut.height/2)
            }
        }


        //------------------------------------------
        //
        // Header
        //
        //------------------------------------------

        // Button to open the definition
        /*Button {
            id: btnOpenDefinition

            anchors {
                top: parent.top
                topMargin: 10
                left: parent.left
                leftMargin: 10
            }

            style: LabellessSvgButtonStyle {
                fileCache: IngeScapeEditorTheme.svgFileIngeScapeEditor

                pressedID: releasedID + "-pressed"
                releasedID: "definition-button"
                disabledID : releasedID
            }

            opacity: rootItem.actionItemIsHovered ? 1 : 0

            visible: (opacity !== 0)
            enabled: visible

            Behavior on opacity {
                NumberAnimation {}
            }

            onClicked: {
                // Open the definition(s)
                rootItem.agentsGroupedByName.openDefinition();
            }
        }*/


        // Action Name
        Text {
            id: txtActionName

            anchors {
                left: parent.left
                leftMargin: 15
                right: parent.right
                rightMargin: 15
                verticalCenter: parent.verticalCenter
            }

            text: rootItem.actionInMappingVM ? rootItem.actionInMappingVM.name : "";
            //wrapMode: Text.WordWrap
            wrapMode: Text.Wrap
            maximumLineCount: 2
            elide: Text.ElideRight
            font: IngeScapeTheme.headingFont
            color: IngeScapeEditorTheme.agentsONNameMappingColor
        }


        // Remove button
        Button {
            id: btnRemoveFromMapping

            anchors {
                top: parent.top
                topMargin: 10
                horizontalCenter: parent.horizontalCenter
            }

            opacity: rootItem.actionItemIsHovered ? 1 : 0

            visible: (opacity !== 0)
            enabled: visible

            Behavior on opacity {
                NumberAnimation {}
            }

            activeFocusOnPress: true

            style: LabellessSvgButtonStyle {
                fileCache: IngeScapeTheme.svgFileIngeScape

                pressedID: releasedID + "-pressed"
                releasedID: "delete"
                disabledID : releasedID
            }

            onClicked: {
                if (controller)
                {
                    console.log("QML: FIXME TODO deleteActionInMapping...");

                    // Delete our action
                    //controller.deleteActionInMapping(rootItem.actionInMappingVM);
                }
            }
        }


        //-------------------
        //
        // Warnings
        //
        //-------------------

        // Drop Impossible
        /*I2SvgItem {
            anchors {
                right: parent.right
                top: parent.top
                margins: 2
            }

            svgFileCache: IngeScapeTheme.svgFileIngeScape
            svgElementId: "dropImpossible"

            visible: (rootItem.dropEnabled === false)
        }*/
    }
}
