#include "authmanager.h"

void AuthManager::setAdminMode(bool admin)
{
    if (m_isAdminMode == admin) return;
    m_isAdminMode = admin;
    emit modeChanged(admin);
}

bool AuthManager::enterDeveloperMode(const QString& username, const QString& password)
{
    // Retain the existing local UI credentials; never log or persist input.
    if (username.trimmed() != QStringLiteral("admin") || password != QStringLiteral("xidian")) return false;
    setAdminMode(true);
    return true;
}
