/*
 *	IngeScape Editor
 *
 *  Copyright © 2017-2019 Ingenuity i/o. All rights reserved.
 *
 *	See license terms for the rights and conditions
 *	defined by copyright holders.
 *
 *
 *	Contributors:
 *      Vincent Peyruqueou <peyruqueou@ingenuity.io>
 *
 */

#include "licensescontroller.h"
#include <misc/ingescapeutils.h>
#include <settings/ingescapesettings.h>
#include <QFileDialog>

extern "C" {
#include <ingescape.h>
//#include <ingescape_advanced.h>
#include <ingescape_private.h>
}


/**
 * @brief onLicenseCallback
 * @param limit
 * @param myData
 */
void onLicenseCallback(igs_license_limit_t limit, void *myData)
{
    LicensesController* licensesController = static_cast<LicensesController*>(myData);
    if (licensesController != nullptr)
    {
        switch (limit)
        {
        case IGS_LICENSE_TIMEOUT:
            qCritical("IngeScape is stopped because demonstration mode timeout has been reached");
            licensesController->seterrorMessageWhenLicenseFailed("IngeScape is stopped because demonstration mode timeout has been reached !");
            break;

        case IGS_LICENSE_TOO_MANY_AGENTS:
            qCritical("IngeScape is stopped because too many agents are running on the platform compared to what the license allows !");
            licensesController->seterrorMessageWhenLicenseFailed("IngeScape is stopped because too many agents are running on the platform compared to what the license allows !");
            break;

        case IGS_LICENSE_TOO_MANY_IOPS:
            qCritical("IngeScape is stopped because too many IOPs have been created on the platform compared to what the license allows");
            licensesController->seterrorMessageWhenLicenseFailed("IngeScape is stopped because too many IOPs have been created on the platform compared to what the license allows !");
            break;

        default:
            break;
        }

        // Update flags
        licensesController->setisLicenseValid(false);
        //licensesController->setisEditorLicenseValid(false);
    }
}


//--------------------------------------------------------------
//
// Licenses Controller
//
//--------------------------------------------------------------

/**
 * @brief Constructor
 * @param parent
 */
LicensesController::LicensesController(QObject *parent) : QObject(parent),
    _licensesPath(""),
    _errorMessageWhenLicenseFailed(""),
    _isLicenseValid(false),
    _licenseId(""),
    _licenseCustomer(""),
    _licenseOrder(""),
    _licenseExpirationDate(QDate()),
    _isEditorLicenseValid(false),
    _editorOwner(""),
    _editorExpirationDate(QDate()),
    _maxNbOfAgents(0),
    _maxNbOfIOPs(0),
    _featureNames(QStringList()),
    _agentNames(QStringList()),
    _mergedLicense(nullptr)
{
    // Force ownership of our object, it will prevent Qml from stealing it
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);


    //
    // Settings
    //
    IngeScapeSettings &settings = IngeScapeSettings::Instance();

    settings.beginGroup("licenses");

    // Get the default path for "Licenses"
    QString defaultLicensesPath = IngeScapeUtils::getLicensesPath();

    _licensesPath = settings.value("directoryPath", QVariant(defaultLicensesPath)).toString();

    settings.endGroup();

    qInfo() << "New Licenses Controller with licenses path" << _licensesPath;

    // Begin to observe license events (events are triggered only when no valid license is available)
    igs_observeLicense(onLicenseCallback, this);

    // Set the IngeScape license path
    igs_setLicensePath(_licensesPath.toStdString().c_str());

    // Get the data about licenses
    _getLicensesData();

}


/**
 * @brief Destructor
 */
LicensesController::~LicensesController()
{
    qInfo() << "Delete Licenses Controller";

    // Clean-up merged license
    if (_mergedLicense != nullptr)
    {
        LicenseInformationM* temp = _mergedLicense;
        setmergedLicense(nullptr);
        delete temp;
    }

    // Clean-up license details
    _licenseDetailsList.deleteAllItems();
}


/**
 * @brief Select a directory with IngeScape licenses
 * @return
 */
QString LicensesController::selectLicensesDirectory()
{
    // Open a directory dialog box
    return QFileDialog::getExistingDirectory(nullptr,
                                             "Select a directory with IngeScape licenses",
                                             _licensesPath);
}


/**
 * @brief Update the licenses path
 * @param newLicensesPath
 */
void LicensesController::updateLicensesPath(QString newLicensesPath)
{
    // Licenses path has been changed
    if (newLicensesPath != _licensesPath)
    {
        qInfo() << "Licenses path changes from" << _licensesPath << "to" << newLicensesPath;

        // Update property
        setlicensesPath(newLicensesPath);

        // Update settings file
        IngeScapeSettings &settings = IngeScapeSettings::Instance();
        settings.beginGroup("licenses");
        settings.setValue("directoryPath", _licensesPath);
        settings.endGroup();

        // Save new value
        settings.sync();

        // Set the IngeScape license path
        igs_setLicensePath(_licensesPath.toStdString().c_str());

        // Get the data about licenses
        _getLicensesData();

        // Emit the signal "Licenses Updated"
        Q_EMIT licensesUpdated();
    }
}


/**
 * @brief Delete the given license from the platform and from the filesystem
 * @param licenseInformation
 * @return
 */
bool LicensesController::deleteLicense(LicenseInformationM* licenseInformation)
{
    QDir licenseDirectory(_licensesPath);
    if (licenseDirectory.exists() && licenseDirectory.remove(licenseInformation->fileName()))
    {
        // Success
        _getLicensesData();
        return true;
    }

    // Failure
    qDebug() << "Unable to delete the license file.";
    return false;
}


/**
 * @brief Copy the license file from the given path the the current license directory
 * then refresh the global license information
 * @param licenseFilePath
 * @return
 */
bool LicensesController::addLicenses(const QList<QUrl>& licenseUrlList)
{
    bool completeSuccess = true;

    // Are all URLs corresponding to local files ?
    if (std::all_of(licenseUrlList.begin(), licenseUrlList.end(), [](const QUrl& url){ return url.isLocalFile(); }))
    {
        for (QUrl licenseUrl : licenseUrlList)
        {
            QFileInfo licenseFile(licenseUrl.toLocalFile());
            if (licenseFile.exists())
            {
                bool success = QFile::copy(licenseFile.absoluteFilePath(), QDir(_licensesPath).filePath(licenseFile.fileName()));
                if (!success)
                {
                    qDebug() << "Unable to copy" << licenseFile.fileName() << "to the license directory";
                }
                completeSuccess &= success;
            }
        }

        _getLicensesData();
    }
    else
    {
        completeSuccess = false;
    }

    return completeSuccess;
}


/**
 * @brief Get the data about licenses
 */
void LicensesController::_getLicensesData()
{
    // Allows to update data about licenses
    igs_checkLicenseForAgent(nullptr);

    if (license != nullptr)
    {
        if (_mergedLicense != nullptr)
        {
            LicenseInformationM* temp = _mergedLicense;
            setmergedLicense(nullptr);
            delete temp;
        }

        setmergedLicense(new LicenseInformationM(license));
        qDebug() << "License information:";
        qDebug() << *_mergedLicense;

        QDateTime licenseExpirationDateTime = QDateTime::fromSecsSinceEpoch(license->licenseExpirationDate);
        setlicenseExpirationDate(licenseExpirationDateTime.date());

        QDateTime editorExpirationDateTime = QDateTime::fromSecsSinceEpoch(license->editorExpirationDate);
        seteditorExpirationDate(editorExpirationDateTime.date());

        setmaxNbOfAgents(license->platformNbAgents);
        setmaxNbOfIOPs(license->platformNbIOPs);

        if (license->isLicenseValid)
        {
            setlicenseId(license->id);
            setlicenseCustomer(license->customer);
            setlicenseOrder(license->order);
            setisLicenseValid(true);

            qInfo() << "VALID License: id" << _licenseId << "order" << _licenseOrder << "customer" << _licenseCustomer << "licenseExpirationDate" << _licenseExpirationDate;
        }
        else
        {
            setlicenseId("");
            setlicenseCustomer("");
            setlicenseOrder("");
            setisLicenseValid(false);

            qInfo() << "IN-valid License: id" << QString(license->id) << "order" << QString(license->order) << "customer" << QString(license->customer) << "licenseExpirationDate" << _licenseExpirationDate;
        }

        if (license->isEditorLicenseValid)
        {
            seteditorOwner(license->editorOwner);
            setisEditorLicenseValid(true);

            qInfo() << "VALID EDITOR License: editorOwner" << _editorOwner << "editorExpirationDate" << _editorExpirationDate;
        }
        else
        {
            seteditorOwner("");
            setisEditorLicenseValid(false);

            qInfo() << "IN-valid EDITOR License: editorOwner" << QString(license->editorOwner) << "editorExpirationDate" << _editorExpirationDate;
        }

        qDebug() << "Nb MAX Agents" << _maxNbOfAgents << "Nb MAX IOPs" << _maxNbOfIOPs;


        //
        // Features
        //
        QStringList featureNamesTemp = QStringList();

        if ((license->features != nullptr) && (zhash_size(license->features) > 0))
        {
            zlist_t *keys = zhash_keys(license->features);
            if (keys != nullptr)
            {
                char* key = static_cast<char*>(zlist_first(keys));
                while (key != nullptr)
                {
                    featureNamesTemp.append(QString(key));
                    key = static_cast<char*>(zlist_next(keys));
                }
            }
            zlist_destroy(&keys);
        }
        setfeatureNames(featureNamesTemp);
        qInfo() << "Features" << _featureNames;


        //
        // Licenses for agents
        //
        QStringList agentNamesTemp = QStringList();

        if ((license->agents != nullptr) && (zhash_size(license->agents) > 0))
        {
            licenseForAgent_t* agent = static_cast<licenseForAgent_t *>(zhash_first(license->agents));
            while (agent != nullptr)
            {
                agentNamesTemp.append(QString(agent->agentName));
                agent = static_cast<licenseForAgent_t *>(zhash_next(license->agents));
            }
        }
        setagentNames(agentNamesTemp);
        qInfo() << "Agents" << _agentNames;

        _licenseDetailsList.deleteAllItems();

        //
        // License details
        //
        if ((license->licenseDetails != nullptr) && (zlist_size(license->licenseDetails) > 0))
        {
            qDebug() << zlist_size(license->licenseDetails) << "license details";

            license_t* detail = static_cast<license_t*>(zlist_first(license->licenseDetails));
            while (detail != nullptr)
            {
                LicenseInformationM* licenseDetails = new LicenseInformationM(detail);
                _licenseDetailsList.append(licenseDetails);
                qDebug() << *licenseDetails;
                detail = static_cast<license_t*>(zlist_next(license->licenseDetails));
            }
        }
    }
}
