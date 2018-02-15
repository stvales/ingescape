/*
 *  Mastic - QML binding
 *
 *  Copyright (c) 2018 Ingenuity i/o. All rights reserved.
 *
 *  See license terms for the rights and conditions
 *  defined by copyright holders.
 *
 *
 *  Contributors:
 *      Alexandre Lemort <lemort@ingenuity.io>
 *
 */

#include "masticquickinputbinding.h"

#include <QDebug>

#include "masticquickbindingsingleton.h"
#include "MasticQuick.h"



/**
 * @brief Default constructor
 * @param parent
 */
MasticQuickInputBinding::MasticQuickInputBinding(QObject *parent)
    : MasticQuickAbstractIOPBinding(parent)
{
}



/**
 * @brief Destructor
 */
MasticQuickInputBinding::~MasticQuickInputBinding()
{
    // Clear
    clear();
}



//-------------------------------------------------------------------
//
// Custom setters
//
//-------------------------------------------------------------------


/**
 * @brief Set the prefix of Mastic inputs
 * @param value
 */
void MasticQuickInputBinding::setinputsPrefix(QString value)
{
    if (_inputsPrefix != value)
    {
        // Save our new value
        _inputsPrefix = value;

        // Update our component
        update();

        // Notify change
        Q_EMIT inputsPrefixChanged(value);
    }
}



/**
 * @brief Set the suffix of Mastic inputs
 * @param value
 */
void MasticQuickInputBinding::setinputsSuffix(QString value)
{
    if (_inputsSuffix != value)
    {
        // Save our new value
        _inputsSuffix = value;

        // Update our component
        update();

        // Notify change
        Q_EMIT inputsSuffixChanged(value);
    }
}



//-------------------------------------------------------------------
//
// Protected slots
//
//-------------------------------------------------------------------


/**
 * @brief Called when a Mastic input changes
 * @param name
 * @param value
 */
void MasticQuickInputBinding::_onMasticObserveInput(QString name, QVariant value)
{
    // Check if our binding is active
    if (_when)
    {
        // Check if we are interested by this input
        if (_qmlPropertiesByMasticInputName.contains(name))
        {
            QQmlProperty property = _qmlPropertiesByMasticInputName.value(name);
            if (!property.write(value))
            {
                qmlWarning(this) << "failed to update property '" << property.name()
                                 << "' on " << MasticQuickBindingSingleton::prettyObjectTypeName(_target)
                                 << " binded to Mastic input '" << name << "' with value=" << value;
            }
        }
    }
    // Else: our binding is not active
}




//-------------------------------------------------------------------
//
// Protected methods
//
//-------------------------------------------------------------------



/**
 * @brief Connect to MasticQuick
 */
void MasticQuickInputBinding::_connectToMasticQuick()
{
    // Check if we have at least one valid Mastic input
    MasticQuick* masticQuick = MasticQuick::instance();
    if ((masticQuick != NULL) && (_qmlPropertiesByMasticInputName.count() > 0))
    {
        connect(masticQuick, &MasticQuick::observeInput, this, &MasticQuickInputBinding::_onMasticObserveInput, Qt::UniqueConnection);
    }
}



/**
 * @brief Disconnect to MasticQuick
 */
void MasticQuickInputBinding::_disconnectToMasticQuick()
{
    // Check if we have at least one valid Mastic input
    MasticQuick* masticQuick = MasticQuick::instance();
     if ((masticQuick != NULL) && (_qmlPropertiesByMasticInputName.count() > 0))
    {
        disconnect(masticQuick, &MasticQuick::observeInput, this, &MasticQuickInputBinding::_onMasticObserveInput);
    }
}



/**
 * @brief Clear internal data
 */
void MasticQuickInputBinding::_clearInternalData()
{
    // Clear our additional data
    _qmlPropertiesByMasticInputName.clear();
}



/**
 * @brief Update internal data
 */
void MasticQuickInputBinding::_updateInternalData()
{
    // Check if we have at least one valid property
    if (_qmlPropertiesByName.count() > 0)
    {
        MasticQuick* masticQuick = MasticQuick::instance();
        if (masticQuick != NULL)
        {
            // Try to create a Mastic input for each property
            foreach(const QString propertyName, _qmlPropertiesByName.keys())
            {
                // Get our property
                QQmlProperty property = _qmlPropertiesByName.value(propertyName);

                // Name of our MasticInput
                QString masticInputName = _inputsPrefix + propertyName + _inputsSuffix;

                // Get MasticIOP type
                MasticIopType::Value masticIopType = MasticQuickBindingSingleton::getMasticIOPTypeForProperty(property);

                // Try to build a Mastic input
                bool succeeded = false;
                switch(masticIopType)
                {
                    case MasticIopType::INVALID:
                        // Should not happen because we should have filter invalid properties
                        break;

                    case MasticIopType::INTEGER:
                        {
                            // Get our initial value
                            QVariant qmlValue = property.read();
                            bool ok = false;
                            int cValue = qmlValue.toInt(&ok);
                            if (!ok)
                            {
                                cValue = 0;
                                qmlWarning(this) << "invalid value " << qmlValue
                                                 << " to create a Mastic input with type INTEGER";
                            }

                            // Try to create a Mastic input
                            succeeded = masticQuick->createInputInt(masticInputName, cValue);
                        }
                        break;

                    case MasticIopType::DOUBLE:
                        {
                            // Get our initial value
                            QVariant qmlValue = property.read();
                            bool ok = false;
                            double cValue = qmlValue.toDouble(&ok);
                            if (!ok)
                            {
                                cValue = 0.0;
                                qmlWarning(this) << "invalid value " << qmlValue
                                                 << " to create a Mastic input with type DOUBLE";
                            }

                            // Try to create a Mastic input
                            succeeded = masticQuick->createInputDouble(masticInputName, cValue);
                        }
                        break;

                    case MasticIopType::STRING:
                        {
                            // Get our initial value
                            QVariant qmlValue = property.read();

                            // Try to create a Mastic input
                            succeeded = masticQuick->createInputString(masticInputName, qmlValue.toString());
                        }
                        break;

                    case MasticIopType::BOOLEAN:
                        {
                            // Get our initial value
                            QVariant qmlValue = property.read();

                            // Try to create a Mastic input
                            succeeded = masticQuick->createInputBool(masticInputName, qmlValue.toBool());
                        }
                        break;

                    case MasticIopType::IMPULSION:
                        // Should not happen because QML properties can not have the type impulsion
                        break;

                    case MasticIopType::DATA:
                        {
                            succeeded = masticQuick->createInputData(masticInputName, NULL);
                        }
                        break;

                    default:
                        break;
                }


                // Check if we have succeeded
                if (succeeded)
                {
                    _qmlPropertiesByMasticInputName.insert(masticInputName, property);
                }
                else
                {
                    qmlWarning(this) << "failed to create Mastic input '" << masticInputName
                                     << "' with type=" << MasticIopType::staticEnumToString(masticIopType);
                }
            }
        }
    }
    // Else: no valid property => nothing to do
}



