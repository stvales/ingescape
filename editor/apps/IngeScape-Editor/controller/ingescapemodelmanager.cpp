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
 *      Vincent Peyruqueou <peyruqueou@ingenuity.io>
 *      Alexandre Lemort   <lemort@ingenuity.io>
 *
 */

#include "ingescapemodelmanager.h"

#include <QQmlEngine>
#include <QDebug>
#include <QFileDialog>

#include <I2Quick.h>

#include "controller/ingescapelaunchermanager.h"


/**
 * @brief Constructor
 * @param jsonHelper
 * @param rootDirectoryPath
 * @param parent
 */
IngeScapeModelManager::IngeScapeModelManager(JsonHelper* jsonHelper,
                                             QString rootDirectoryPath,
                                             QObject *parent) : QObject(parent),
    _isMappingActivated(false),
    _isMappingControlled(false),
    _jsonHelper(jsonHelper),
    _rootDirectoryPath(rootDirectoryPath)
{
    // Force ownership of our object, it will prevent Qml from stealing it
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

    qInfo() << "New INGESCAPE Model Manager";

    // Agents grouped are sorted on their name (alphabetical order)
    _allAgentsGroupedByName.setSortProperty("name");
}


/**
 * @brief Destructor
 */
IngeScapeModelManager::~IngeScapeModelManager()
{
    qInfo() << "Delete INGESCAPE Model Manager";

    // Clear all opened definitions
    _openedDefinitions.clear();

    // Free memory
    _publishedValues.deleteAllItems();

    // Free memory
    _hashFromNameToAgentsGrouped.clear();
    _allAgentsGroupedByName.deleteAllItems();

    // Reset pointers
    _jsonHelper = NULL;
}


/**
 * @brief Setter for property "is Activated Mapping"
 * @param value
 */
void IngeScapeModelManager::setisMappingActivated(bool value)
{
    if (_isMappingActivated != value)
    {
        _isMappingActivated = value;

        if (_isMappingActivated) {
            qInfo() << "Mapping Activated";
        }
        else {
            qInfo() << "Mapping DE-activated";
        }

        Q_EMIT isMappingActivatedChanged(value);
    }
}


/**
 * @brief Setter for property "is Controlled Mapping"
 * @param value
 */
void IngeScapeModelManager::setisMappingControlled(bool value)
{
    if (_isMappingControlled != value)
    {
        _isMappingControlled = value;

        if (_isMappingControlled) {
            qInfo() << "Mapping Controlled";
        }
        else {
            qInfo() << "Mapping Observed";
        }

        Q_EMIT isMappingControlledChanged(value);
    }
}


/**
 * @brief Add a model of agent
 * @param agent
 */
void IngeScapeModelManager::addAgentModel(AgentM* agent)
{
    if (agent != nullptr)
    {
        // Connect to signals from this new agent
        connect(agent, &AgentM::networkDataWillBeCleared, this, &IngeScapeModelManager::_onNetworkDataOfAgentWillBeCleared);

        if (!agent->peerId().isEmpty()) {
            _mapFromPeerIdToAgentM.insert(agent->peerId(), agent);
        }

        // Emit the signal "Agent Model Created"
        Q_EMIT agentModelCreated(agent);

        _printAgents();
    }
}


/**
 * @brief Delete a model of agent
 * @param agent
 */
void IngeScapeModelManager::deleteAgentModel(AgentM* agent)
{
    if (agent != nullptr)
    {
        // Emit the signal "Agent Model Will Be Deleted"
        Q_EMIT agentModelWillBeDeleted(agent);

        // DIS-connect to signals from the agent
        disconnect(agent, 0, this, 0);

        // Delete its model of definition if needed
        if (agent->definition() != nullptr)
        {
            DefinitionM* temp = agent->definition();
            agent->setdefinition(nullptr);
            delete temp;
        }

        // Delete its model of mapping if needed
        if (agent->mapping() != nullptr)
        {
            AgentMappingM* temp = agent->mapping();
            agent->setmapping(nullptr);
            delete temp;
        }

        if (!agent->peerId().isEmpty()) {
            _mapFromPeerIdToAgentM.remove(agent->peerId());
        }

        // FIXME: do not use AgentsGroupedByNameVM inside a basic function about AgentM
        // --> Only AgentsGroupedByNameVM could call this function
        AgentsGroupedByNameVM* agentsGroupedByName = getAgentsGroupedForName(agent->name());

        // Remove the model from the view model of agents grouped by name
        if ((agentsGroupedByName != nullptr) && agentsGroupedByName->models()->contains(agent)) {
            agentsGroupedByName->models()->remove(agent);
        }

        // Free memory...later
        // the call to "agent->setdefinition" will produce the call of the slot _onAgentDefinitionChangedWithPreviousAndNewValues
        // ...and in some cases, the call to deleteAgentModel on this agent. So we cannot call directly "delete agent;"
        //delete agent;
        //agent->deleteLater();

        // Free memory
        delete agent;

        // Delete the view model of agents grouped by name if it does not contain model anymore
        if ((agentsGroupedByName != nullptr) && agentsGroupedByName->models()->isEmpty()) {
            deleteAgentsGroupedByName(agentsGroupedByName);
        }

        _printAgents();
    }
}


/**
 * @brief Save a new view model of agents grouped by name
 * @param agentsGroupedByName
 */
void IngeScapeModelManager::_saveNewAgentsGroupedByName(AgentsGroupedByNameVM* agentsGroupedByName)
{
    if ((agentsGroupedByName != nullptr) && !agentsGroupedByName->name().isEmpty())
    {
        // Connect to its signals
        connect(agentsGroupedByName, &AgentsGroupedByNameVM::agentsGroupedByDefinitionHasBeenCreated, this, &IngeScapeModelManager::agentsGroupedByDefinitionHasBeenCreated);
        connect(agentsGroupedByName, &AgentsGroupedByNameVM::agentsGroupedByDefinitionWillBeDeleted, this, &IngeScapeModelManager::agentsGroupedByDefinitionWillBeDeleted);
        connect(agentsGroupedByName, &AgentsGroupedByNameVM::agentModelHasToBeDeleted, this, &IngeScapeModelManager::onAgentModelHasToBeDeleted);
        connect(agentsGroupedByName, &AgentsGroupedByNameVM::definitionsToOpen, this, &IngeScapeModelManager::onDefinitionsToOpen);

        // Add to the hash table
        _hashFromNameToAgentsGrouped.insert(agentsGroupedByName->name(), agentsGroupedByName);

        // Add to the sorted list
        _allAgentsGroupedByName.append(agentsGroupedByName);

        // Emit the signal "Agents grouped by name has been created"
        Q_EMIT agentsGroupedByNameHasBeenCreated(agentsGroupedByName);
    }
}


/**
 * @brief Delete a view model of agents grouped by name
 * @param agentsGroupedByName
 */
void IngeScapeModelManager::deleteAgentsGroupedByName(AgentsGroupedByNameVM* agentsGroupedByName)
{
    if ((agentsGroupedByName != nullptr) && !agentsGroupedByName->name().isEmpty())
    {
        // DIS-connect to its signals
        disconnect(agentsGroupedByName, 0, this, 0);

        // Remove from the hash table
        _hashFromNameToAgentsGrouped.remove(agentsGroupedByName->name());

        // Remove from the sorted list
        _allAgentsGroupedByName.remove(agentsGroupedByName);

        // Emit the signal "Agents grouped by name will be deleted"
        Q_EMIT agentsGroupedByNameWillBeDeleted(agentsGroupedByName);

        // Free memory
        delete agentsGroupedByName;
    }
}


/**
 * @brief Get the model of agent from a Peer Id
 * @param peerId
 * @return
 */
AgentM* IngeScapeModelManager::getAgentModelFromPeerId(QString peerId)
{
    if (_mapFromPeerIdToAgentM.contains(peerId)) {
        return _mapFromPeerIdToAgentM.value(peerId);
    }
    else {
        return NULL;
    }
}


/**
 * @brief Get the (view model of) agents grouped for a name
 * @param name
 * @return
 */
AgentsGroupedByNameVM* IngeScapeModelManager::getAgentsGroupedForName(QString name)
{
    if (_hashFromNameToAgentsGrouped.contains(name)) {
        return _hashFromNameToAgentsGrouped.value(name);
    }
    else {
        return NULL;
    }
}


/**
 * @brief Get the map from agent name to list of active agents
 * @return
 */
QHash<QString, QList<AgentM*>> IngeScapeModelManager::getMapFromAgentNameToActiveAgentsList()
{
    QHash<QString, QList<AgentM*>> hashFromAgentNameToActiveAgentsList;

    // Traverse the list of all agents grouped by name
    for (AgentsGroupedByNameVM* agentsGroupedByName : _allAgentsGroupedByName.toList())
    {
        if ((agentsGroupedByName != nullptr) && agentsGroupedByName->isON())
        {
            QList<AgentM*> activeAgentsList;
            for (AgentM* agent : agentsGroupedByName->models()->toList())
            {
                if ((agent != nullptr) && agent->isON()) {
                    activeAgentsList.append(agent);
                }
            }
            if (!activeAgentsList.isEmpty()) {
                hashFromAgentNameToActiveAgentsList.insert(agentsGroupedByName->name(), activeAgentsList);
            }
        }
    }

    return hashFromAgentNameToActiveAgentsList;
}


/**
 * @brief Import an agent or an agents list from selected file (definition)
 */
bool IngeScapeModelManager::importAgentOrAgentsListFromSelectedFile()
{
    bool success = true;

    if (_jsonHelper != nullptr)
    {
        // "File Dialog" to get the file (path) to open
        QString agentFilePath = QFileDialog::getOpenFileName(NULL,
                                                             "Open an agent(s) definition",
                                                             _rootDirectoryPath,
                                                             "JSON (*.json)");

        if (!agentFilePath.isEmpty())
        {
            QFile jsonFile(agentFilePath);
            if (jsonFile.open(QIODevice::ReadOnly))
            {
                QByteArray byteArrayOfJson = jsonFile.readAll();
                jsonFile.close();

                QJsonDocument jsonDocument = QJsonDocument::fromJson(byteArrayOfJson);

                QJsonObject jsonRoot = jsonDocument.object();

                // List of agents
                if (jsonRoot.contains("agents"))
                {
                    // Import the agents list from a json byte content
                    success = importAgentsListFromJson(jsonRoot.value("agents").toArray());
                }
                // One agent
                else if (jsonRoot.contains("definition"))
                {
                    QJsonValue jsonValue = jsonRoot.value("definition");
                    if (jsonValue.isObject())
                    {
                        // Create a model of agent definition from the JSON
                        DefinitionM* agentDefinition = _jsonHelper->createModelOfAgentDefinitionFromJSON(jsonValue.toObject());
                        if (agentDefinition != nullptr)
                        {
                            QString agentName = agentDefinition->name();

                            // Create a new model of agent with the name of the definition
                            AgentM* agent = new AgentM(agentName, this);

                            // Add this new model of agent
                            addAgentModel(agent);

                            // Set its definition
                            agent->setdefinition(agentDefinition);
                        }
                        // An error occured, the definition is NULL
                        else {
                            qWarning() << "The file" << agentFilePath << "does not contain an agent definition !";

                            success = false;
                        }
                    }
                }
                else {
                    qWarning() << "The file" << agentFilePath << "does not contain one or several agent definition(s) !";

                    success = false;
                }
            }
            else {
                qCritical() << "Can not open file" << agentFilePath;

                success = false;
            }
        }
    }
    return success;
}


/**
 * @brief Import an agents list from a JSON array
 * @param jsonArrayOfAgents
 */
bool IngeScapeModelManager::importAgentsListFromJson(QJsonArray jsonArrayOfAgents)
{
    bool success = true;

    if (_jsonHelper != nullptr)
    {
        for (QJsonValue jsonValue : jsonArrayOfAgents)
        {
            if (jsonValue.isObject())
            {
                QJsonObject jsonAgent = jsonValue.toObject();

                // Get values for keys "agentName", "definition" and "clones"
                QJsonValue jsonName = jsonAgent.value("agentName");
                QJsonValue jsonDefinition = jsonAgent.value("definition");
                QJsonValue jsonClones = jsonAgent.value("clones");

                if (jsonName.isString() && (jsonDefinition.isObject() || jsonDefinition.isNull()) && jsonClones.isArray())
                {
                    QString agentName = jsonName.toString();
                    QJsonArray arrayOfClones = jsonClones.toArray();

                    DefinitionM* agentDefinition = nullptr;

                    if (jsonDefinition.isObject())
                    {
                        // Create a model of agent definition from JSON object
                        agentDefinition = _jsonHelper->createModelOfAgentDefinitionFromJSON(jsonDefinition.toObject());
                    }

                    // None clones have a defined hostname (agent is only defined by a definition)
                    if (arrayOfClones.isEmpty())
                    {
                        qDebug() << "Clone of" << agentName << "without hostname and command line";

                        // Create a new model of agent with the name
                        AgentM* agent = new AgentM(agentName, this);

                        // Add this new model of agent
                        addAgentModel(agent);

                        if (agentDefinition != nullptr)
                        {
                            // Set its definition
                            agent->setdefinition(agentDefinition);
                        }
                    }
                    // There are some clones with a defined hostname
                    else
                    {
                        for (QJsonValue jsonValue : arrayOfClones)
                        {
                            if (jsonValue.isObject())
                            {
                                QJsonObject jsonClone = jsonValue.toObject();

                                QJsonValue jsonHostname = jsonClone.value("hostname");
                                QJsonValue jsonCommandLine = jsonClone.value("commandLine");
                                QJsonValue jsonPeerId = jsonClone.value("peerId");
                                QJsonValue jsonAddress = jsonClone.value("address");

                                if (jsonHostname.isString() && jsonCommandLine.isString() && jsonPeerId.isString() && jsonAddress.isString())
                                {
                                    QString hostname = jsonHostname.toString();
                                    QString commandLine = jsonCommandLine.toString();
                                    QString peerId = jsonPeerId.toString();
                                    QString ipAddress = jsonAddress.toString();

                                    if (!hostname.isEmpty() && !commandLine.isEmpty())
                                    {
                                        qDebug() << "Clone of" << agentName << "on" << hostname << "with command line" << commandLine << "(" << peerId << ")";

                                        // Create a new model of agent with the name
                                        AgentM* agent = new AgentM(agentName, peerId, ipAddress,  this);

                                        // Update the hostname and the command line
                                        agent->sethostname(hostname);
                                        agent->setcommandLine(commandLine);

                                        // Update corresponding host
                                        HostM* host = IngeScapeLauncherManager::Instance().getHostWithName(hostname);
                                        if (host != nullptr) {
                                            agent->setcanBeRestarted(true);
                                        }

                                        // Add this new model of agent
                                        addAgentModel(agent);

                                        if (agentDefinition != nullptr)
                                        {
                                            // Make a copy of the definition
                                            DefinitionM* copyOfDefinition = agentDefinition->copy();
                                            if (copyOfDefinition != nullptr)
                                            {
                                                // Set its definition
                                                agent->setdefinition(copyOfDefinition);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if (agentDefinition != nullptr)
                    {
                        // Free memory
                        delete agentDefinition;
                    }
                }
                else {
                    qWarning() << "The JSON object does not contain an agent name !";

                    success = false;
                }
            }
        }
    }
    return success;
}


/**
 * @brief Export the agents list to selected file
 * @param jsonArrayOfAgents
 */
void IngeScapeModelManager::exportAgentsListToSelectedFile(QJsonArray jsonArrayOfAgents)
{
    // "File Dialog" to get the file (path) to save
    QString agentsListFilePath = QFileDialog::getSaveFileName(NULL,
                                                              "Save agents",
                                                              _rootDirectoryPath,
                                                              "JSON (*.json)");

    if (!agentsListFilePath.isEmpty())
    {
        qInfo() << "Save the agents list to JSON file" << agentsListFilePath;

        QJsonObject jsonRoot = QJsonObject();
        jsonRoot.insert("agents", jsonArrayOfAgents);

        QByteArray byteArrayOfJson = QJsonDocument(jsonRoot).toJson(QJsonDocument::Indented);

        QFile jsonFile(agentsListFilePath);
        if (jsonFile.open(QIODevice::WriteOnly))
        {
            jsonFile.write(byteArrayOfJson);
            jsonFile.close();
        }
        else {
            qCritical() << "Can not open file" << agentsListFilePath;
        }
    }
}


/**
 * @brief Simulate an exit for each active agent
 * an active agent has its flag "isON" equal to true
 */
void IngeScapeModelManager::simulateExitForEachActiveAgent()
{
    for (AgentM* agent : _mapFromPeerIdToAgentM.values())
    {
        if (agent != nullptr)
        {
            if (agent->isON())
            {
                // Simulate an exit for each agent
                onAgentExited(agent->peerId(), agent->name());
            }

            // Force the flag indicating that the agent can NOT be restarted (by an INGESCAPE launcher)
            agent->setcanBeRestarted(false);
        }
    }
}


/**
 * @brief Open a definition
 * If there are variants of this definition, we open each variant
 * @param definition
 */
void IngeScapeModelManager::openDefinition(DefinitionM* definition)
{
    if (definition != nullptr)
    {
        QString definitionName = definition->name();

        qDebug() << "Open the definition" << definitionName;

        QList<DefinitionM*> definitionsToOpen;

        // Variant --> we have to open each variants of this definition
        if (definition->isVariant())
        {
            for (AgentsGroupedByNameVM* agentsGroupedByName : _allAgentsGroupedByName.toList())
            {
                if (agentsGroupedByName != nullptr)
                {
                    // Get the list of definitions with a specific name
                    QList<DefinitionM*> definitionsList = agentsGroupedByName->getDefinitionsWithName(definitionName);

                    if (!definitionsList.isEmpty())
                    {
                        for (DefinitionM* iterator : definitionsList)
                        {
                            // Same name, same version and variant, we have to open it
                            if ((iterator != nullptr) && (iterator->version() == definition->version()) && iterator->isVariant()) {
                                definitionsToOpen.append(iterator);
                            }
                        }
                    }
                }
            }
        }
        else {
            // Simply add the definition
            definitionsToOpen.append(definition);
        }

        // Open the list of definitions
        _openDefinitions(definitionsToOpen);
    }
}


/**
 * @brief Slot called when an agent enter the network
 * @param peerId
 * @param agentName
 * @param ipAddress
 * @param hostname
 * @param commandLine
 * @param canBeFrozen
 * @param loggerPort
 */
void IngeScapeModelManager::onAgentEntered(QString peerId, QString agentName, QString ipAddress, QString hostname, QString commandLine, bool canBeFrozen, QString loggerPort)
{
    if (!peerId.isEmpty() && !agentName.isEmpty() && !ipAddress.isEmpty())
    {
        AgentM* agent = getAgentModelFromPeerId(peerId);

        // An agent with this peer id already exist
        if (agent != nullptr)
        {
            qInfo() << "The agent" << agentName << "with peer id" << peerId << "on" << hostname << "(" << ipAddress << ") is back on the network !";

            // Useless !
            //agent->sethostname(hostname);
            //agent->setcommandLine(commandLine);

            // Usefull ?
            /*if (!hostname.isEmpty() && !commandLine.isEmpty())
            {
                HostM* host = IngeScapeLauncherManager::Instance().getHostWithName(hostname);
                if (host != nullptr)
                {
                    agent->setcanBeRestarted(true);
                }
            }*/

            agent->setcanBeFrozen(canBeFrozen);
            agent->setloggerPort(loggerPort);

            // Update the state (flag "is ON")
            agent->setisON(true);

            // When this agent exited, we set its flag to "OFF" and emited "removeInputsToEditorForOutputs"
            // Now, we just set its flag to ON and we have to emit "addInputsToEditorForOutputs"
            // Because we consider that its definition will be the same...consequently, when "onDefinitionReceived" will be called,
            // there will be no change detected and the signal "addInputsToEditorForOutputs" will not be called
            if ((agent->definition() != nullptr) && !agent->definition()->outputsList()->isEmpty())
            {
                Q_EMIT addInputsToEditorForOutputs(agentName, agent->definition()->outputsList()->toList());
            }
        }
        // New peer id
        else
        {
            // Create a new model of agent
            agent = new AgentM(agentName, peerId, ipAddress, this);

            agent->sethostname(hostname);
            agent->setcommandLine(commandLine);

            if (!hostname.isEmpty() && !commandLine.isEmpty())
            {
                HostM* host = IngeScapeLauncherManager::Instance().getHostWithName(hostname);
                if (host != nullptr)
                {
                    agent->setcanBeRestarted(true);
                }
            }

            agent->setcanBeFrozen(canBeFrozen);
            agent->setloggerPort(loggerPort);

            // Update the state (flag "is ON")
            agent->setisON(true);

            // Add this new model of agent
            addAgentModel(agent);


            // Get the (view model of) agents grouped for this name
            AgentsGroupedByNameVM* agentsGroupedByName = getAgentsGroupedForName(agentName);
            if (agentsGroupedByName == nullptr)
            {
                // Create a new view model of agents grouped by name
                agentsGroupedByName = new AgentsGroupedByNameVM(agentName, this);

                // Save this new view model of agents grouped by name
                _saveNewAgentsGroupedByName(agentsGroupedByName);

                // Manage the new model of agent
                agentsGroupedByName->manageNewModel(agent);
            }
            else
            {
                // Manage the new model of agent
                agentsGroupedByName->manageNewModel(agent);
            }
        }
    }
}


/**
 * @brief Slot called when an agent quit the network
 * @param peer Id
 * @param agent name
 */
void IngeScapeModelManager::onAgentExited(QString peerId, QString agentName)
{
    AgentM* agent = getAgentModelFromPeerId(peerId);
    if (agent != nullptr)
    {
        qInfo() << "The agent" << agentName << "with peer id" << peerId << "exited from the network !";

        // Update the state (flag "is ON")
        agent->setisON(false);

        if ((agent->definition() != nullptr) && !agent->definition()->outputsList()->isEmpty()) {
            Q_EMIT removeInputsToEditorForOutputs(agentName, agent->definition()->outputsList()->toList());
        }

        // Get the (view model of) agents grouped for this name
        /*AgentsGroupedByNameVM* agentsGroupedByName = getAgentsGroupedForName(agentName);
        if (agentsGroupedByName != nullptr)
        {
        }*/
    }
}


/**
 * @brief Slot called when a model of agent has to be deleted
 * @param model
 */
void IngeScapeModelManager::onAgentModelHasToBeDeleted(AgentM* model)
{
    if (model != nullptr) {
        deleteAgentModel(model);
    }
}


/**
 * @brief Slot called when the definition(s) of an agent (agents grouped by name) must be opened
 * @param definitionsList
 */
void IngeScapeModelManager::onDefinitionsToOpen(QList<DefinitionM*> definitionsList)
{
    _openDefinitions(definitionsList);
}


/**
 * @brief Slot called when a launcher enter the network
 * @param peerId
 * @param hostname
 * @param ipAddress
 */
void IngeScapeModelManager::onLauncherEntered(QString peerId, QString hostname, QString ipAddress, QString streamingPort)
{
    // Add an IngeScape Launcher to the manager
    IngeScapeLauncherManager::Instance().addIngeScapeLauncher(peerId, hostname, ipAddress, streamingPort);

    // Traverse the list of all agents grouped by name
    for (AgentsGroupedByNameVM* agentsGroupedByName : _allAgentsGroupedByName.toList())
    {
        if (agentsGroupedByName != nullptr)
        {
            // Traverse the list of all models
            for (AgentM* agent : agentsGroupedByName->models()->toList())
            {
                if ((agent != nullptr) && (agent->hostname() == hostname) && !agent->commandLine().isEmpty()) {
                    agent->setcanBeRestarted(true);
                }
            }
        }
    }
}


/**
 * @brief Slot called when a launcher quit the network
 * @param peerId
 * @param hostname
 */
void IngeScapeModelManager::onLauncherExited(QString peerId, QString hostname)
{
    // Remove an IngeScape Launcher to the manager
    IngeScapeLauncherManager::Instance().removeIngeScapeLauncher(peerId, hostname);

    // Traverse the list of all agents grouped by name
    for (AgentsGroupedByNameVM* agentsGroupedByName : _allAgentsGroupedByName.toList())
    {
        if (agentsGroupedByName != nullptr)
        {
            // Traverse the list of all models
            for (AgentM* agent : agentsGroupedByName->models()->toList())
            {
                if ((agent != nullptr) && (agent->hostname() == hostname)) {
                    agent->setcanBeRestarted(false);
                }
            }
        }
    }
}


/**
 * @brief Slot called when an agent definition has been received and must be processed
 * @param peer Id
 * @param agent name
 * @param definition in JSON format
 */
void IngeScapeModelManager::onDefinitionReceived(QString peerId, QString agentName, QString definitionJSON)
{
    if (!definitionJSON.isEmpty() && (_jsonHelper != nullptr))
    {
        AgentM* agent = getAgentModelFromPeerId(peerId);
        if (agent != nullptr)
        {
            // Create a model of agent definition from JSON
            DefinitionM* agentDefinition = _jsonHelper->createModelOfAgentDefinitionFromBytes(definitionJSON.toUtf8());
            if (agentDefinition != nullptr)
            {
                 if (agent->definition() == nullptr)
                 {
                     // Set this definition to the agent
                     agent->setdefinition(agentDefinition);

                     if (!agentDefinition->outputsList()->isEmpty()) {
                         Q_EMIT addInputsToEditorForOutputs(agentName, agentDefinition->outputsList()->toList());
                     }
                 }
                 // Update with the new definition
                 else
                 {
                     DefinitionM* previousDefinition = agent->definition();
                     if (previousDefinition != nullptr)
                     {
                         //
                         // Check if output(s) have been removed
                         //
                         QList<OutputM*> removedOutputsList;
                         for (OutputM* output : previousDefinition->outputsList()->toList()) {
                             if ((output != nullptr) && !output->id().isEmpty() && !agentDefinition->outputsIdsList().contains(output->id())) {
                                 removedOutputsList.append(output);
                             }
                         }
                         if (!removedOutputsList.isEmpty()) {
                             // Emit the signal "Remove Inputs to Editor for Outputs"
                             Q_EMIT removeInputsToEditorForOutputs(agentName, removedOutputsList);
                         }

                         // Set this definition to the agent
                         agent->setdefinition(agentDefinition);


                         //
                         // Check if output(s) have been added
                         //
                         QList<OutputM*> addedOutputsList;
                         for (OutputM* output : agentDefinition->outputsList()->toList()) {
                             if ((output != nullptr) && !output->id().isEmpty() && !previousDefinition->outputsIdsList().contains(output->id())) {
                                 addedOutputsList.append(output);
                             }
                         }
                         if (!addedOutputsList.isEmpty()) {
                             // Emit the signal "Add Inputs to Editor for Outputs"
                             Q_EMIT addInputsToEditorForOutputs(agentName, addedOutputsList);
                         }


                         // Delete the previous model of agent definition
                         delete previousDefinition;
                     }
                 }

                 // Emit the signal "Active Agent Defined"
                 Q_EMIT activeAgentDefined(agent);
            }
        }
    }
}


/**
 * @brief Slot called when an agent mapping has been received and must be processed
 * @param peer Id
 * @param agent name
 * @param mapping in JSON format
 */
void IngeScapeModelManager::onMappingReceived(QString peerId, QString agentName, QString mappingJSON)
{
    AgentM* agent = getAgentModelFromPeerId(peerId);
    if ((agent != nullptr) && (_jsonHelper != nullptr))
    {
        AgentMappingM* agentMapping = NULL;

        if (mappingJSON.isEmpty())
        {
            QString mappingName = QString("EMPTY MAPPING of %1").arg(agentName);
            agentMapping = new AgentMappingM(mappingName, "", "");
        }
        else
        {
            // Create a model of agent mapping from the JSON
            agentMapping = _jsonHelper->createModelOfAgentMappingFromBytes(agentName, mappingJSON.toUtf8());
        }

        if (agentMapping != nullptr)
        {
            if (agent->mapping() == nullptr)
            {
                // Set this mapping to the agent
                agent->setmapping(agentMapping);

                // Emit the signal "Active Agent Mapping Defined"
                Q_EMIT activeAgentMappingDefined(agent);
            }
            // There is already a mapping for this agent
            else
            {
                qWarning() << "Update the mapping of agent" << agentName << "(if this mapping has changed)";

                AgentMappingM* previousMapping = agent->mapping();

                QStringList idsOfRemovedMappingElements;
                for (QString idPreviousList : previousMapping->namesOfMappingElements())
                {
                    if (!agentMapping->namesOfMappingElements().contains(idPreviousList)) {
                        idsOfRemovedMappingElements.append(idPreviousList);
                    }
                }

                QStringList idsOfAddedMappingElements;
                for (QString idNewList : agentMapping->namesOfMappingElements())
                {
                    if (!previousMapping->namesOfMappingElements().contains(idNewList)) {
                        idsOfAddedMappingElements.append(idNewList);
                    }
                }

                // If there are some Removed mapping elements
                if (!idsOfRemovedMappingElements.isEmpty())
                {
                    for (ElementMappingM* mappingElement : previousMapping->mappingElements()->toList())
                    {
                        if ((mappingElement != nullptr) && idsOfRemovedMappingElements.contains(mappingElement->id()))
                        {
                            // Emit the signal "UN-mapped"
                            Q_EMIT unmapped(mappingElement);
                        }
                    }
                }
                // If there are some Added mapping elements
                if (!idsOfAddedMappingElements.isEmpty())
                {
                    for (ElementMappingM* mappingElement : agentMapping->mappingElements()->toList())
                    {
                        if ((mappingElement != nullptr) && idsOfAddedMappingElements.contains(mappingElement->id()))
                        {
                            // Emit the signal "Mapped"
                            Q_EMIT mapped(mappingElement);
                        }
                    }
                }

                // Set this new mapping to the agent
                agent->setmapping(agentMapping);

                // Delete the previous model of agent mapping
                delete previousMapping;
            }
        }
    }
}


/**
 * @brief Slot called when a new value is published
 * @param publishedValue
 */
void IngeScapeModelManager::onValuePublished(PublishedValueM* publishedValue)
{
    if (publishedValue != nullptr)
    {
        // Add to the list at the first position
        _publishedValues.prepend(publishedValue);

        // Get the (view model of) agents grouped for the name
        AgentsGroupedByNameVM* agentsGroupedByName = getAgentsGroupedForName(publishedValue->agentName());
        if (agentsGroupedByName != nullptr)
        {
            // Update the current value of an I/O/P of this agent(s)
            agentsGroupedByName->updateCurrentValueOfIOP(publishedValue);
        }
    }
}


/**
 * @brief Slot called when the flag "is Muted" from an agent updated
 * @param peerId
 * @param isMuted
 */
void IngeScapeModelManager::onisMutedFromAgentUpdated(QString peerId, bool isMuted)
{
    AgentM* agent = getAgentModelFromPeerId(peerId);
    if (agent != nullptr) {
        agent->setisMuted(isMuted);
    }
}


/**
 * @brief Slot called when the flag "can be Frozen" from an agent updated
 * @param peerId
 * @param canBeFrozen
 */
void IngeScapeModelManager::onCanBeFrozenFromAgentUpdated(QString peerId, bool canBeFrozen)
{
    AgentM* agent = getAgentModelFromPeerId(peerId);
    if (agent != nullptr) {
        agent->setcanBeFrozen(canBeFrozen);
    }
}


/**
 * @brief Slot called when the flag "is Frozen" from an agent updated
 * @param peerId
 * @param isFrozen
 */
void IngeScapeModelManager::onIsFrozenFromAgentUpdated(QString peerId, bool isFrozen)
{
    AgentM* agent = getAgentModelFromPeerId(peerId);
    if (agent != nullptr) {
        agent->setisFrozen(isFrozen);
    }
}


/**
 * @brief Slot called when the flag "is Muted" from an output of agent updated
 * @param peerId
 * @param isMuted
 * @param outputName
 */
void IngeScapeModelManager::onIsMutedFromOutputOfAgentUpdated(QString peerId, bool isMuted, QString outputName)
{
    AgentM* agent = getAgentModelFromPeerId(peerId);
    if (agent != nullptr) {
        agent->setisMutedOfOutput(isMuted, outputName);
    }
}


/**
 * @brief Slot called when the state of an agent changes
 * @param peerId
 * @param stateName
 */
void IngeScapeModelManager::onAgentStateChanged(QString peerId, QString stateName)
{
    AgentM* agent = getAgentModelFromPeerId(peerId);
    if (agent != nullptr) {
        agent->setstate(stateName);
    }
}


/**
 * @brief Slot called when we receive the flag "Log In Stream" for an agent
 * @param peerId
 * @param hasLogInStream
 */
void IngeScapeModelManager::onAgentHasLogInStream(QString peerId, bool hasLogInStream)
{
    AgentM* agent = getAgentModelFromPeerId(peerId);
    if (agent != nullptr) {
        agent->sethasLogInStream(hasLogInStream);
    }
}


/**
 * @brief Slot called when we receive the flag "Log In File" for an agent
 * @param peerId
 * @param hasLogInStream
 */
void IngeScapeModelManager::onAgentHasLogInFile(QString peerId, bool hasLogInFile)
{
    AgentM* agent = getAgentModelFromPeerId(peerId);
    if (agent != nullptr) {
        agent->sethasLogInFile(hasLogInFile);
    }
}


/**
 * @brief Slot called when we receive the path of "Log File" for an agent
 * @param peerId
 * @param logFilePath
 */
void IngeScapeModelManager::onAgentLogFilePath(QString peerId, QString logFilePath)
{
    AgentM* agent = getAgentModelFromPeerId(peerId);
    if (agent != nullptr) {
        agent->setlogFilePath(logFilePath);
    }
}


/**
 * @brief Slot called when we receive the path of "Definition File" for an agent
 * @param peerId
 * @param definitionFilePath
 */
void IngeScapeModelManager::onAgentDefinitionFilePath(QString peerId, QString definitionFilePath)
{
    AgentM* agent = getAgentModelFromPeerId(peerId);
    if (agent != nullptr) {
        agent->setdefinitionFilePath(definitionFilePath);
    }
}


/**
 * @brief Slot called when we receive the path of "Mapping File" for an agent
 * @param peerId
 * @param mappingFilePath
 */
void IngeScapeModelManager::onAgentMappingFilePath(QString peerId, QString mappingFilePath)
{
    AgentM* agent = getAgentModelFromPeerId(peerId);
    if (agent != nullptr) {
        agent->setmappingFilePath(mappingFilePath);
    }
}


/**
 * @brief Slot called when the network data (of an agent) will be cleared
 * @param peerId
 */
void IngeScapeModelManager::_onNetworkDataOfAgentWillBeCleared(QString peerId)
{
    /*AgentM* agent = qobject_cast<AgentM*>(sender());
    if (agent != nullptr)
    {
        qDebug() << "Model Manager: on Network Data of agent" << agent->name() << "will be cleared" << agent->hostname() << "(" << agent->peerId() << ")";
    }*/

    if (!peerId.isEmpty()) {
        _mapFromPeerIdToAgentM.remove(peerId);
    }
}


/**
 * @brief Open a list of definitions (if the definition is already opened, we bring it to front)
 * @param definitionsToOpen
 */
void IngeScapeModelManager::_openDefinitions(QList<DefinitionM*> definitionsToOpen)
{
    // Traverse the list of definitions to open
    for (DefinitionM* definition : definitionsToOpen)
    {
        if (definition != nullptr)
        {
            if (!_openedDefinitions.contains(definition)) {
                _openedDefinitions.append(definition);
            }
            else {
                qDebug() << "The 'Definition'" << definition->name() << "is already opened...bring it to front !";

                Q_EMIT definition->bringToFront();
            }
        }
    }
}


/**
 * @brief Print all models of agents (for Debug)
 */
void IngeScapeModelManager::_printAgents()
{
    qDebug() << "Print Agents:";
    for (AgentsGroupedByNameVM* agentsGroupedByName : _allAgentsGroupedByName.toList())
    {
        if (agentsGroupedByName != nullptr) {
            qDebug() << agentsGroupedByName->name() << ":" << agentsGroupedByName->models()->count() << "agents";
        }
    }
}
