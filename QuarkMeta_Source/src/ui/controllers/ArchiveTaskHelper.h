#pragma once

#include <QString>

namespace QuarkMeta {

class ContentPanel;

class ArchiveTaskHelper {
public:
    /**
     * @brief 执行解压压缩包动作
     * @param panel 宿主面板指针
     * @param path 右键点击的目标路径（当面板无多选选中时作为回退路径）
     */
    static void extractArchive(ContentPanel* panel, const QString& path);

    /**
     * @brief 执行打包压缩为 ZIP 动作
     * @param panel 宿主面板指针
     * @param currentPath 当前目录路径
     * @param path 右键点击的目标路径（当面板无多选选中时作为回退路径）
     */
    static void compressToZip(ContentPanel* panel, const QString& currentPath, const QString& path);
};

} // namespace QuarkMeta
