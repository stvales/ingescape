/*
 *	IngeScape Assessments
 *
 *  Copyright © 2019 Ingenuity i/o. All rights reserved.
 *
 *	See license terms for the rights and conditions
 *	defined by copyright holders.
 *
 *
 *	Contributors:
 *      Vincent Peyruqueou <peyruqueou@ingenuity.io>
 *
 */

#ifndef TASKSCONTROLLER_H
#define TASKSCONTROLLER_H

#include <QObject>
#include <I2PropertyHelpers.h>

#include <controller/assessmentsmodelmanager.h>
#include <model/jsonhelper.h>
#include <model/experimentationm.h>


/**
 * @brief The TasksController class defines the controller to manage the tasks of the current experimentation
 */
class TasksController : public QObject
{
    Q_OBJECT

    // List of all types for independent variable value
    I2_ENUM_LISTMODEL(IndependentVariableValueTypes, allIndependentVariableValueTypes)

    // Model of the current experimentation
    I2_QML_PROPERTY_READONLY(ExperimentationM*, currentExperimentation)

    // Model of the selected task
    I2_QML_PROPERTY(TaskM*, selectedTask)


public:

    /**
     * @brief Constructor
     * @param modelManager
     * @param jsonHelper
     * @param parent
     */
    explicit TasksController(AssessmentsModelManager* modelManager,
                             JsonHelper* jsonHelper,
                             QObject *parent = nullptr);


    /**
     * @brief Destructor
     */
    ~TasksController();


    /**
     * @brief Return true if the user can create a task with the name
     * Check if the name is not empty and if a task with the same name does not already exist
     * @param taskName
     * @return
     */
    Q_INVOKABLE bool canCreateTaskWithName(QString taskName);


    /**
     * @brief Create a new task with an IngeScape platform file
     * @param taskName
     * @param platformFilePath
     */
    Q_INVOKABLE void createNewTaskWithIngeScapePlatformFile(QString taskName, QString platformFilePath);


    /**
     * @brief Delete a task
     * @param task
     */
    Q_INVOKABLE void deleteTask(TaskM* task);


    /**
     * @brief Return true if the user can create an independent variable with the name
     * Check if the name is not empty and if a independent variable with the same name does not already exist
     * @param independentVariableName
     * @return
     */
    Q_INVOKABLE bool canCreateIndependentVariableWithName(QString independentVariableName);


    /**
     * @brief Create a new independent variable
     * @param independentVariableName
     * @param independentVariableDescription
     * @param nIndependentVariableValueType
     */
    Q_INVOKABLE void createNewIndependentVariable(QString independentVariableName, QString independentVariableDescription, int nIndependentVariableValueType);


    /**
     * @brief Create a new independent variable of type enum
     * @param independentVariableName
     * @param independentVariableDescription
     * @param enumValues
     */
    Q_INVOKABLE void createNewIndependentVariableEnum(QString independentVariableName, QString independentVariableDescription, QStringList enumValues);


    /**
     * @brief Delete an independent variable
     * @param independentVariable
     */
    Q_INVOKABLE void deleteIndependentVariable(IndependentVariableM* independentVariable);


Q_SIGNALS:


public Q_SLOTS:


private:

    /**
     * @brief Update the list of dependent variables of a task
     * @param task
     */
    void _updateDependentVariablesOfTask(TaskM* task);


private:

    // Manager for the data model of our IngeScape Assessments application
    AssessmentsModelManager* _modelManager;

    // Helper to manage JSON files
    JsonHelper* _jsonHelper;


};

QML_DECLARE_TYPE(TasksController)

#endif // TASKSCONTROLLER_H
