/*
 *	IngeScape Common
 *
 *  Copyright © 2017-2018 Ingenuity i/o. All rights reserved.
 *
 *	See license terms for the rights and conditions
 *	defined by copyright holders.
 *
 *
 *	Contributors:
 *      Alexandre Lemort    <lemort@ingenuity.io>
 *      Vincent Peyruqueou  <peyruqueou@ingenuity.io>
 *
 */

#include "ingescapeutils.h"

#include <QQmlEngine>
#include <QDebug>


// Biggest unique id of action model
static int BIGGEST_UID_OF_ACTION_MODEL = -1;

// Biggest unique id of action in mapping view model
static int BIGGEST_UID_OF_ACTION_IN_MAPPING_VIEW_MODEL = -1;


//--------------------------------------------------------------
//
// IngeScape Utils
//
//--------------------------------------------------------------

/**
 * @brief Default constructor
 * @param parent
 */
IngeScapeUtils::IngeScapeUtils(QObject *parent) : QObject(parent)
{
    // Force ownership of our object, it will prevent Qml from stealing it
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}


/**
 * @brief Destructor
 */
IngeScapeUtils::~IngeScapeUtils()
{
}


/**
  * @brief Create a directory if it does not exist
  * @param directoryPath
  */
void IngeScapeUtils::createDirectoryIfNotExist(QString directoryPath)
{
    // Check if the directory path is not empty
    if (!directoryPath.isEmpty())
    {
        QDir dir(directoryPath);

        // Check if the directory exists
        // NB: It should be useless because a directory can create its parent directories
        //     BUT it allows us to know exactly which directory can not be created and thus
        //     to identify permission issues
        if (!dir.exists())
        {
            if (!dir.mkpath("."))
            {
                qCritical() << "ERROR: could not create directory at '" << directoryPath << "' !";
                qFatal("ERROR: could not create directory");
            }
        }
    }
}


/**
 * @brief Get (and create if needed) the root path of our application
 * "[DocumentsLocation]/IngeScape/"
 * @return
 */
QString IngeScapeUtils::getRootPath()
{
    static QString rootDirectoryPath;

    if (rootDirectoryPath.isEmpty())
    {
        QStringList documentsLocation = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
        if (documentsLocation.count() > 0)
        {
            QString documentsDirectoryPath = documentsLocation.first();

            rootDirectoryPath = QString("%1%2IngeScape%2").arg(documentsDirectoryPath, QDir::separator());

            // Create a directory if it does not exist
            IngeScapeUtils::createDirectoryIfNotExist(rootDirectoryPath);
        }
    }

    return rootDirectoryPath;
}


/**
 * @brief Get (and create if needed) the settings path of our application
 * "[DocumentsLocation]/IngeScape/Settings/"
 * @return
 */
QString IngeScapeUtils::getSettingsPath()
{
    return IngeScapeUtils::_getSubDirectoryPath("settings");
}


/**
 * @brief Get (and create if needed) the snapshots path of our application
 * "[DocumentsLocation]/IngeScape/Snapshots/"
 * @return
 */
QString IngeScapeUtils::getSnapshotsPath()
{
    return IngeScapeUtils::_getSubDirectoryPath("snapshots");
}


/**
 * @brief Get (and create if needed) the path with files about platforms
 * "[DocumentsLocation]/IngeScape/platforms/"
 * @return
 */
QString IngeScapeUtils::getPlatformsPath()
{
    return IngeScapeUtils::_getSubDirectoryPath("platforms");
}


/**
 * @brief Get an UID for a new model of action
 * @return
 */
int IngeScapeUtils::getUIDforNewActionM()
{
    BIGGEST_UID_OF_ACTION_MODEL++;

    return BIGGEST_UID_OF_ACTION_MODEL;
}


/**
 * @brief Free an UID of a model of action
 * @param uid
 */
void IngeScapeUtils::freeUIDofActionM(int uid)
{
    // Decrement only if the uid correspond to the biggest one
    if (uid == BIGGEST_UID_OF_ACTION_MODEL)
    {
        BIGGEST_UID_OF_ACTION_MODEL--;
    }
}


/**
 * @brief Book an UID for a new model of action
 * @param uid
 */
void IngeScapeUtils::bookUIDforActionM(int uid)
{
    if (uid > BIGGEST_UID_OF_ACTION_MODEL) {
        BIGGEST_UID_OF_ACTION_MODEL = uid;
    }
}


/**
 * @brief Get an UID for a new view model of action in mapping
 * @return
 */
int IngeScapeUtils::getUIDforNewActionInMappingVM()
{
    BIGGEST_UID_OF_ACTION_IN_MAPPING_VIEW_MODEL++;

    return BIGGEST_UID_OF_ACTION_IN_MAPPING_VIEW_MODEL;
}


/**
 * @brief Free an UID of a view model of action in mapping
 * @param uid
 */
void IngeScapeUtils::freeUIDofActionInMappingVM(int uid)
{
    // Decrement only if the uid correspond to the biggest one
    if (uid == BIGGEST_UID_OF_ACTION_IN_MAPPING_VIEW_MODEL)
    {
        BIGGEST_UID_OF_ACTION_IN_MAPPING_VIEW_MODEL--;
    }
}


/**
 * @brief Book an UID for a new view model of action in mapping
 * @param uid
 */
void IngeScapeUtils::bookUIDforActionInMappingVM(int uid)
{
    if (uid > BIGGEST_UID_OF_ACTION_IN_MAPPING_VIEW_MODEL) {
        BIGGEST_UID_OF_ACTION_IN_MAPPING_VIEW_MODEL = uid;
    }
}


/**
 * @brief Get (and create if needed) the fullpath of a given sub-directory
 * @param subDirectory
 * @return
 */
QString IngeScapeUtils::_getSubDirectoryPath(QString subDirectory)
{
    QString subDirectoryPath = QString("%1%3%2").arg(IngeScapeUtils::getRootPath(), QDir::separator(), subDirectory);

    // Create this directory if it does not exist
    IngeScapeUtils::createDirectoryIfNotExist(subDirectoryPath);

    return subDirectoryPath;
}
