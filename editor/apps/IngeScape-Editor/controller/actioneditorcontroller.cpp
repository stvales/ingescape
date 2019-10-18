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
 *
 */

#include "actioneditorcontroller.h"

#include <QDebug>
#include <misc/ingescapeutils.h>

/**
 * @brief Constructor
 * @param actionName
 * @param originalAction
 * @param allAgentsGroupsByName
 * @param parent
 */
ActionEditorController::ActionEditorController(QString actionName,
                                               ActionM* originalAction,
                                               QList<AgentsGroupedByNameVM*> allAgentsGroupsByName,
                                               bool toDuplicate,
                                               QObject *parent) : QObject(parent),
    _toDuplicate(toDuplicate),
    _originalAction(originalAction),
    _editedAction(nullptr),
    _originalViewModel(nullptr),
    _editedViewModel(nullptr),
    _allAgentsGroupsByName(allAgentsGroupsByName)
{
    // Force ownership of our object, it will prevent Qml from stealing it
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

    // Get an UID for our new model of action
    int uid = IngeScapeUtils::getUIDforNewActionM();

    _editedAction = new ActionM(uid, actionName);

    if (_originalAction != nullptr)
    {
        _editedAction->copyFrom(_originalAction);

        if (toDuplicate) {
            // Keep uid & name
            _editedAction->setuid(uid);
            _editedAction->setname(actionName);
            // No more original action
            _originalAction = nullptr;
        }
        else { // Cancel the increment because the edited action uses now (after the call to "copyFrom") the uid of the original action
            // Free the UID of the action model
            IngeScapeUtils::freeUIDofActionM(uid);
        }
    }
}


/**
 * @brief Destructor
 */
ActionEditorController::~ActionEditorController()
{
    _allAgentsGroupsByName.clear();

    setoriginalAction(nullptr);

    if (_editedAction != nullptr)
    {
        ActionM* tmp = _editedAction;
        seteditedAction(nullptr);
        delete tmp;
    }

    setoriginalViewModel(nullptr);

    if (_editedViewModel != nullptr)
    {
        ActionVM* tmp = _editedViewModel;
        seteditedViewModel(nullptr);
        delete tmp;
    }
}


/**
 * @brief Valide the edition/creation
 */
void ActionEditorController::validateModification()
{
    // Save action model changes
    if (_originalAction == nullptr)
    {
        setoriginalAction(_editedAction);
        seteditedAction(nullptr);
    }
    else {
        _originalAction->copyFrom(_editedAction);
    }

    // Save action view model changes if it's an action editor from a view model
    if ((_editedViewModel != nullptr) && (_originalViewModel != nullptr))
    {
        _originalViewModel->setcolor(_editedViewModel->color());
        _originalViewModel->setstartTimeString(_editedViewModel->startTimeString());
    }

    // If the model is connected we reinitialize the conditions
    if (_originalAction->isConnected())
    {
        // Reset connections
        _originalAction->resetConditionsConnections();

        // Init connections
        _originalAction->initializeConditionsConnections();
    }
}


/**
 * @brief Create a new condition
 */
void ActionEditorController::createNewCondition()
{
    ActionConditionVM* conditionVM = new ActionConditionVM();

    // Set a condition model (by default: condition on value)
    conditionVM->setmodelM(new IOPValueConditionM());
    //conditionVM->setmodelM(new ConditionOnAgentM());

    // List of agents is NOT empty
    if (!_allAgentsGroupsByName.isEmpty()) {
        conditionVM->modelM()->setagent(_allAgentsGroupsByName.at(0));
    }

    _editedAction->addConditionToList(conditionVM);

    // Set condition VM type (by default: condition on value)
    conditionVM->setconditionType(ActionConditionTypes::VALUE);
    //conditionVM->setconditionType(ActionConditionTypes::AGENT);
}


/**
 * @brief Remove the conditionVM
 */
void ActionEditorController::removeCondition(ActionConditionVM* conditionVM)
{
    // Remove the condition
    if (_editedAction->conditionsList()->contains(conditionVM)) {
        _editedAction->conditionsList()->remove(conditionVM);
    }
}


/**
 * @brief Create a new effectVM
 */
void ActionEditorController::createNewEffect()
{
    ActionEffectVM* effectVM = new ActionEffectVM();

    // Set an effect model
    effectVM->setmodelM(new IOPValueEffectM());

    // List of agents is NOT empty
    if (!_allAgentsGroupsByName.isEmpty())
    {
        effectVM->modelM()->setagent(_allAgentsGroupsByName.at(0));

        if (_allAgentsGroupsByName.count() > 1) {
            effectVM->setsecondAgentForMapping(_allAgentsGroupsByName.at(1));
        }
    }

    _editedAction->addEffectToList(effectVM);

    // Set effect VM type
    effectVM->seteffectType(ActionEffectTypes::VALUE);
}


/**
 * @brief Remove the effect
 */
void ActionEditorController::removeEffect(ActionEffectVM* effectVM)
{
    if (_editedAction->effectsList()->contains(effectVM)) {
        _editedAction->effectsList()->remove(effectVM);
    }
}

