/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-03-11 11:53:22
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:44
 * @Description: 
 */
/*
 * @Author: wuxiaoxiao
 * @Date: 2026-03-11
 * @Description: 管理者模式认证管理器（单例）
 */
#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H
#include <QObject>
#include <QString>

/**
 * @brief 管理者模式单例
 *
 * 启动固定为用户模式，仅人工验证成功后启用开发者模式。
 * 保留 isAdminMode 命名兼容已有参数窗口，模式变更仅刷新UI权限。
 */
class AuthManager final : public QObject
{
    Q_OBJECT
public:
    static AuthManager& instance()
    {
        static AuthManager inst;
        return inst;
    }

    /** 当前是否处于管理者模式 */
    bool isAdminMode() const { return m_isAdminMode; }

    /** 设置管理者模式 */
    void setAdminMode(bool admin);
    bool enterDeveloperMode(const QString& username, const QString& password);

signals:
    void modeChanged(bool developerMode);

private:
    AuthManager() = default;
    AuthManager(const AuthManager&) = delete;
    AuthManager& operator=(const AuthManager&) = delete;

    bool m_isAdminMode = false;
};

#endif // AUTHMANAGER_H
