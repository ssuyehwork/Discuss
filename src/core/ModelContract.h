#pragma once
#include <Qt>

namespace QuarkMeta {

/**
 * @brief 全局模型契约角色定义 (ModelContract)
 * 物理统一全应用 Role 定义，彻底解决跨组件 Role 冲突问题。
 */
enum CommonRole {
    // 基础角色 (UserRole + 0..100)
    TypeRole            = Qt::UserRole + 0,  // 类型 (file/folder/category/system/bookmark)
    IdRole              = Qt::UserRole + 1,  // 数据库 ID (分类 ID 等)
    NameRole            = Qt::UserRole + 2,  // 原始名称
    PathRole            = Qt::UserRole + 3,  // 物理路径
    ColorRole           = Qt::UserRole + 4,  // 颜色标记 (Hex)
    RatingRole          = Qt::UserRole + 5,  // 星级评级 (0-5)
    TagsRole            = Qt::UserRole + 6,  // 标签列表 (QStringList)
    
    // 状态角色 (UserRole + 101..200)
    PinnedRole          = Qt::UserRole + 101, // 置顶状态 (快速访问镜像)
    IsLockedRole        = Qt::UserRole + 102, // 锁定/置顶状态 (列表显示)
    EncryptedRole       = Qt::UserRole + 103, // 是否加密
    EncryptHintRole     = Qt::UserRole + 104, // 加密提示
    ManagedRole         = Qt::UserRole + 105, // 是否受控 (已在索引中登记)
    IsEmptyRole         = Qt::UserRole + 106, // 是否为空目录
    CategoryIdRole      = Qt::UserRole + 107, // 所属分类 ID
    CategoryKindRole    = Qt::UserRole + 110, // 分类类型 (0=User, 1=SystemLibrary)
    
    // 扩展 UI 角色 (UserRole + 201..300)
    RegistrationProgressRole = Qt::UserRole + 201, // 异步登记解析状态 (-1=未登记, 0..99=进度, 100=已完成)
    IsGroupHeaderRole   = Qt::UserRole + 202  // 逻辑分组页眉标识
};

} // namespace QuarkMeta
