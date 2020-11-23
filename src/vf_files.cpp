#include "vf_files.h"
#include "dirwatcherentry.h"
#include "mountwatcherentry.h"
#include "defaultpathentry.h"
#include <QStorageInfo>

using namespace vfFiles;

vf_files::vf_files(QObject *parent, int id) : QObject(parent),
    m_entity(new VfCpp::veinmoduleentity(id)),
    m_dataSizeflags(QLocale::DataSizeSIFormat),
    m_isInitalized(false)
{
}

bool vf_files::initOnce()
{
    // create our entity once
    if(!m_isInitalized) {
        m_isInitalized = true;
        m_entity->initModule();
        m_entity->createComponent("EntityName", "_Files", true);
        m_entity->createRpc(this, "RPC_CopyFile", VfCpp::cVeinModuleRpc::Param({{"p_source", "QString"},{"p_dest", "QString"},{"p_overwrite", "bool"}}));
        m_entity->createRpc(this, "RPC_GetDriveInfo",
                            VfCpp::cVeinModuleRpc::Param({
                                                             {"p_mountDir", "QString"},
                                                             {"p_localeName", "QString"},
                                                             {"p_getDriveName", "bool"},
                                                             {"p_getMemTotal", "bool"},
                                                             {"p_getMemFree", "bool"}}));
    }
    return true;
}

void appendErrorMsg(QString& errorMsg, const QString appendError)
{
    if(!errorMsg.isEmpty()) {
        errorMsg += QStringLiteral(" / ");
    }
    errorMsg += appendError;
}

QVariant vf_files::RPC_CopyFile(QVariantMap p_params)
{
    QString strError;

    QString sourceFile = p_params["p_source"].toString();
    QString destFile = p_params["p_dest"].toString();
    bool p_overwrite = p_params["p_overwrite"].toBool();

    bool bSourceExists = QFile::exists(sourceFile);
    bool bDestExists = QFile::exists(destFile);

    // check some obvious plausis first
    if(!bSourceExists) {
        appendErrorMsg(strError, sourceFile + QStringLiteral(" does not exist"));
    }
    if(!p_overwrite && bDestExists) {
        appendErrorMsg(strError, QStringLiteral("Cannot overwrite ") + destFile);
    }

    // ensure dest directory is there
    if(strError.isEmpty()) {
        QFileInfo destFileInfo(destFile);
        QDir destDir = destFileInfo.absoluteDir();
        if(!destDir.mkpath(destDir.path())) {
            appendErrorMsg(strError, QStringLiteral("Cannot create directory ") + destDir.path());
        }
    }
    // do copy
    if(strError.isEmpty()) {
        QFile file(sourceFile);
        // we checked plausis above / copy cannot overwrite
        if(bDestExists) {
            file.remove(destFile);
        }
        if(!file.copy(destFile)) {
            appendErrorMsg(strError, sourceFile + QStringLiteral(" was not copied to ") + destFile);
        }
    }
    bool ok = strError.isEmpty();
    if(!ok) {
        // drop a note to whoever listens (usually journal)
        qWarning("%s", qPrintable(strError));
    }
    return ok;
}

QVariant vf_files::RPC_GetDriveInfo(QVariantMap p_params)
{
    QString strError;

    QString mountDir = p_params["p_mountDir"].toString();
    QString localeName = p_params["p_localeName"].toString();
    bool getDriveName = p_params["p_getDriveName"].toBool();
    bool getMemTotal = p_params["p_getMemTotal"].toBool();
    bool getMemFree = p_params["p_getMemFree"].toBool();

    bool mountExists = QFile::exists(mountDir);
    // check some obvious plausis first
    if(!mountExists) {
        appendErrorMsg(strError, mountDir + QStringLiteral(" does not exist"));
    }

    QStringList listMountInfo;
    if(strError.isEmpty()) {
        QStorageInfo storageInfo(mountDir);
        if(storageInfo.isValid()) {
            if(getDriveName) {
                listMountInfo.append(QStringLiteral("name:") + storageInfo.name());
            }
            if(getMemTotal || getMemFree) {
                QLocale locale(localeName);
                qint64 bytes;
                if(getMemTotal) {
                    bytes = storageInfo.bytesTotal();
                    listMountInfo.append(QStringLiteral("total:") + locale.formattedDataSize(bytes, 2, m_dataSizeflags));
                }
                if(getMemFree) {
                    bytes = storageInfo.bytesFree();
                    listMountInfo.append(QStringLiteral("free:") + locale.formattedDataSize(bytes, 2, m_dataSizeflags));
                }
            }
        }
        else {
            appendErrorMsg(strError, QStringLiteral("Could not create storage info for ") + mountDir);
        }
    }
    return listMountInfo;
}

bool vf_files::addDirToWatch(const QString componentName,
                             const QString dir,
                             const QStringList nameFilters,
                             QDir::Filters filters,
                             bool fullPathForResults)
{
    bool ok = false;
    if(m_isInitalized) { // prerequisites ok?
        // try to add another watcher component
        DirWatcherEntry* dirWatcherEntry = new DirWatcherEntry(this);
        if(dirWatcherEntry->create(m_entity, componentName, dir, nameFilters, filters, fullPathForResults)) {
            ok = true;
        }
        else {
            delete dirWatcherEntry;
        }
    }
    return ok;
}

bool vf_files::addMountToWatch(const QString componentName, const QString mountBasePath, const QString procFileMount)
{
    bool ok = false;
    if(m_isInitalized) { // prerequisites ok?
        // try to add another watcher component
        MountWatcherEntry* mountWatcherEntry = new MountWatcherEntry(this);
        if(mountWatcherEntry->create(m_entity, componentName, procFileMount, mountBasePath)) {
            ok = true;
        }
        else {
            delete mountWatcherEntry;
        }
    }
    return ok;
}

bool vf_files::addDefaultPathComponent(const QString componentName, const QString dir, bool createDir)
{
    bool ok = false;
    if(m_isInitalized) { // prerequisites ok?
        // try to add another default path component
        DefaultPathEntry* defaultPathEntry = new DefaultPathEntry(this);
        if(defaultPathEntry->create(m_entity, componentName, dir, createDir)) {
            ok = true;
        }
        else {
            delete defaultPathEntry;
        }
    }
    return ok;
}

void vf_files::setDataSizeFormat(QLocale::DataSizeFormats dataSizeflags)
{
    m_dataSizeflags = dataSizeflags;
}

VfCpp::veinmoduleentity *vf_files::getVeinEntity() const
{
    return m_entity;
}
