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
 *
 */

#include "valueshistorycontroller.h"

/**
 * @brief Constructor
 * @param modelManager
 * @param parent
 */
ValuesHistoryController::ValuesHistoryController(MasticModelManager* modelManager,
                                                 QObject *parent) : QObject(parent),
    _modelManager(modelManager)
{
    // Force ownership of our object, it will prevent Qml from stealing it
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

    if (_modelManager != NULL)
    {
        //
        // Link our list to the list of the model manager
        //
        _filteredValues.setSourceModel(_modelManager->publishedValues());

        // Fill the list with all enum values
        _allAgentIOPTypes.fillWithAllEnumValues();

        // By default: all types are selected
        _selectedAgentIOPTypes.fillWithAllEnumValues();
    }
}


/**
 * @brief Destructor
 */
ValuesHistoryController::~ValuesHistoryController()
{
    _selectedAgentIOPTypes.deleteAllItems();
    _allAgentIOPTypes.deleteAllItems();

    _modelManager = NULL;
}


/**
 * @brief Show the values of agent Input/Output/Parameter type
 * @param agentIOPType
 */
void ValuesHistoryController::showValuesOfAgentIOPType(AgentIOPTypes::Value agentIOPType)
{
    _selectedAgentIOPTypes.appendEnumValue(agentIOPType);

    // Update the filters on the list of values
    _updateFilters();
}


/**
 * @brief Hide the values of agent Input/Output/Parameter type
 * @param agentIOPType
 */
void ValuesHistoryController::hideValuesOfAgentIOPType(AgentIOPTypes::Value agentIOPType)
{
    _selectedAgentIOPTypes.removeEnumValue(agentIOPType);

    // Update the filters on the list of values
    _updateFilters();
}


/**
 * @brief Show the values of agent with name
 * @param agentName
 */
void ValuesHistoryController::showValuesOfAgent(QString agentName)
{
    QStringList temp = _selectedAgentNamesList;
    temp.append(agentName);
    temp.sort(Qt::CaseInsensitive);

    // Use the setter to emit a signal for QML binding
    setselectedAgentNamesList(temp);

    // Update the filters on the list of values
    _updateFilters();
}


/**
 * @brief Hide the values of agent with name
 * @param agentName
 */
void ValuesHistoryController::hideValuesOfAgent(QString agentName)
{
    QStringList temp = _selectedAgentNamesList;
    temp.removeOne(agentName);
    // No need to sort (list is already sorted)

    // Use the setter to emit a signal for QML binding
    setselectedAgentNamesList(temp);

    // Update the filters on the list of values
    _updateFilters();
}


/**
 * @brief Show the values of all agents
 */
void ValuesHistoryController::showValuesOfAllAgents()
{
    // Use the setter to emit a signal for QML binding
    setselectedAgentNamesList(_allAgentNamesList);

    // Update the filters on the list of values
    _updateFilters();
}


/**
 * @brief Hide the values of all agents
 */
void ValuesHistoryController::hideValuesOfAllAgents()
{
    // Use the setter to emit a signal for QML binding
    setselectedAgentNamesList(QStringList());

    // Update the filters on the list of values
    _updateFilters();
}


/**
 * @brief Return true if the values of the agent are shown
 * @param agentName
 * @return
 */
bool ValuesHistoryController::areShownValuesOfAgent(QString agentName)
{
    return _selectedAgentNamesList.contains(agentName);
}


/**
 * @brief Slot called when a new "Agent in Mapping" is added
 * @param agentName
 */
void ValuesHistoryController::onAgentInMappingAdded(QString agentName)
{
    QStringList temp1 = _allAgentNamesList;
    temp1.append(agentName);
    temp1.sort(Qt::CaseInsensitive);

    // Use the setter to emit a signal for QML binding
    setallAgentNamesList(temp1);

    // By default: the agent name is selected
    QStringList temp2 = _selectedAgentNamesList;
    temp2.append(agentName);
    temp2.sort(Qt::CaseInsensitive);

    // Use the setter to emit a signal for QML binding
    setselectedAgentNamesList(temp2);

    // Update the filters on the list of values
    _updateFilters();
}


/**
 * @brief Slot called when an "Agent in Mapping" is removed
 * @param agentName
 */
void ValuesHistoryController::onAgentInMappingRemoved(QString agentName)
{
    QStringList temp1 = _allAgentNamesList;
    temp1.removeOne(agentName);
    // No need to sort (list is already sorted)

    // Use the setter to emit a signal for QML binding
    setallAgentNamesList(temp1);

    if (_selectedAgentNamesList.contains(agentName))
    {
        QStringList temp2 = _selectedAgentNamesList;
        temp2.removeOne(agentName);
        // No need to sort (list is already sorted)

        // Use the setter to emit a signal for QML binding
        setselectedAgentNamesList(temp2);

        // Update the filters on the list of values
        _updateFilters();
    }
}


/**
 * @brief Slot called when we have to filter values to show only those of the agent (with the name)
 * @param agentName
 */
void ValuesHistoryController::filterValuesToShowOnlyAgent(QString agentName)
{
    // Empty list and append only the agent name
    QStringList temp;
    temp.append(agentName);
    temp.sort(Qt::CaseInsensitive);

    // Use the setter to emit a signal for QML binding
    setselectedAgentNamesList(temp);

    qDebug() << "Filter Values to Show Only Agent" << agentName;

    // Update the filters on the list of values
    _updateFilters();
}


/**
 * @brief Update the filters on the list of values
 */
void ValuesHistoryController::_updateFilters()
{
    /*qDebug() << "Display Values for type:";
    foreach (int iterator, _selectedAgentIOPTypes.toEnumValuesList()) {
        AgentIOPTypes::Value agentIOPType = static_cast<AgentIOPTypes::Value>(iterator);
        qDebug() << AgentIOPTypes::staticEnumToString(agentIOPType);
    }*/

    qDebug() << "All agents" << _allAgentNamesList << "-- selected agents" << _selectedAgentNamesList;

    // Update the list of agent names of the filter
    _filteredValues.setselectedAgentNamesList(_selectedAgentNamesList);

    // Update the filter
    _filteredValues.updateFilter();
}
