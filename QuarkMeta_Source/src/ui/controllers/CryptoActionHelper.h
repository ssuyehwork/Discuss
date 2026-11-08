#pragma once

class QAbstractItemView;

namespace QuarkMeta {

class ContentPanel;

class CryptoActionHelper {
public:
    /**
     * @brief 执行外壳加密保护动作
     */
    static void executeEncrypt(ContentPanel* panel, QAbstractItemView* view);

    /**
     * @brief 执行解除外壳保护动作
     */
    static void executeDecrypt(ContentPanel* panel, QAbstractItemView* view);

    /**
     * @brief 执行修改保护密码动作
     */
    static void executeChangePassword(ContentPanel* panel, QAbstractItemView* view);
};

} // namespace QuarkMeta
