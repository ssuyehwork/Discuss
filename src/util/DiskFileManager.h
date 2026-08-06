#pragma once

#include <vector>
#include <string>
#include <QString>

namespace ArcMeta {

class DiskFileManager {
public:
    /**
     * @brief 执行物理磁盘文件批量重命名/移动/复制，并同步处理元数据索引
     * @return 实际成功处理的文件数量
     */
    static int batchRenameDiskFiles(const std::vector<std::wstring>& originalPaths,
                                    const std::vector<std::wstring>& newNames,
                                    const QString& targetDir,
                                    bool isCopy,
                                    bool isMove);
};

} // namespace ArcMeta
