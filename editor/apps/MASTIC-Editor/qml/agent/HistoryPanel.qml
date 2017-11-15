/*
 *	MASTIC Editor
 *
 *  Copyright © 2017 Ingenuity i/o. All rights reserved.
 *
 *	See license terms for the rights and conditions
 *	defined by copyright holders.
 *
 *
 *	Contributors:
 *      Vincent Peyruqueou <peyruqueou@ingenuity.io>
 *      Justine Limoges <limoges@ingenuity.io>
 */

import QtQuick 2.8
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

import I2Quick 1.0

import MASTIC 1.0

import "../theme" as Theme;


I2PopupBase {
    id: rootItem

    width: 600
    height: 545

    isModal: false
    dismissOnOutsideTap : false;
    keepRelativePositionToInitialParent : false;


    //--------------------------------
    //
    // Properties
    //
    //--------------------------------

    property var controller : MasticEditorC.valuesHistoryC

    // Columns with a fixed size
    // - Agent
    property int widthColumnAgent: 125
    // - Type
    property int widthColumnType: 110
    // - Definition Value
    property int widthColumnValue: 140
    // - Mapping Value
    property int widthColumnDate: 100

    // Resizable columns
    // - Name
    property int widthColumnName: (tableHistory.width - widthColumnType - widthColumnAgent - widthColumnValue - widthColumnDate)

    // List of widths
    property var widthsOfColumns: [
        widthColumnName,
        widthColumnAgent,
        widthColumnType,
        widthColumnValue,
        widthColumnDate
    ]

    //--------------------------------
    //
    // Signals
    //
    //--------------------------------

    // Emitted when user pressed our popup
    signal bringToFront();


    // Emitted when "All Agents" is selected or unselected
    signal clickAllAgents();

    //--------------------------------
    //
    // Content
    //
    //--------------------------------

    Rectangle {

        anchors {
            fill: parent
        }
        radius: 5
        border {
            width: 2
            color: MasticTheme.editorsBackgroundBorderColor
        }
        color: MasticTheme.editorsBackgroundColor


        MouseArea {
            id : dragMouseArea
            hoverEnabled: true
            anchors.fill: parent
            drag.target: rootItem

            /*drag.minimumX : - faisceauEditor.width/2
            drag.maximumX : PGIMTheme.applicationWidth - faisceauEditor.width/2
            drag.minimumY : 0
            drag.maximumY : PGIMTheme.applicationHeight -  (dragButton.height + 30)*/

            onPressed: {
                // Emit signal "bring to front"
                rootItem.bringToFront();
            }
        }

        Button {
            id: btnClose

            anchors {
                verticalCenter: titleItem.verticalCenter
                right : parent.right
                rightMargin: 20
            }

            activeFocusOnPress: true
            style: Theme.LabellessSvgButtonStyle {
                fileCache: MasticTheme.svgFileMASTIC

                pressedID: releasedID + "-pressed"
                releasedID: "closeEditor"
                disabledID : releasedID
            }

            onClicked: {
                // Close our popup
                rootItem.close();
            }
        }


        // Title
        Item {
            id : titleItem

            anchors {
                top : parent.top
                topMargin: 15
                left : parent.left
                leftMargin: 18
                right : parent.right
                rightMargin: 18
            }

            height : titleTxt.height


            Text {
                id : titleTxt

                anchors {
                    left : parent.left
                }

                text : "Outputs history"
                elide: Text.ElideRight
                color: MasticTheme.definitionEditorsLabelColor
                font {
                    family: MasticTheme.textFontFamily
                    pixelSize: 23
                    weight: Font.Medium
                }
            }
        }


        //Agents Filter
        Item {
            id : _combobox

            anchors {
                right : parent.right
                rightMargin: 20
                top : titleItem.bottom
                topMargin: 20
            }

            height : 25
            width : 197
            enabled: rootItem.controller && rootItem.controller.allAgentNamesList.length > 0

            onVisibleChanged: {
                if (!visible) {
                    close();
                }
            }

            /***
            * open function : open the combobox
            ***/
            function open() {
                _comboButton.checked = true;
                popup.open();
            }

            /***
            * close function : close the combobox
            ***/
            function close() {
                _comboButton.checked = false;
                _combobox.forceActiveFocus();
                popup.close();
            }

            Rectangle {
                id: _comboButton
                property bool checked : false;
                width:  parent.width;
                height: parent.height;
                radius: 1;

                border.width: _mouseAreaCombo.containsPress ? 1 : 0;
                border.color: MasticTheme.darkBlueGreyColor
                color : _mouseAreaCombo.containsPress? MasticTheme.darkGreyColor2 : MasticTheme.darkBlueGreyColor

                Text {
                    id:_comboPlaceholder

                    visible: (_comboText.text === "");
                    text : _combobox.enabled? "- Select an agent -" : "- No agent -"
                    anchors {
                        verticalCenter: parent.verticalCenter;
                        left: parent.left;
                        leftMargin: 10
                        right: _imageCombo.left;
                        rightMargin: 10
                    }


                    font {
                        pixelSize: 15
                        family: MasticTheme.textFontFamily;
                        italic : true;
                    }

                    color : MasticTheme.greyColor
                    verticalAlignment: Text.AlignVCenter;
                    elide : Text.ElideRight;
                }


                Text {
                    id:_comboText

                    anchors {
                        verticalCenter: parent.verticalCenter;
                        left: parent.left;
                        leftMargin: 10
                        right: _imageCombo.left;
                        rightMargin: 10
                    }

                    font {
                        pixelSize: 15
                        family: MasticTheme.textFontFamily;
                    }

                    color : MasticTheme.lightGreyColor
                    verticalAlignment: Text.AlignVCenter;
                    elide : Text.ElideRight;
                    text : (_combobox.enabled && rootItem.controller && rootItem.controller.selectedAgentNamesList.length > 0)?
                               (rootItem.controller.selectedAgentNamesList.length < rootItem.controller.allAgentNamesList.length)?
                                   (rootItem.controller.selectedAgentNamesList.length === 1 ? "- " + rootItem.controller.selectedAgentNamesList.length + " agent selected -" : "- " + rootItem.controller.selectedAgentNamesList.length + " agents selected -")
                                 : "- All agents selected -"
                    : "";
                }


                Image {
                    id:_imageCombo;
                    anchors.verticalCenter: parent.verticalCenter;
                    anchors.right: parent.right;
                    anchors.rightMargin: 10
                    rotation : (_comboButton.checked ? 180 : 0);
                    source : "image://I2svg/resources/SVG/mastic-pictos.svg#iconCombo";

                    Behavior on rotation {
                        NumberAnimation {}
                    }
                }


                MouseArea {
                    id:_mouseAreaCombo;
                    anchors.fill: parent;
                    activeFocusOnTab: true;
                    onClicked: {
                        _mouseAreaCombo.forceActiveFocus();
                        (_comboButton.checked) ? _combobox.close() : _combobox.open();
                    }
                }

                onVisibleChanged: {
                    if(!visible)
                        _combobox.close();
                }
            }

            I2PopupBase {
                id : popup
                anchors.top:_comboButton.bottom;

                width: _comboButton.width;
                height: ((_combolist.count < 8) ? (_combolist.count+1)*(_comboButton.height+1) : 9*(_comboButton.height+1) );


                isModal: true;
                layerColor: "transparent"
                layerObjectName: "overlayLayerComboBox";
                dismissOnOutsideTap : true;

                keepRelativePositionToInitialParent : true;

                onClosed: {
                    _combobox.close();
                }

                onOpened: {

                }

                Rectangle {
                    id : popUpBackground
                    anchors.fill : parent
                    color:  MasticTheme.darkBlueGreyColor
                }

                ScrollView {
                    id : _scrollView

                    visible: _comboButton.checked;

                    anchors {
                        top:  parent.top;
                        bottom:  parent.bottom;
                    }

                    width: _comboButton.width;
                    height: ((_combolist.count < 8) ? (_combolist.count+1)*(_comboButton.height+1) : 9*(_comboButton.height+1) );

                    // Prevent drag overshoot on Windows
                    flickableItem.boundsBehavior: Flickable.OvershootBounds

                    style: MasticScrollViewStyle {
                    }

                    contentItem: Item {
                        width: _scrollView.width;
                        height: ( (_combolist.count<8) ? (_combolist.count+1)*(_comboButton.height+1) : 9*(_comboButton.height+1) );

                        CheckBox {
                            id : filterAllAgentCB
                            anchors {
                                left: parent.left
                                leftMargin :10
                                right : parent.right
                                rightMargin : 10
                                top : parent.top
                                topMargin: 4
                            }

                            property bool isPartiallyChecked : false

                            checked : false;
                            partiallyCheckedEnabled : false;
                            activeFocusOnPress: true;

                            style: CheckBoxStyle {
                                label:  Text {
                                    anchors {
                                        verticalCenter: parent.verticalCenter
                                        verticalCenterOffset: 1
                                    }

                                    color: MasticTheme.lightGreyColor

                                    text: " All agents"
                                    elide: Text.ElideRight

                                    font {
                                        family: MasticTheme.textFontFamily
                                        pixelSize: 16
                                    }

                                }

                                indicator: Rectangle {
                                    implicitWidth: 14
                                    implicitHeight: 14
                                    border.width: 0;
                                    color : MasticTheme.veryDarkGreyColor

                                    I2SvgItem {
                                        visible : control.checkedState === Qt.Checked
                                        anchors.centerIn: parent

                                        svgFileCache : MasticTheme.svgFileMASTIC;
                                        svgElementId:  "check";

                                    }

                                    Text {
                                        visible : filterAllAgentCB.isPartiallyChecked
                                        anchors {
                                            centerIn: parent
                                        }

                                        color: MasticTheme.lightGreyColor

                                        text: "-"
                                        elide: Text.ElideRight

                                        font {
                                            family: MasticTheme.textFontFamily
                                            pixelSize: 16
                                        }
                                    }
                                }

                            }

                            onClicked : {
                                // reset isPartiallyChecked property
                                filterAllAgentCB.isPartiallyChecked =  false;

                                // show / hide agents values
                                if (rootItem.controller) {
                                    if (filterAllAgentCB.checked) {
                                        rootItem.controller.showValuesOfAllAgents()
                                    } else {
                                        rootItem.controller.hideValuesOfAllAgents()
                                    }
                                }

                                // signal the change to all agents checkboxes
                                rootItem.clickAllAgents();
                            }

                            Connections {
                                target : popup
                                onOpened : {
                                    // update "all agents" checkbox state
                                    // reset isPartiallyChecked and checkedState properties
                                    filterAllAgentCB.isPartiallyChecked =  false;
                                    filterAllAgentCB.checkedState =  Qt.Unchecked;
                                    if (rootItem.controller && rootItem.controller.selectedAgentNamesList.length>0) {
                                        (rootItem.controller.selectedAgentNamesList.length === rootItem.controller.allAgentNamesList.length)?
                                                    filterAllAgentCB.checkedState =  Qt.Checked
                                                  :  filterAllAgentCB.isPartiallyChecked =  true;
                                    }
                                }
                            }
                        }

                        ListView {
                            id:_combolist

                            boundsBehavior: Flickable.StopAtBounds

                            anchors {
                                top : filterAllAgentCB.bottom
                                topMargin: 4
                            }

                            width: parent.width;
                            height: ( (_combolist.count<8) ? _combolist.count*(_comboButton.height+1) : 8*(_comboButton.height+1) );

                            visible: parent.visible;

                            model: rootItem.controller ? rootItem.controller.allAgentNamesList : 0;

                            delegate: Item {
                                anchors {
                                    left: parent.left
                                    right: parent.right
                                }

                                width:  _comboButton.width
                                height: _comboButton.height

                                CheckBox {
                                    id : filterAgentCB
                                    anchors {
                                        verticalCenter: parent.verticalCenter
                                        left: parent.left
                                        leftMargin :10
                                        right : parent.right
                                        rightMargin : 10
                                    }

                                    checked : false;
                                    activeFocusOnPress: true;

                                    style: CheckBoxStyle {
                                        label:  Text {
                                            anchors {
                                                verticalCenter: parent.verticalCenter
                                                verticalCenterOffset: 1
                                            }

                                            color: MasticTheme.lightGreyColor

                                            text: " " + modelData
                                            elide: Text.ElideRight

                                            font {
                                                family: MasticTheme.textFontFamily
                                                pixelSize: 16
                                            }

                                        }

                                        indicator: Rectangle {
                                            implicitWidth: 14
                                            implicitHeight: 14
                                            border.width: 0;
                                            color : MasticTheme.veryDarkGreyColor

                                            I2SvgItem {
                                                visible : control.checkedState === Qt.Checked
                                                anchors.centerIn: parent

                                                svgFileCache : MasticTheme.svgFileMASTIC;
                                                svgElementId:  "check";

                                            }
                                        }

                                    }

                                    onClicked : {
                                        if (rootItem.controller) {
                                            if (filterAgentCB.checked) {
                                                rootItem.controller.showValuesOfAgent(modelData)
                                            } else {
                                                rootItem.controller.hideValuesOfAgent(modelData)
                                            }

                                            // update "all agents" checkbox state
                                            // reset isPartiallyChecked and checkedState properties
                                            filterAllAgentCB.isPartiallyChecked =  false;
                                            filterAllAgentCB.checkedState =  Qt.Unchecked;
                                            if (rootItem.controller && rootItem.controller.selectedAgentNamesList.length>0) {
                                                (rootItem.controller.selectedAgentNamesList.length === rootItem.controller.allAgentNamesList.length)?
                                                            filterAllAgentCB.checkedState =  Qt.Checked
                                                          :  filterAllAgentCB.isPartiallyChecked =  true;
                                            }
                                        }
                                    }

                                    Connections {
                                        target : popup
                                        onOpened : {
                                            // update agents checkboxes states when the pop up is opening
                                            if (controller) {
                                                filterAgentCB.checked = controller.areShownValuesOfAgent(modelData);
                                            }
                                        }
                                    }

                                    Connections {
                                        target : rootItem
                                        onClickAllAgents : {
                                            // update agents checkboxes states when the "pop up is opening   "All Agents" check box is selected or unselected
                                            if (controller) {
                                                filterAgentCB.checked = controller.areShownValuesOfAgent(modelData);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // History List
        Item {
            id : tableHistory

            anchors {
                top: parent.top
                topMargin: 90
                left: parent.left
                leftMargin: 20
                right: parent.right
                rightMargin: 20
                bottom: parent.bottom
                bottomMargin: 55
            }

            /// ****** Headers of columns ***** ////
            Row {
                id: tableauHeaderRow

                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.top
                    topMargin: 15
                }
                height : 33

                Repeater {
                    model: [
                        qsTr("Name"),
                        qsTr("Agent"),
                        qsTr("Type"),
                        qsTr("Value"),
                        qsTr("Date")
                    ]

                    Item {
                        height : 33
                        width : rootItem.widthsOfColumns[index]

                        Text {
                            anchors {
                                left: parent.left
                                leftMargin: 2
                                verticalCenter: parent.verticalCenter
                            }

                            text : modelData

                            color : MasticTheme.definitionEditorsAgentDescriptionColor
                            font {
                                family: MasticTheme.textFontFamily
                                pixelSize : 16
                            }
                        }
                    }
                }
            }


            // separator
            Rectangle {
                anchors {
                    left : parent.left
                    right : parent.right
                    top : tableauHeaderRow.bottom
                }
                height : 1

                color : MasticTheme.blackColor
            }

            /// ****** List ***** ////
            ScrollView {
                id : scrollView

                anchors {
                    top: tableauHeaderRow.bottom
                    left : parent.left
                    right : parent.right
                    bottom : parent.bottom
                }

                style: MasticScrollViewStyle {
                }

                // Prevent drag overshoot on Windows
                flickableItem.boundsBehavior: Flickable.OvershootBounds

                // Content of our scrollview
                ListView {
                    model: if (controller) {
                               controller.filteredValues
                           }
                           else {
                               0
                           }

                    delegate: Item {
                        anchors {
                            left : parent.left
                            right : parent.right
                        }
                        height : 30

                        Row {
                            id: listLine

                            anchors {
                                fill : parent
                                leftMargin: 2
                            }

                            // IOP Name
                            Text {
                                text: model.iopName

                                anchors {
                                    verticalCenter: parent.verticalCenter
                                }
                                verticalAlignment: Text.AlignVCenter
                                width : rootItem.widthsOfColumns[0]
                                elide: Text.ElideRight
                                height: parent.height
                                color: MasticTheme.whiteColor
                                font {
                                    family: MasticTheme.textFontFamily
                                    pixelSize : 16
                                }
                            }

                            // Agent Name
                            Text {
                                text: model.agentName

                                anchors {
                                    verticalCenter: parent.verticalCenter
                                }
                                verticalAlignment: Text.AlignVCenter
                                width : rootItem.widthsOfColumns[1]
                                elide: Text.ElideRight
                                height: parent.height
                                color: MasticTheme.lightGreyColor
                                font {
                                    family: MasticTheme.textFontFamily
                                    pixelSize : 15
                                }
                            }


                            // Type
                            Item {
                                anchors {
                                    verticalCenter: parent.verticalCenter
                                }
                                height: parent.height
                                width : rootItem.widthsOfColumns[2]

                                Rectangle {
                                    id : circle
                                    anchors {
                                        left : parent.left
                                        verticalCenter: parent.verticalCenter
                                    }

                                    width : 16
                                    height : width
                                    radius : width/2

                                    color : switch (model.iopValueTypeGroup)
                                            {
                                            case AgentIOPValueTypeGroups.NUMBER:
                                                MasticTheme.orangeColor2
                                                break;
                                            case AgentIOPValueTypeGroups.STRING:
                                                MasticTheme.redColor2
                                                break;
                                            case AgentIOPValueTypeGroups.IMPULSION:
                                                MasticTheme.purpleColor
                                                break;
                                            case AgentIOPValueTypeGroups.DATA:
                                                MasticTheme.greenColor
                                                break;
                                            case AgentIOPValueTypeGroups.MIXED:
                                                MasticTheme.whiteColor
                                                break;
                                            case AgentIOPValueTypeGroups.UNKNOWN:
                                                "#000000"
                                                break;
                                            default:
                                                MasticTheme.whiteColor;
                                                break;
                                            }
                                }

                                Text {
                                    text: AgentIOPValueTypes.enumToString(model.iopValueType)

                                    anchors {
                                        verticalCenter: circle.verticalCenter
                                        verticalCenterOffset: 1
                                        left : circle.right
                                        leftMargin: 5
                                        right : parent.right
                                    }
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                    height: parent.height
                                    color: MasticTheme.whiteColor
                                    font {
                                        family: MasticTheme.textFontFamily
                                        pixelSize : 16
                                    }
                                }
                            }


                            // Value
                            Text {
                                text: model.displayableValue

                                anchors {
                                    verticalCenter: parent.verticalCenter
                                }
                                verticalAlignment: Text.AlignVCenter
                                width : rootItem.widthsOfColumns[3]
                                height: parent.height
                                elide: Text.ElideRight
                                color: MasticTheme.whiteColor
                                font {
                                    family: MasticTheme.textFontFamily
                                    pixelSize : 16
                                }
                            }


                            // Date
                            Text {
                                text: model.time.toLocaleString(Qt.locale(),"hh:mm:ss.zzz")

                                anchors {
                                    verticalCenter: parent.verticalCenter
                                }
                                verticalAlignment: Text.AlignVCenter
                                width : rootItem.widthsOfColumns[4]
                                height: parent.height
                                color: MasticTheme.whiteColor
                                elide: Text.ElideRight

                                font {
                                    family: MasticTheme.textFontFamily
                                    pixelSize : 16
                                }
                            }

                        }

                        //separator
                        Rectangle {
                            anchors {
                                left : parent.left
                                right : parent.right
                                bottom : parent.bottom
                            }
                            height : 1

                            color : MasticTheme.blackColor
                        }

                    }
                }
            }
        }




        Button {
            id: okButton

            property var boundingBox: MasticTheme.svgFileMASTIC.boundsOnElement("button");
            height : boundingBox.height
            width :  boundingBox.width

            enabled : visible
            activeFocusOnPress: true
            text : "OK"

            anchors {
                right : parent.right
                rightMargin: 16
                bottom : parent.bottom
                bottomMargin: 16
            }

            style: I2SvgButtonStyle {
                fileCache: MasticTheme.svgFileMASTIC

                pressedID: releasedID + "-pressed"
                releasedID: "button"
                disabledID : releasedID

                font {
                    family: MasticTheme.textFontFamily
                    weight : Font.Medium
                    pixelSize : 16
                }
                labelColorPressed: MasticTheme.blackColor
                labelColorReleased: MasticTheme.whiteColor
                labelColorDisabled: MasticTheme.whiteColor
            }

            onClicked: {
                // Close our popup
                rootItem.close();
            }
        }

    }
}

