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

#include "agentinmappingvm.h"

/**
 * @brief Default constructor
 * @param models. The first agent is needed to instanciate an agent mapping VM.
 * Typically passing during the drag-drop from the list of agents on the left side.
 * @param position Position of the top left corner
 * @param parent
 */
AgentInMappingVM::AgentInMappingVM(QList<AgentM*> models,
                                   QPointF position,
                                   QObject *parent) : QObject(parent),
    _agentName(""),
    _position(position),
    _isON(false),
    _isReduced(false),
    _reducedMapValueTypeGroupInInput(AgentIOPValueTypeGroups::MIXED),
    _reducedMapValueTypeGroupInOutput(AgentIOPValueTypeGroups::MIXED),
    _isGhost(false),
    _areIdenticalsAllDefinitions(true)
{
    // Force ownership of our object, it will prevent Qml from stealing it
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

    if (models.count() > 0)
    {
        AgentM* firstModel = models.first();
        if (firstModel != NULL)
        {
            // Set the name of our agent in mapping
            _agentName = firstModel->name();

            // Connect to signal "Count Changed" from the list of models
            connect(&_models, &AbstractI2CustomItemListModel::countChanged, this, &AgentInMappingVM::_onModelsChanged);
            //connect(&_inputsList, &AbstractI2CustomItemListModel::countChanged, this, &AgentInMappingVM::_onInputsListChanged);
            //connect(&_outputsList, &AbstractI2CustomItemListModel::countChanged, this, &AgentInMappingVM::_onOutputsListChanged);

            // Initialize our list
            _models.append(models);
        }
        else {
            qCritical() << "No agent model for the agent in mapping !";
        }
    }
}


/**
 * @brief Ghost Constructor: model (and definition) is not defined.
 * The agent is an empty shell only defined by a name.
 * @param agentName
 * @param parent
 */
AgentInMappingVM::AgentInMappingVM(QString agentName,
                                   QObject *parent) : AgentInMappingVM(QList<AgentM*>(),
                                                                       QPointF(),
                                                                       parent)
{
    setagentName(agentName);
    setisGhost(true);

    qInfo() << "New Ghost of Agent in Mapping" << _agentName;
}


/**
 * @brief Destructor
 */
AgentInMappingVM::~AgentInMappingVM()
{
    qInfo() << "Delete View Model of Agent in Mapping" << _agentName;

    disconnect(&_models, &AbstractI2CustomItemListModel::countChanged, this, &AgentInMappingVM::_onModelsChanged);
    //disconnect(&_inputsList, &AbstractI2CustomItemListModel::countChanged, this, &AgentInMappingVM::_onInputsListChanged);
    //disconnect(&_outputsList, &AbstractI2CustomItemListModel::countChanged, this, &AgentInMappingVM::_onOutputsListChanged);

    // Clear maps of Inputs & Outputs
    _mapFromNameToInputsList.clear();
    _mapFromUniqueIdToInput.clear();
    _mapFromNameToOutputsList.clear();
    _mapFromUniqueIdToOutput.clear();

    // Delete elements in the lists of Inputs & Outputs
    _inputsList.deleteAllItems();
    _outputsList.deleteAllItems();

    // Clear the previous list of models
    _previousAgentsList.clear();

    // Clear the list of definition
    _models.clear();
}


/**
 * @brief Return the corresponding list of view models of input from the input name
 * @param inputName
 */
QList<InputVM*> AgentInMappingVM::getInputsListFromName(QString inputName)
{
    if (_mapFromNameToInputsList.contains(inputName)) {
        return _mapFromNameToInputsList.value(inputName);
    }
    else {
        return QList<InputVM*>();
    }
}


/**
 * @brief Return the corresponding view model of input from the input id
 * @param inputId
 */
InputVM* AgentInMappingVM::getInputFromId(QString inputId)
{
    if (_mapFromUniqueIdToInput.contains(inputId)) {
        return _mapFromUniqueIdToInput.value(inputId);
    }
    else {
        return NULL;
    }
}


/**
 * @brief Return the corresponding list of view models of output from the output name
 * @param outputName
 */
QList<OutputVM*> AgentInMappingVM::getOutputsListFromName(QString outputName)
{
    if (_mapFromNameToOutputsList.contains(outputName)) {
        return _mapFromNameToOutputsList.value(outputName);
    }
    else {
        return QList<OutputVM*>();
    }
}


/**
 * @brief Return the corresponding view model of output from the output id
 * @param outputId
 */
OutputVM* AgentInMappingVM::getOutputFromId(QString outputId)
{
    if (_mapFromUniqueIdToOutput.contains(outputId)) {
        return _mapFromUniqueIdToOutput.value(outputId);
    }
    else {
        return NULL;
    }
}


/**
 * @brief Slot when the list of models changed
 */
void AgentInMappingVM::_onModelsChanged()
{
    QList<AgentM*> newAgentsList = _models.toList();

    // Model of agent added
    if (_previousAgentsList.count() < newAgentsList.count())
    {
        qDebug() << _previousAgentsList.count() << "--> ADD --> " << newAgentsList.count();

        for (AgentM* model : newAgentsList) {
            if ((model != NULL) && !_previousAgentsList.contains(model))
            {
                qDebug() << "New model" << model->name() << "ADDED (" << model->peerId() << ")";

                // Connect to signals from a model
                connect(model, &AgentM::isONChanged, this, &AgentInMappingVM::_onIsONofModelChanged);
                //connect(model, &AgentM::definitionChanged, this, &AgentInMappingVM::_onDefinitionOfModelChanged);
                //connect(model, &AgentM::mappingChanged, this, &AgentInMappingVM::_onMappingOfModelChanged);

                // A model of agent has been added to our list
                _agentModelAdded(model);
            }
        }
    }
    // Model of agent removed
    else if (_previousAgentsList.count() > newAgentsList.count())
    {
        qDebug() << _previousAgentsList.count() << "--> REMOVE --> " << newAgentsList.count();

        for (AgentM* model : _previousAgentsList) {
            if ((model != NULL) && !newAgentsList.contains(model))
            {
                qDebug() << "Old model" << model->name() << "REMOVED (" << model->peerId() << ")";

                // DIS-connect from signals from a model
                disconnect(model, &AgentM::isONChanged, this, &AgentInMappingVM::_onIsONofModelChanged);
                //disconnect(model, &AgentM::definitionChanged, this, &AgentInMappingVM::_onDefinitionOfModelChanged);
                //disconnect(model, &AgentM::mappingChanged, this, &AgentInMappingVM::_onMappingOfModelChanged);

                // A model of agent has been removed from our list
                _agentModelRemoved(model);
            }
        }
    }

    _previousAgentsList = newAgentsList;

    // Update with all models
    _updateWithAllModels();
}


/**
 * @brief Slot when the list of (view models of) inputs changed
 */
/*void AgentInMappingVM::_onInputsListChanged()
{
    foreach (InputVM* input, _inputsList.toList()) {
        if ((input != NULL) && (input->firstModel() != NULL)) {
        }
    }
}*/


/**
 * @brief Slot when the list of (view models of) outputs changed
 */
/*void AgentInMappingVM::_onOutputsListChanged()
{
    foreach (OutputVM* output, _outputsList.toList()) {
        if ((output != NULL) && (output->firstModel() != NULL)) {
        }
    }
}*/


/**
 * @brief Slot when the flag "is ON" of a model changed
 * @param isON
 */
void AgentInMappingVM::_onIsONofModelChanged(bool isON)
{
    Q_UNUSED(isON)

    // Update the flag "is ON" in function of flags of all models
    _updateIsON();
}


/**
 * @brief A model of agent has been added to our list
 * @param model
 */
void AgentInMappingVM::_agentModelAdded(AgentM* model)
{
    if ((model != NULL) && (model->definition() != NULL))
    {
        QList<InputVM*> inputsListToAdd;
        QList<OutputVM*> outputsListToAdd;

        // Traverse the list of models of inputs in the definition
        foreach (AgentIOPM* input, model->definition()->inputsList()->toList())
        {
            // This method returns a view model only if it is a new one
            InputVM* newInputVM = _inputModelAdded(input);

            if (newInputVM != NULL) {
                inputsListToAdd.append(newInputVM);
            }
        }

        // Traverse the list of models of outputs in the definition
        foreach (OutputM* output, model->definition()->outputsList()->toList())
        {
            // This method returns a view model only if it is a new one
            OutputVM* newOutputVM = _outputModelAdded(output);

            if (newOutputVM != NULL) {
                outputsListToAdd.append(newOutputVM);
            }
        }

        _inputsList.append(inputsListToAdd);
        _outputsList.append(outputsListToAdd);

        // Emit signal "Inputs List Added"
        Q_EMIT inputsListAdded(inputsListToAdd);

        // Emit signal "Outputs List Added"
        Q_EMIT outputsListAdded(outputsListToAdd);
    }
}


/**
 * @brief A model of agent has been removed from our list
 * @param model
 */
void AgentInMappingVM::_agentModelRemoved(AgentM* model)
{
    if ((model != NULL) && (model->definition() != NULL))
    {
        // Traverse the list of models of inputs in the definition
        foreach (AgentIOPM* input, model->definition()->inputsList()->toList())
        {
            InputVM* inputVM = _inputModelRemoved(input);
            Q_UNUSED(inputVM)

            // Usefull ?
            /*if ((inputVM != NULL) && (inputVM->models()->count() == 0)) {
                _inputsList.remove(inputVM);
            }*/
        }

        // Traverse the list of models of outputs in the definition
        foreach (OutputM* output, model->definition()->outputsList()->toList())
        {
            OutputVM* outputVM = _outputModelRemoved(output);
            Q_UNUSED(outputVM)

            // Usefull ?
            /*if ((outputVM != NULL) && (outputVM->models()->count() == 0)) {
                _outputsList.remove(outputVM);
            }*/
        }
    }
}


/**
 * @brief A model of input has been added
 * @param input
 * @return a view model of input only if it is a new one
 */
InputVM* AgentInMappingVM::_inputModelAdded(AgentIOPM* input)
{
    InputVM* newInputVM = NULL;

    if (input != NULL)
    {
        // First, we get a ghost of this input: an input without id (only the same name)
        QList<InputVM*> inputsWithSameName = getInputsListFromName(input->name());
        InputVM* inputWithoutId = NULL;

        foreach (InputVM* iterator, inputsWithSameName)
        {
            // Already a view model with an EMPTY id (NOT defined)
            if ((iterator != NULL) && iterator->id().isEmpty()) {
                inputWithoutId = iterator;
                break;
            }
        }

        // Input id is NOT defined
        if (input->id().isEmpty())
        {
            // There is already a view model without id
            if (inputWithoutId != NULL)
            {
                // Add this new model to the list
                inputWithoutId->models()->append(input);
            }
            // There is not yet a view model without id
            else
            {
                // Create a new view model of input (without id)
                newInputVM = new InputVM(input->name(),
                                         "",
                                         input,
                                         this);

                // Don't add to the list here (this input will be added globally via temporary list)

                // Update the hash table with the input name
                inputsWithSameName.append(newInputVM);
                _mapFromNameToInputsList.insert(input->name(), inputsWithSameName);
            }
        }
        // Input id is defined
        else
        {
            // There is a view model without id
            if (inputWithoutId != NULL)
            {
                // FIXME TODO: gestion du ghost...passage en view model avec id
            }

            InputVM* inputVM = getInputFromId(input->id());

            // There is already a view model for this id
            if (inputVM != NULL)
            {
                // Add this new model to the list
                inputVM->models()->append(input);
            }
            // There is not yet a view model for this id
            else
            {
                // Create a new view model of input
                newInputVM = new InputVM(input->name(),
                                         input->id(),
                                         input,
                                         this);

                // Don't add to the list here (this input will be added globally via temporary list)

                // Update the hash table with the input id
                _mapFromUniqueIdToInput.insert(input->id(), newInputVM);

                // Update the hash table with the input name
                inputsWithSameName.append(newInputVM);
                _mapFromNameToInputsList.insert(input->name(), inputsWithSameName);
            }
        }
    }

    return newInputVM;
}


/**
 * @brief A model of input has been removed
 * @param input
 * @return
 */
InputVM* AgentInMappingVM::_inputModelRemoved(AgentIOPM* input)
{
    InputVM* inputVM = NULL;

    if (input != NULL)
    {
        // Input id is defined
        if (!input->id().isEmpty())
        {
            inputVM = getInputFromId(input->id());
            if (inputVM != NULL)
            {
                // Remove this model from the list
                inputVM->models()->remove(input);
            }
            /*else
            {
                inputVM = getInputFromName(input->name());
                if (inputVM != NULL)
                {
                    // Remove this model from the list
                    inputVM->models().remove(input);
                }
            }*/
        }
    }

    return inputVM;
}


/**
 * @brief A model of output has been added
 * @param output
 * @return a view model of output only if it is a new one
 */
OutputVM* AgentInMappingVM::_outputModelAdded(OutputM* output)
{
    OutputVM* newOutputVM = NULL;

    if (output != NULL)
    {
        // First, we get a ghost of this output: an output without id (only the same name)
        QList<OutputVM*> outputsWithSameName = getOutputsListFromName(output->name());
        OutputVM* outputWithoutId = NULL;

        foreach (OutputVM* iterator, outputsWithSameName)
        {
            // Already a view model with an EMPTY id (NOT defined)
            if ((iterator != NULL) && iterator->id().isEmpty()) {
                outputWithoutId = iterator;
                break;
            }
        }

        // Output id is NOT defined
        if (output->id().isEmpty())
        {
            // There is already a view model without id
            if (outputWithoutId != NULL)
            {
                // Add this new model to the list
                outputWithoutId->models()->append(output);
            }
            // There is not yet a view model without id
            else
            {
                // Create a new view model of output (without id)
                newOutputVM = new OutputVM(output->name(),
                                           "",
                                           output,
                                           this);

                // Don't add to the list here (this output will be added globally via temporary list)

                // Update the hash table with the output name
                outputsWithSameName.append(newOutputVM);
                _mapFromNameToOutputsList.insert(output->name(), outputsWithSameName);
            }
        }
        // Output id is defined
        else
        {
            // There is a view model without id
            if (outputWithoutId != NULL)
            {
                // FIXME TODO: gestion du ghost...passage en view model avec id
            }

            OutputVM* outputVM = getOutputFromId(output->id());

            // There is already a view model for this id
            if (outputVM != NULL)
            {
                // Add this new model to the list
                outputVM->models()->append(output);
            }
            // There is not yet a view model for this id
            else
            {
                // Create a new view model of output
                newOutputVM = new OutputVM(output->name(),
                                           output->id(),
                                           output,
                                           this);

                // Don't add to the list here (this output will be added globally via temporary list)

                // Update the hash table with the output id
                _mapFromUniqueIdToOutput.insert(output->id(), newOutputVM);

                // Update the hash table with the output name
                outputsWithSameName.append(newOutputVM);
                _mapFromNameToOutputsList.insert(output->name(), outputsWithSameName);
            }
        }
    }

    return newOutputVM;
}



/**
 * @brief A model of output has been removed
 * @param output
 * @return
 */
OutputVM* AgentInMappingVM::_outputModelRemoved(OutputM* output)
{
    OutputVM* outputVM = NULL;

    if (output != NULL)
    {
        // Output id is defined
        if (!output->id().isEmpty())
        {
            outputVM = getOutputFromId(output->id());
            if (outputVM != NULL)
            {
                // Remove this model from the list
                outputVM->models()->remove(output);
            }
            /*else
            {
                outputVM = getOutputFromName(output->name());
                if (outputVM != NULL)
                {
                    // Remove this model from the list
                    outputVM->models().remove(output);
                }
            }*/
        }
    }

    return outputVM;
}


/**
 * @brief Update with all models of agents
 */
void AgentInMappingVM::_updateWithAllModels()
{
    bool areIdenticalsAllDefinitions = true;

    if (_models.count() > 1)
    {
        QList<AgentM*> modelsList = _models.toList();

        AgentM* firstModel = modelsList.at(0);
        DefinitionM* firstDefinition = NULL;

        if ((firstModel != NULL) && (firstModel->definition() != NULL))
        {
            firstDefinition = firstModel->definition();

            for (int i = 1; i < modelsList.count(); i++) {
                AgentM* model = modelsList.at(i);

                if ((model != NULL) && (model->definition() != NULL))
                {
                    if (!DefinitionM::areIdenticals(firstDefinition, model->definition())) {
                        areIdenticalsAllDefinitions = false;
                        break;
                    }
                }
            }
        }
    }
    setareIdenticalsAllDefinitions(areIdenticalsAllDefinitions);

    // Update flags in function of models
    _updateIsON();

    // Update the group (of value type) of the reduced map (= brin) in input and in output of our agent
    _updateReducedMapValueTypeGroupInInput();
    _updateReducedMapValueTypeGroupInOutput();
}


/**
 * @brief Update the flag "is ON" in function of flags of models
 */
void AgentInMappingVM::_updateIsON()
{
    bool globalIsON = false;

    foreach (AgentM* model, _models.toList()) {
        if ((model != NULL) && model->isON()) {
            globalIsON = true;
            break;
        }
    }
    setisON(globalIsON);
}


/**
 * @brief Update the group (of value type) of the reduced map (= brin) in input of our agent
 */
void AgentInMappingVM::_updateReducedMapValueTypeGroupInInput()
{
    AgentIOPValueTypeGroups::Value globalReducedMapValueTypeGroupInInput = AgentIOPValueTypeGroups::UNKNOWN;

    for (int i = 0; i < _inputsList.count(); i++)
    {
        InputVM* input = _inputsList.at(i);
        if ((input != NULL) && (input->firstModel() != NULL)) {
            if (i == 0) {
                globalReducedMapValueTypeGroupInInput = input->firstModel()->agentIOPValueTypeGroup();
            }
            else {
                if (globalReducedMapValueTypeGroupInInput != input->firstModel()->agentIOPValueTypeGroup()) {
                    globalReducedMapValueTypeGroupInInput = AgentIOPValueTypeGroups::MIXED;
                    break;
                }
            }
        }
    }
    setreducedMapValueTypeGroupInInput(globalReducedMapValueTypeGroupInInput);
}


/**
 * @brief Update the group (of value type) of the reduced map (= brin) in output of our agent
 */
void AgentInMappingVM::_updateReducedMapValueTypeGroupInOutput()
{
    AgentIOPValueTypeGroups::Value globalReducedMapValueTypeGroupInOutput = AgentIOPValueTypeGroups::UNKNOWN;

    for (int i = 0; i < _outputsList.count(); i++)
    {
        OutputVM* output = _outputsList.at(i);
        if ((output != NULL) && (output->firstModel() != NULL)) {
            if (i == 0) {
                globalReducedMapValueTypeGroupInOutput = output->firstModel()->agentIOPValueTypeGroup();
            }
            else {
                if (globalReducedMapValueTypeGroupInOutput != output->firstModel()->agentIOPValueTypeGroup()) {
                    globalReducedMapValueTypeGroupInOutput = AgentIOPValueTypeGroups::MIXED;
                    break;
                }
            }
        }
    }
    setreducedMapValueTypeGroupInOutput(globalReducedMapValueTypeGroupInOutput);
}
