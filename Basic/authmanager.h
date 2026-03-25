/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-03-11 11:53:22
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-03-25 16:20:17
 * @Description: 
 */
/*
 * @Author: wuxiaoxiao
 * @Date: 2026-03-11
 * @Description: 管理者模式认证管理器（单例）
 */
#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

/**
 * @brief 管理者模式单例
 *
 * 在启动时通过登录对话框设置管理者模式状态，
 * 各对话框根据 isAdminMode() 决定是否隐藏高级控件。
 */
class AuthManager
{
public:
    static AuthManager& instance()
    {
        static AuthManager inst;
        return inst;
    }

    /** 当前是否处于管理者模式 */
    bool isAdminMode() const { return m_isAdminMode; }

    /** 设置管理者模式 */
    void setAdminMode(bool admin) { m_isAdminMode = admin; }

private:
    AuthManager() = default;
    AuthManager(const AuthManager&) = delete;
    AuthManager& operator=(const AuthManager&) = delete;

    bool m_isAdminMode = true;  // 默认管理者模式，跳过登录
};

#endif // AUTHMANAGER_H
