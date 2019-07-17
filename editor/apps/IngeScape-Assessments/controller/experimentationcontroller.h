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

#ifndef EXPERIMENTATIONCONTROLLER_H
#define EXPERIMENTATIONCONTROLLER_H

#include <QObject>
#include <I2PropertyHelpers.h>
#include <controller/assessmentsmodelmanager.h>
#include <model/jsonhelper.h>
#include <controller/record/recordcontroller.h>
#include <model/experimentationm.h>


/**
 * @brief The ExperimentationController class defines the controller to manage the current experimentation
 */
class ExperimentationController : public QObject
{
    Q_OBJECT

    // Controller to manage a record of the current experimentation
    I2_QML_PROPERTY_READONLY(RecordController*, recordC)

    // Model of the current experimentation
    I2_QML_PROPERTY_READONLY_CUSTOM_SETTER(ExperimentationM*, currentExperimentation)

    // Model of the current experimentation
    I2_QML_PROPERTY_READONLY_CUSTOM_SETTER(ExperimentationsGroupVM*, currentExperimentationGroup)


public:

    /**
     * @brief Constructor
     * @param modelManager
     * @param jsonHelper
     * @param parent
     */
    explicit ExperimentationController(AssessmentsModelManager* modelManager,
                                       JsonHelper* jsonHelper,
                                       QObject *parent = nullptr);


    /**
     * @brief Destructor
     */
    ~ExperimentationController();


    /**
     * @brief Create a new record for a subject and a task
     * @param recordName
     * @param subject
     * @param task
     */
    Q_INVOKABLE void createNewRecordForSubjectAndTask(QString recordName, SubjectM* subject, TaskM* task);


    /**
     * @brief Open a record setup
     * @param record
     */
    Q_INVOKABLE void openRecordSetup(RecordSetupM* recordSetup);


    /**
     * @brief Delete a record setup
     * @param record
     */
    Q_INVOKABLE void deleteRecordSetup(RecordSetupM* recordSetup);
    
Q_SIGNALS:


public Q_SLOTS:


private Q_SLOTS:

    /**
     * @brief Slot called when the current experimentation changed
     * @param currentExperimentation
     */
    void _onCurrentExperimentationChanged(ExperimentationM* currentExperimentation);

protected: // Methods
    /**
     * @brief Retrieve all independent variables from the Cassandra DB for the given task.
     * The task will be updated by this method.
     * @param task
     */
    void _retrieveIndependentVariableForTask(TaskM* task);

    /**
     * @brief Retrieve all dependent variables from the Cassandra DB for the given task.
     * The task will be updated by this method.
     * @param task
     */
    void _retrieveDependentVariableForTask(TaskM* task);

protected: // Attributes
    // Manager for the data model of our IngeScape Assessments application
    AssessmentsModelManager* _modelManager;

    // Helper to manage JSON files
    JsonHelper* _jsonHelper;

};

QML_DECLARE_TYPE(ExperimentationController)

#endif // EXPERIMENTATIONCONTROLLER_H
