#include "defaultpathentry.h"
#include <QDir>

DefaultPathEntry::DefaultPathEntry(QObject *parent) : QObject(parent)
{
}

bool DefaultPathEntry::create(VfCpp::VeinModuleEntity *entity, const QString componentName, const QString path, const bool create)
{
    bool ok = true;
    // first try to create if we are asked for
    if(create) {
        // same return if path exists ot not
        QDir dir;
        ok = dir.mkpath(path);
    }
    if(ok) {
        // to be consistent and to make consumers' life easier: append trailing separator
        QString setPath = path;
        if(!setPath.endsWith(QDir::separator())) {
            setPath.append(QDir::separator());
        }

        m_veinComponent = entity->createComponent(componentName, path, VfCpp::cVeinModuleComponent::Direction::out);
    }
    return ok;
}
