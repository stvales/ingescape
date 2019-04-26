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

#ifndef EXPERIMENTATIONRECORDM_H
#define EXPERIMENTATIONRECORDM_H

#include <QObject>
#include <I2PropertyHelpers.h>

#include <model/subject/subjectm.h>
#include <model/task/taskm.h>
#include <model/assessmentsenums.h>


/**
 * @brief The ExperimentationRecordM class defines a model of record
 */
class ExperimentationRecordM : public QObject
{
    Q_OBJECT

    // Unique identifier of our record
    I2_QML_PROPERTY(QString, uid)

    // Name of our record
    I2_QML_PROPERTY(QString, name)

    // Subject of our record
    I2_QML_PROPERTY(SubjectM*, subject)

    // Task of our record
    I2_QML_PROPERTY(TaskM*, task)

    // Start date and time of our record
    I2_QML_PROPERTY(QDateTime, startDateTime)

    // End date and time of our record
    I2_QML_PROPERTY_CUSTOM_SETTER(QDateTime, endDateTime)

    // Duration of our record
    I2_QML_PROPERTY_QTime(duration)
    //I2_QML_PROPERTY(QDateTime, duration)

    // Values of the independent variables of the task
    // "Qml Property Map" allows to set key-value pairs that can be used in QML bindings
    I2_QML_PROPERTY_READONLY(QQmlPropertyMap*, mapIndependentVariableValues)

    // Hash table from a (unique) id of independent variable to the independent variable value
    //I2_QOBJECT_HASHMODEL(QVariant, hashFromIndependentVariableIdToValue)

    // DependentVariableValues (TODO ?): Les valeurs des VD (sorties d’agents)
    // sont stockées avec le temps correspondant au changement de la valeur d’une sortie


public:

    /**
     * @brief Constructor
     * @param uid
     * @param name
     * @param subject
     * @param task
     * @param startDateTime
     * @param parent
     */
    explicit ExperimentationRecordM(QString uid,
                                    QString name,
                                    SubjectM* subject,
                                    TaskM* task,
                                    QDateTime startDateTime,
                                    QObject *parent = nullptr);


    /**
     * @brief Destructor
     */
    ~ExperimentationRecordM();


Q_SIGNALS:


public Q_SLOTS:


private:


};

QML_DECLARE_TYPE(ExperimentationRecordM)

#endif // EXPERIMENTATIONRECORDM_H
