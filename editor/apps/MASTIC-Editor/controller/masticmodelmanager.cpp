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
 *      Alexandre Lemort   <lemort@ingenuity.io>
 *
 */

#include "masticmodelmanager.h"

#include <QQmlEngine>
#include <QDebug>

#include <I2Quick.h>

#include <QJsonDocument>


/**
 * @brief Default constructor
 * @param parent
 */
MasticModelManager::MasticModelManager(QObject *parent) : QObject(parent),
    _jsonHelper(NULL)
{
    // Force ownership of our object, it will prevent Qml from stealing it
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

    qInfo() << "New MASTIC Model Manager";

    // Create the helper to manage JSON definitions of agents
    _jsonHelper = new JsonHelper(this);
}


/**
 * @brief Destructor
 */
MasticModelManager::~MasticModelManager()
{
    // Delete all models of agents
    //qDeleteAll(_allAgentsModel);

    qInfo() << "Delete MASTIC Model Manager";
}


/**
 * @brief Initialize agents (from JSON files) inside a directory
 * @param agentsDirectoryPath agents directory path
 */
void MasticModelManager::initAgentsInsideDirectory(QString agentsDirectoryPath)
{
    QDir agentsDefinitionsAndMappingsDirectory(agentsDirectoryPath);
    if (agentsDefinitionsAndMappingsDirectory.exists())
    {
        // Get all sub directories
        QFileInfoList agentsDirectoriesList = agentsDefinitionsAndMappingsDirectory.entryInfoList(QDir::NoDotAndDotDot | QDir::AllDirs);

        // Traverse the list of sub directories
        foreach (QFileInfo fileInfo, agentsDirectoriesList)
        {
            if (fileInfo.isDir()) {
                // Initialize an agent (from JSON files) inside a sub directory
                _initAgentInsideSubDirectory(fileInfo.absoluteFilePath());
            }
        }
    }
}


/**
 * @brief Slot when an agent enter the network
 * @param peerId
 * @param agentName
 * @param agentAddress
 * @param pid
 * @param hostname
 * @param executionPath
 * @param canBeFrozen
 */
void MasticModelManager::onAgentEntered(QString peerId, QString agentName, QString agentAddress, int pid, QString hostname, QString executionPath, bool canBeFrozen)
{
    if (!peerId.isEmpty() && !agentName.isEmpty() && !agentAddress.isEmpty())
    {
        AgentM* agent = getAgentModelFromPeerId(peerId);

        // An agent with this peer id already exist
        if (agent != NULL)
        {
            qInfo() << "The agent" << agentName << "with peer id" << peerId << "and address" << agentAddress << "is back on the network !";

            // Update the status
            agent->setstatus(AgentStatus::ON);
        }
        // New peer id
        else
        {
            // Create a new model of agent
            agent = new AgentM(agentName, peerId, agentAddress, this);

            agent->sethostname(hostname);
            agent->setexecutionPath(executionPath);
            agent->setpid(pid);
            agent->setcanBeFrozen(canBeFrozen);
            agent->setstatus(AgentStatus::ON);

            _mapFromPeerIdToAgentM.insert(peerId, agent);

            // Add this new model of agent
            addAgentModel(agent);

            // Emit the signal "Agent Model Created"
            Q_EMIT agentModelCreated(agent);
        }
    }
}


/**
 * @brief Slot when an agent definition has been received and must be processed
 * @param peer Id
 * @param agent name
 * @param definition in JSON format
 */
void MasticModelManager::onDefinitionReceived(QString peerId, QString agentName, QString definitionJSON)
{
    Q_UNUSED(agentName)

    if (!definitionJSON.isEmpty())
    {
        AgentM* agent = getAgentModelFromPeerId(peerId);
        if (agent != NULL)
        {
            QByteArray byteArrayOfJson = definitionJSON.toUtf8();

            // Create a model of agent definition with JSON
            DefinitionM* definition = _jsonHelper->createModelOfDefinition(byteArrayOfJson);
            if (definition != NULL)
            {
                // Add this new model of agent definition
                addAgentDefinition(definition);

                // Emit the signal "Agent Definition Created"
                Q_EMIT agentDefinitionCreated(definition, agent);
            }
        }
    }
}


/**
 * @brief Slot when an agent mapping has been received and must be processed
 * @param peer Id
 * @param agent name
 * @param mapping in JSON format
 */
void MasticModelManager::onMappingReceived(QString peerId, QString agentName, QString mappingJSON)
{
    if (!mappingJSON.isEmpty())
    {
        AgentM* agent = getAgentModelFromPeerId(peerId);
        if(agent != NULL)
        {
            QByteArray byteArrayOfJson = mappingJSON.toUtf8();

            // Create a model of agent mapping with JSON
            AgentMappingM* agentMapping = _jsonHelper->createModelOfAgentMapping(agentName, byteArrayOfJson);
            if (agentMapping != NULL)
            {
                // Add this new model of agent mapping
                addAgentMapping(agentMapping);

                // Emit the signal "Agent Mapping Created"
                Q_EMIT agentMappingCreated(agentMapping, agent);
            }
        }
    }
}


/**
 * @brief Slot when an agent quit the network
 * @param peer Id
 * @param agent name
 */
void MasticModelManager::onAgentExited(QString peerId, QString agentName)
{
    AgentM* agent = getAgentModelFromPeerId(peerId);
    if(agent != NULL)
    {
        qInfo() << "The agent" << agentName << "with peer id" << peerId << "exited from the network !";

        // Update the status
        agent->setstatus(AgentStatus::OFF);
    }
}


/**
 * @brief Slot when the flag "Is Muted" of an agent updated
 * @param peerId
 * @param isMuted
 */
void MasticModelManager::onisMutedOfAgentUpdated(QString peerId, bool isMuted)
{
    AgentM* agent = getAgentModelFromPeerId(peerId);
    if(agent != NULL) {
        agent->setisMuted(isMuted);
    }
}


/**
 * @brief Slot when the flag "Is Frozen" of an agent updated
 * @param peerId
 * @param isFrozen
 */
void MasticModelManager::onIsFrozenOfAgentUpdated(QString peerId, bool isFrozen)
{
    AgentM* agent = getAgentModelFromPeerId(peerId);
    if(agent != NULL) {
        agent->setisFrozen(isFrozen);
    }
}


/**
 * @brief Add a model of agent
 * @param agent
 */
void MasticModelManager::addAgentModel(AgentM* agent)
{
    if (agent != NULL)
    {
        QList<AgentM*> agentModelsList = getAgentModelsListFromName(agent->name());
        agentModelsList.append(agent);

        // Update the list in the map
        _mapFromNameToAgentModelsList.insert(agent->name(), agentModelsList);
    }
}


/**
 * @brief Get the model of agent from a Peer Id
 * @param peerId
 * @return
 */
AgentM* MasticModelManager::getAgentModelFromPeerId(QString peerId)
{
    if (_mapFromPeerIdToAgentM.contains(peerId)) {
        return _mapFromPeerIdToAgentM.value(peerId);
    }
    else {
        return NULL;
    }
}


/**
 * @brief Get the list of models of agent from a name
 * @param name
 * @return
 */
QList<AgentM*> MasticModelManager::getAgentModelsListFromName(QString name)
{
    if (_mapFromNameToAgentModelsList.contains(name)) {
        return _mapFromNameToAgentModelsList.value(name);
    }
    else {
        return QList<AgentM*>();
    }
}


/**
 * @brief Delete a model of Agent
 * @param agant
 */
void MasticModelManager::deleteAgentModel(AgentM* agent)
{
    if (agent != NULL)
    {
        QList<AgentM*> agentModelsList = getAgentModelsListFromName(agent->name());
        agentModelsList.removeOne(agent);

        // Update the list in the map
        _mapFromNameToAgentModelsList.insert(agent->name(), agentModelsList);

        // Free memory
        delete agent;
    }
}


/**
 * @brief Add a model of agent definition
 * @param definition
 */
void MasticModelManager::addAgentDefinition(DefinitionM* definition)
{
    if (definition != NULL)
    {
        QString definitionName = definition->name();

        QList<DefinitionM*> agentDefinitionsList = getAgentDefinitionsListFromName(definitionName);
        agentDefinitionsList.append(definition);

        // Update the list in the map
        _mapFromNameToAgentDefinitionsList.insert(definitionName, agentDefinitionsList);

        // Update definition variants of a list of definitions with the same name
        _updateDefinitionVariants(definitionName);
    }
}


/**
 * @brief Get the list (of models) of agent definition from a name
 * @param name
 * @return
 */
QList<DefinitionM*> MasticModelManager::getAgentDefinitionsListFromName(QString name)
{
    if (_mapFromNameToAgentDefinitionsList.contains(name)) {
        return _mapFromNameToAgentDefinitionsList.value(name);
    }
    else {
        return QList<DefinitionM*>();
    }
}


/**
 * @brief Delete a model of agent definition
 * @param definition
 */
void MasticModelManager::deleteAgentDefinition(DefinitionM* definition)
{
    if (definition != NULL)
    {
        QString definitionName = definition->name();

        QList<DefinitionM*> agentDefinitionsList = getAgentDefinitionsListFromName(definitionName);
        agentDefinitionsList.removeOne(definition);

        // Update the list in the map
        _mapFromNameToAgentDefinitionsList.insert(definitionName, agentDefinitionsList);

        // Free memory
        delete definition;

        // Update definition variants of a list of definitions with the same name
        _updateDefinitionVariants(definitionName);
    }
}


/**
 * @brief Add a model of agent mapping
 * @param agentMapping
 */
void MasticModelManager::addAgentMapping(AgentMappingM* agentMapping)
{
    if (agentMapping != NULL)
    {
        QString mappingName = agentMapping->name();

        QList<AgentMappingM*> agentMappingsList = getAgentMappingsListFromName(mappingName);
        agentMappingsList.append(agentMapping);

        // Update the list in the map
        _mapFromNameToAgentMappingsList.insert(mappingName, agentMappingsList);

        /*foreach (ElementMappingM* elementMapping, agentMapping->elementMappingsList()) {
            if (elementMapping != NULL) {

            }
        }*/
    }
}


/**
 * @brief Get the list (of models) of agent mapping from a name
 * @param name
 * @return
 */
QList<AgentMappingM*> MasticModelManager::getAgentMappingsListFromName(QString name)
{
    if (_mapFromNameToAgentMappingsList.contains(name)) {
        return _mapFromNameToAgentMappingsList.value(name);
    }
    else {
        return QList<AgentMappingM*>();
    }
}


/**
 * @brief Initialize an agent (from JSON files) inside a sub directory
 * @param subDirectoryPath
 */
void MasticModelManager::_initAgentInsideSubDirectory(QString subDirectoryPath)
{
    QDir subDirectory(subDirectoryPath);
    if (subDirectory.exists())
    {
        QStringList nameFilters;
        nameFilters << "*.json";

        QFileInfoList agentsFilesList = subDirectory.entryInfoList(nameFilters);
        qInfo() << agentsFilesList.count() << "files in directory" << subDirectory.absolutePath();

        DefinitionM* agentDefinition = NULL;
        AgentMappingM* agentMapping = NULL;

        // Only 1 file, it must be the definition
        if (agentsFilesList.count() == 1)
        {
            QFileInfo fileInfo = agentsFilesList.first();

            QFile jsonFile(fileInfo.absoluteFilePath());
            if (jsonFile.open(QIODevice::ReadOnly))
            {
                QByteArray byteArrayOfJson = jsonFile.readAll();
                jsonFile.close();

                // Create a model of agent definition with JSON
                agentDefinition = _jsonHelper->createModelOfDefinition(byteArrayOfJson);
            }
            else {
                qCritical() << "Can not open file" << fileInfo.absoluteFilePath();
            }
        }
        // 2 files, they must be the definition and the mapping
        else if (agentsFilesList.count() == 2)
        {
            QFileInfo fileInfo1 = agentsFilesList.at(0);
            QFileInfo fileInfo2 = agentsFilesList.at(1);

            QFile jsonFile1(fileInfo1.absoluteFilePath());
            QFile jsonFile2(fileInfo2.absoluteFilePath());

            if (jsonFile1.open(QIODevice::ReadOnly) && jsonFile2.open(QIODevice::ReadOnly))
            {
                QByteArray byteArrayOfJson1 = jsonFile1.readAll();
                jsonFile1.close();

                QByteArray byteArrayOfJson2 = jsonFile2.readAll();
                jsonFile2.close();

                // Try to create a model of agent definition with the first JSON
                agentDefinition = _jsonHelper->createModelOfDefinition(byteArrayOfJson1);

                // If succeeded
                if (agentDefinition != NULL)
                {
                    // Create a model of agent mapping with the second JSON
                    agentMapping = _jsonHelper->createModelOfAgentMapping(agentDefinition->name(), byteArrayOfJson2);
                }
                else
                {
                    // Try to create a model of agent definition with the second JSON
                    agentDefinition = _jsonHelper->createModelOfDefinition(byteArrayOfJson2);

                    // If succeeded
                    if (agentDefinition != NULL)
                    {
                        // Create a model of agent mapping with the first JSON
                        agentMapping = _jsonHelper->createModelOfAgentMapping(agentDefinition->name(), byteArrayOfJson1);
                    }
                }
            }
            else {
                qCritical() << "Can not open 2 files" << fileInfo1.absoluteFilePath() << "and" << fileInfo2.absoluteFilePath();
            }
        }
        else {
            qCritical() << "There are more than 2 JSON files in the directory" << subDirectory.absolutePath();
        }

        if (agentDefinition != NULL)
        {
            // Create a new model of agent with the name of the definition
            AgentM* agent = new AgentM(agentDefinition->name(), this);

            // Add this new model of agent
            addAgentModel(agent);

            // Emit the signal "Agent Model Created"
            Q_EMIT agentModelCreated(agent);

            // Add this new model of agent definition
            addAgentDefinition(agentDefinition);

            // Emit the signal "Agent Definition Created"
            Q_EMIT agentDefinitionCreated(agentDefinition, agent);

            if (agentMapping != NULL)
            {
                // Add this new model of agent mapping
                addAgentMapping(agentMapping);

                // Emit the signal "Agent Mapping Created"
                Q_EMIT agentMappingCreated(agentMapping, agent);
            }
        }
    }
}


/**
 * @brief Update definition variants of a list of definitions with the same name
 * @param definitionName
 */
void MasticModelManager::_updateDefinitionVariants(QString definitionName)
{
    QList<DefinitionM*> agentDefinitionsList = getAgentDefinitionsListFromName(definitionName);

    // We can use versions as keys of the map because the list contains only definition with the same name
    QHash<QString, QList<DefinitionM*>> mapFromVersionToDefinitionsList;
    QList<QString> versionsWithVariant;

    foreach (DefinitionM* iterator, agentDefinitionsList)
    {
        if ((iterator != NULL) && !iterator->version().isEmpty())
        {
            // First, reset all
            iterator->setisVariant(false);

            QString version = iterator->version();
            QList<DefinitionM*> definitionsListForVersion;

            // Other(s) definition(s) have the same version (and the same name)
            if (mapFromVersionToDefinitionsList.contains(version)) {
                definitionsListForVersion = mapFromVersionToDefinitionsList.value(version);

                // The lists of I/O/P must be different to have a variant !
                if (!versionsWithVariant.contains(version))
                {
                    // We compare I/O/P between current iterator and the first one
                    DefinitionM* first = definitionsListForVersion.first();
                    if ((first != NULL) && (first->md5Hash() != iterator->md5Hash())) {
                        versionsWithVariant.append(version);
                    }
                }
            }

            definitionsListForVersion.append(iterator);
            mapFromVersionToDefinitionsList.insert(version, definitionsListForVersion);
        }
    }

    // The list contains only the versions that have variants
    foreach (QString version, versionsWithVariant)
    {
        QList<DefinitionM*> definitionsListForVersion = mapFromVersionToDefinitionsList.value(version);
        foreach (DefinitionM* iterator, definitionsListForVersion)
        {
            if (iterator != NULL) {
                iterator->setisVariant(true);
                //qDebug() << iterator->name() << iterator->version() << "is variant (" << iterator->md5Hash() << ")";
            }
        }
    }
}

