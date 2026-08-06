# 批量重命名核心逻辑下沉与辅件同步精确重构方案 —— Modification_Plan-43.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
本方案承接自 `Modification_Plan-42.md`，用于彻底消除关于“双轨缩略图物理位置认知不一致”造成的逻辑缺陷：
1. **缩略图双轨存放逻辑存在本质差异**：
   - **内存模式**：主资产和配套缩略图存放在同一个 `.arc` 逻辑胶囊内部，执行重命名时**必须**对该胶囊内部的 `_thumbnail.png` 或 `[baseName]_thumbnail.png` 配套物理缩略图一并执行重命名，否则资产将彻底丢失缩略图回显。
   - **磁盘模式**：提取生成的缩略图统一保存在主程序运行目录下的全局 `.arcmeta/disk_thumbs/` 中，以路径/修改时间/大小的 `qHash` 散列值命名。用户物理磁盘的资产同级目录下根本不存在任何 `_thumbnail.png` 文件。因此，磁盘重命名执行物理 I/O 时，**只对主资产执行复制/移动/更名，绝不去搜索、不移动、不删除原资产同级任何不存在的 _thumbnail 辅件**，保持磁盘操作的高纯度、高安全性。

为了实现极致严谨的批量更名逻辑：
1. **业务逻辑下沉，UI 禁做物理 I/O**（对应用户原话：“业务逻辑下沉，UI 对话框彻底禁做文件 I/O”）：
   - 在 `MetadataManager` 中声明并实现 `batchRenameMemoryAssets(...)`，原子化穿透 `.arc` 并对胶囊内部资产和缩略图进行重命名（支持 `_thumbnail.png` 与 `[baseName]_thumbnail.png` 双重兼容）；
   - 在 `DiskFileManager` 服务类中声明并实现 `batchRenameDiskFiles(...)`，负责处理非托管磁盘模式下的重命名、移动和复制物理文件及元数据。
2. **依据实际成功数精确计数**（对应用户原话：“依据‘实际成功数’精确递增序列号”）：
   - 后端更名方法直接返回真实成功的物理文件数 `actualSuccessCount`；
   - UI 界面据此自增起始序列值（`start += actualSuccessCount * step`），确保不发生断号。
3. **UI 选项及布局完全阻断**（对应用户原话：“彻底禁用选项 UI”）：
   - 当 `m_isMirrorSource == true` 时，禁选 `m_rbMove` 与 `m_rbCopy`，并将 `m_targetPathEdit`（路径输入框）与 `m_btnBrowse`（浏览按钮）进行 `hide()` 彻底隐藏。

## 2. 问题定位
- 批量更名文件 I/O 散落于 UI 层的 `BatchRenameDialog`。
- 磁盘模式由于在物理原盘去搜索并不存在的 `_thumbnail.png` 辅件导致产生不必要的逻辑冗余和误触风险。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 业务逻辑下沉，UI 对话框彻底禁做文件 I/O | UI 层一律不准调用 QFile 的物理复制、移动、更名操作，全部下沉至 `MetadataManager` 和 `DiskFileManager` 后端实现 | ✅ 一致 |
| 2    | 依据“实际成功数”精确递增序列号 | 后端返回实际成功的数量，UI 再进行 `start = start + actualSuccessCount * step` 累加并 `doAutoSave` 自动保存 | ✅ 一致 |
| 3    | 彻底禁用选项 UI 并隐藏选择框 | 内存模式下直接 `hide()` 隐藏目标文件夹选择框和浏览按钮，并 `setEnabled(false)` 禁用移动和复制单选框 | ✅ 一致 |
| 4    | 内存模式下精准穿透 .arc 胶囊文件夹，同时原子化重命名内部主资产与 _thumbnail.png | 在 `batchRenameMemoryAssets` 中对旧主资产及其胶囊内 `_thumbnail.png` 以及 `[baseName]_thumbnail.png` 缩略图一并执行重命名 | ✅ 一致 |
| 5    | 磁盘模式不盲目操作同级不存在的缩略图辅件 | 在 `DiskFileManager::batchRenameDiskFiles` 中只对主资产执行物理 I/O，绝不去物理原盘目录中搜寻或移动缩略图文件，保持普通盘清洁 | ✅ 一致 |

## 4. 详细解决方案

本部分由执行者 AI 角色直接读取，并严格、机械地按照给出的 Git merge diff 代码块进行物理替换，不得做任何自由发挥或脑补改动。

### 4.1 修改 `CMakeLists.txt`
将新增的 `src/util/DiskFileManager.h` 与 `src/util/DiskFileManager.cpp` 注册进项目。

```
<<<<<<< SEARCH
    src/util/AssetImporter.cpp
    src/util/AssetImporter.h
    ArcMeta.rc
=======
    src/util/AssetImporter.cpp
    src/util/AssetImporter.h
    src/util/DiskFileManager.cpp
    src/util/DiskFileManager.h
    ArcMeta.rc
>>>>>>> REPLACE
```

### 4.2 创建 `src/util/DiskFileManager.h` (新增文件)
声明磁盘导航模式下的物理批量更名服务。

```cpp
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
```

### 4.3 创建 `src/util/DiskFileManager.cpp` (新增文件)
实现磁盘批量操作：只对物理磁盘的主资产执行物理复制、移动和更名，绝对不碰任何原盘同级的 `.png` 辅件（因其根本不存在），保持原盘绝对纯净。

```cpp
#include "DiskFileManager.h"
#include "../meta/MetadataManager.h"
#include "../meta/CategoryRepo.h"
#include <QFileInfo>
#include <QDir>
#include <QFile>

namespace ArcMeta {

int DiskFileManager::batchRenameDiskFiles(const std::vector<std::wstring>& originalPaths,
                                         const std::vector<std::wstring>& newNames,
                                         const QString& targetDir,
                                         bool isCopy,
                                         bool isMove) {
    int successCount = 0;

    // 🚨 开启防抖与内部操作锁定
    MetadataManager::instance().beginInternalOperation();

    for (int i = 0; i < (int)originalPaths.size(); ++i) {
        QString oldPath = QString::fromStdWString(originalPaths[i]);
        QFileInfo oldInfo(oldPath);
        QString finalTargetDir = (!isCopy && !isMove) ? oldInfo.absolutePath() : targetDir;
        QString newPathStr = QDir(finalTargetDir).filePath(QString::fromStdWString(newNames[i]));

        bool ok = false;
        if (isCopy) {
            ok = QFile::copy(oldPath, newPathStr);
        } else if (isMove) {
            if (QFile::copy(oldPath, newPathStr)) {
                ok = QFile::remove(oldPath);
            }
        } else {
            ok = QFile::rename(oldPath, newPathStr);
        }

        if (ok) {
            successCount++;
            if (!isCopy) {
                std::wstring oldW = oldInfo.absoluteFilePath().toStdWString();
                std::wstring newW = QDir(finalTargetDir).absoluteFilePath(QString::fromStdWString(newNames[i])).toStdWString();

                // 同步进行磁盘离散元数据、哈希 JSON 的重命名与平滑迁移
                MetadataManager::instance().renameItem(oldW, newW);
                CategoryRepo::renamePhysicalCategoryPath(oldW, newW);
            }
        }
    }

    // 🚨 关闭内部操作锁定并提交
    MetadataManager::instance().endInternalOperation();

    // 发射全量 UI 刷新信号
    MetadataManager::instance().notifyFullUIRebuild();

    return successCount;
}

} // namespace ArcMeta
```

### 4.4 修改 `src/meta/MetadataManager.h`
声明逻辑层内存模式资产原子批量更名接口。

```
<<<<<<< SEARCH
    void renameItem(const std::wstring& oldPath, const std::wstring& newPath);
    void removeMetadataSync(const std::wstring& path);
=======
    void renameItem(const std::wstring& oldPath, const std::wstring& newPath);
    int batchRenameMemoryAssets(const std::vector<std::wstring>& originalPaths, const std::vector<std::wstring>& newNames);
    void removeMetadataSync(const std::wstring& path);
>>>>>>> REPLACE
```

### 4.5 修改 `src/meta/MetadataManager.cpp`
实现 `batchRenameMemoryAssets(...)`，穿透 `.arc` 胶囊并对胶囊内部的主资产及缩略图文件（双重兼容 `_thumbnail.png` 和 `[baseName]_thumbnail.png`）同步物理重命名。

```
<<<<<<< SEARCH
void MetadataManager::syncAfterMove(const std::wstring& oldPath, const std::wstring& newPath) {
    std::wstring nOld = normalizePath(oldPath);
=======
int MetadataManager::batchRenameMemoryAssets(const std::vector<std::wstring>& originalPaths, const std::vector<std::wstring>& newNames) {
    int successCount = 0;

    // 🚨 开启防抖与内部操作锁定
    beginInternalOperation();

    for (int i = 0; i < (int)originalPaths.size(); ++i) {
        QString oldPath = QString::fromStdWString(originalPaths[i]);
        QFileInfo oldInfo(oldPath);
        QString finalTargetDir = oldInfo.absolutePath();
        QString newPathStr = QDir(finalTargetDir).filePath(QString::fromStdWString(newNames[i]));

        if (QFile::rename(oldPath, newPathStr)) {
            successCount++;

            // 同步对配套 _thumbnail.png 缩略图进行物理重命名 (支持 [baseName]_thumbnail.png 与 _thumbnail.png 双重兼容)
            QString oldThumbBase = oldInfo.absolutePath() + "/" + oldInfo.completeBaseName() + "_thumbnail.png";
            QString oldThumbFixed = oldInfo.absolutePath() + "/_thumbnail.png";

            if (QFile::exists(oldThumbBase)) {
                QString newThumbBase = QFileInfo(newPathStr).absolutePath() + "/" + QFileInfo(newPathStr).completeBaseName() + "_thumbnail.png";
                QFile::rename(oldThumbBase, newThumbBase);
            }
            if (QFile::exists(oldThumbFixed)) {
                QString newThumbFixed = QFileInfo(newPathStr).absolutePath() + "/_thumbnail.png";
                if (oldThumbFixed != newThumbFixed) {
                    QFile::rename(oldThumbFixed, newThumbFixed);
                }
            }

            std::wstring oldW = oldInfo.absoluteFilePath().toStdWString();
            std::wstring newW = QDir(finalTargetDir).absoluteFilePath(QString::fromStdWString(newNames[i])).toStdWString();

            // 1. 内存模型下的元数据索引及路径迁移
            renameItem(oldW, newW);

            // 2. 双轨制同步：更新分类关系与 pathHint 映射
            CategoryRepo::renamePhysicalCategoryPath(oldW, newW);
        }
    }

    // 🚨 关闭内部操作锁定并提交
    endInternalOperation();

    // 发射全量 UI 刷新信号
    notifyFullUIRebuild();

    return successCount;
}

void MetadataManager::syncAfterMove(const std::wstring& oldPath, const std::wstring& newPath) {
    std::wstring nOld = normalizePath(oldPath);
>>>>>>> REPLACE
```

### 4.6 修改 `src/ui/BatchRenameDialog.h`
更新构造函数，接收 `isMirrorSource` 参数并保存为成员变量。

```
<<<<<<< SEARCH
    explicit BatchRenameDialog(const std::vector<std::wstring>& originalPaths, QWidget* parent = nullptr);
    ~BatchRenameDialog() override = default;

    QString getFirstNewName() const { return m_firstNewName; }

private slots:
    void onAddRow();
    void updatePreview();
    void onExecute();
    void onPreview();
    void onBrowseTarget();
    void onImportPreset();
    void onExportPreset();
    void onDeleteCurrentPreset();
    void scheduleAutoSave();
    void doAutoSave();

private:
    void initContent();
    void applyTheme();

    std::vector<std::wstring> m_originalPaths;

    // 预设相关
=======
    explicit BatchRenameDialog(const std::vector<std::wstring>& originalPaths, bool isMirrorSource, QWidget* parent = nullptr);
    ~BatchRenameDialog() override = default;

    QString getFirstNewName() const { return m_firstNewName; }

private slots:
    void onAddRow();
    void updatePreview();
    void onExecute();
    void onPreview();
    void onBrowseTarget();
    void onImportPreset();
    void onExportPreset();
    void onDeleteCurrentPreset();
    void scheduleAutoSave();
    void doAutoSave();

private:
    void initContent();
    void applyTheme();

    std::vector<std::wstring> m_originalPaths;
    bool m_isMirrorSource = false;

    // 预设相关
>>>>>>> REPLACE
```

### 4.7 修改 `src/ui/BatchRenameDialog.cpp`
1. 构造函数存储并利用 `isMirrorSource` 判断。
2. `initContent()` 中若处于内存模式下，彻底禁用 Radio 选项，并直接 `hide()` 目标文件夹路径输入框及浏览按钮，阻断 UI 误触。
3. 重构并精简 `onExecute()`：不进行任何本地 I/O，只调用下沉的业务类，并根据真实成功数量对序列号进行安全更新和配置持久化。

```
<<<<<<< SEARCH
BatchRenameDialog::BatchRenameDialog(const std::vector<std::wstring>& originalPaths, QWidget* parent)
    : FramelessDialog("批量重命名 - ArcMeta", parent), m_originalPaths(originalPaths) {
    resize(850, 600); // 2026-04-11 按照用户要求：给予窗口更多弹性空间，提高初始显示质量
=======
BatchRenameDialog::BatchRenameDialog(const std::vector<std::wstring>& originalPaths, bool isMirrorSource, QWidget* parent)
    : FramelessDialog("批量重命名 - ArcMeta", parent), m_originalPaths(originalPaths), m_isMirrorSource(isMirrorSource) {
    resize(850, 600); // 2026-04-11 按照用户要求：给予窗口更多弹性空间，提高初始显示质量
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    m_rbRename = new QRadioButton("在同一文件夹中重命名", targetGroup);
    m_rbMove = new QRadioButton("移动到其他文件夹", targetGroup);
    m_rbCopy = new QRadioButton("复制到其他文件夹", targetGroup);
    m_rbRename->setChecked(true);
    targetL->addWidget(m_rbRename);
    targetL->addWidget(m_rbMove);
    targetL->addWidget(m_rbCopy);

    QHBoxLayout* pathL = new QHBoxLayout();
    m_targetPathEdit = new QLineEdit(targetGroup);
    m_targetPathEdit->setPlaceholderText("选择目标文件夹...");
    m_targetPathEdit->setFixedHeight(25);
    m_targetPathEdit->setEnabled(false);
    m_btnBrowse = new QPushButton("浏览...", targetGroup);
    m_btnBrowse->setFixedSize(80, 25);
    m_btnBrowse->setEnabled(false);
    pathL->addWidget(m_targetPathEdit);
    pathL->addWidget(m_btnBrowse);
    targetL->addLayout(pathL);
    configL->addWidget(targetGroup);
=======
    m_rbRename = new QRadioButton("在同一文件夹中重命名", targetGroup);
    m_rbMove = new QRadioButton("移动到其他文件夹", targetGroup);
    m_rbCopy = new QRadioButton("复制到其他文件夹", targetGroup);
    m_rbRename->setChecked(true);
    targetL->addWidget(m_rbRename);
    targetL->addWidget(m_rbMove);
    targetL->addWidget(m_rbCopy);

    QHBoxLayout* pathL = new QHBoxLayout();
    m_targetPathEdit = new QLineEdit(targetGroup);
    m_targetPathEdit->setPlaceholderText("选择目标文件夹...");
    m_targetPathEdit->setFixedHeight(25);
    m_targetPathEdit->setEnabled(false);
    m_btnBrowse = new QPushButton("浏览...", targetGroup);
    m_btnBrowse->setFixedSize(80, 25);
    m_btnBrowse->setEnabled(false);
    pathL->addWidget(m_targetPathEdit);
    pathL->addWidget(m_btnBrowse);
    targetL->addLayout(pathL);
    configL->addWidget(targetGroup);

    if (m_isMirrorSource) {
        m_rbMove->setEnabled(false);
        m_rbCopy->setEnabled(false);
        m_rbRename->setChecked(true);
        m_targetPathEdit->hide();
        m_btnBrowse->hide();
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void BatchRenameDialog::onExecute() {
    std::vector<RenameRule> rules;
    for (auto* row : m_ruleRows) rules.push_back(row->getRule());

    auto newNames = BatchRenameEngine::instance().preview(m_originalPaths, rules);
    QString targetDir = m_targetPathEdit->text();

    if ((m_rbMove->isChecked() || m_rbCopy->isChecked()) && targetDir.isEmpty()) {
        FramelessMessageBox::warning(this, "错误", "请先选择目标文件夹");
        return;
    }

    int successCount = 0;

    // 🚨 开启防抖与内部操作锁定，防止高密集 Windows IOCP 更名重命名变动反馈产生严重的系统刷新 and 竞态！
    MetadataManager::instance().beginInternalOperation();

    for (int i = 0; i < (int)m_originalPaths.size(); ++i) {
        QString oldPath = QString::fromStdWString(m_originalPaths[i]);
        QFileInfo oldInfo(oldPath);
        QString finalTargetDir = m_rbRename->isChecked() ? oldInfo.absolutePath() : targetDir;
        QString newPathStr = QDir(finalTargetDir).filePath(QString::fromStdWString(newNames[i]));

        bool ok = false;
        if (m_rbCopy->isChecked()) {
            ok = QFile::copy(oldPath, newPathStr);
        } else if (m_rbMove->isChecked()) {
            if (QFile::copy(oldPath, newPathStr)) {
                ok = QFile::remove(oldPath);
            }
        } else {
            ok = QFile::rename(oldPath, newPathStr);
        }

        if (ok) {
            successCount++;
            if (!m_rbCopy->isChecked()) {
                std::wstring oldW = oldInfo.absoluteFilePath().toStdWString();
                std::wstring newW = QDir(finalTargetDir).absoluteFilePath(QString::fromStdWString(newNames[i])).toStdWString();

                // 1. 同步进行元数据迁移
                MetadataManager::instance().renameItem(oldW, newW);

                // 2. 双轨制同步：同步修正 categories 表分类定义与 category_items 表中的 pathHint 引用，保持 1:1 分类镜像不损坏！
                CategoryRepo::renamePhysicalCategoryPath(oldW, newW);
            }
        }
    }

    if (successCount > 0 && !newNames.empty()) {
        m_firstNewName = QString::fromStdWString(newNames.front());
    }

    // 🚨 关闭内部操作锁定并提交
    MetadataManager::instance().endInternalOperation();

    // 发射全量 UI 刷新信号，使侧边栏分类树、内容视图同频重新计数 and 对账
    MetadataManager::instance().notifyFullUIRebuild();

    FramelessMessageBox::information(this, "操作完成", QString("成功处理 %1 个文件").arg(successCount));
    accept();
}
=======
void BatchRenameDialog::onExecute() {
    std::vector<RenameRule> rules;
    for (auto* row : m_ruleRows) rules.push_back(row->getRule());

    auto newNames = BatchRenameEngine::instance().preview(m_originalPaths, rules);
    QString targetDir = m_targetPathEdit->text();

    int actualSuccessCount = 0;

    if (m_isMirrorSource) {
        actualSuccessCount = MetadataManager::instance().batchRenameMemoryAssets(m_originalPaths, newNames);
    } else {
        bool isCopy = m_rbCopy->isChecked();
        bool isMove = m_rbMove->isChecked();

        // 确保非 I/O 调用下沉到 DiskFileManager
        #include "../util/DiskFileManager.h"
        actualSuccessCount = DiskFileManager::batchRenameDiskFiles(m_originalPaths, newNames, targetDir, isCopy, isMove);
    }

    if (actualSuccessCount > 0 && !newNames.empty()) {
        m_firstNewName = QString::fromStdWString(newNames.front());

        // 依据“实际成功数”精确递增序列号起始值，杜绝断号与跳号
        for (auto* row : m_ruleRows) {
            RenameRule rule = row->getRule();
            if (rule.type == RenameComponentType::Sequence) {
                rule.start = rule.start + actualSuccessCount * rule.step;
                row->setRule(rule);
            }
        }
        doAutoSave();
    }

    FramelessMessageBox::information(this, "操作完成", QString("成功处理 %1 个文件").arg(actualSuccessCount));
    accept();
}
>>>>>>> REPLACE
```

### 4.8 修改 `src/ui/ContentPanel.cpp`
连接时传入正确的视图数据来源。

```
<<<<<<< SEARCH
    BatchRenameDialog dlg(originalPaths, this);
    if (dlg.exec() == QDialog::Accepted) {
=======
    BatchRenameDialog dlg(originalPaths, isMirrorSource(), this);
    if (dlg.exec() == QDialog::Accepted) {
>>>>>>> REPLACE
```

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] 模块/文件：`src/ui/BatchRenameDialog.h` / `src/ui/BatchRenameDialog.cpp`（重构 UI 层逻辑，实现选项状态安全阻蔽，业务完全解耦下沉）
- [ ] 模块/文件：`src/meta/MetadataManager.h` / `src/meta/MetadataManager.cpp`（实现原子内存资产穿透更名及缩略图同频重命名）
- [ ] 模块/文件：`src/util/DiskFileManager.h` / `src/util/DiskFileManager.cpp`（新增物理磁盘流批量更名）
- [ ] 模块/文件：`CMakeLists.txt` / `src/ui/ContentPanel.cpp`

**明确禁止越界修改的范围：**
- [ ] 内容面板核心异步扫描及监控逻辑 —— 不修改

## 6. 实现准则与预警【核心】
1. **防止 UI I/O 渗透**：在任何情况下，`BatchRenameDialog.cpp` 内都不能再出现 `QFile::copy`、`QFile::remove` 等底层的具体物理文件操作，彻底划清界面层与数据层的物理界限。
2. **实际成功安全计数**：序列号更新必须直接使用后端方法返回的 `actualSuccessCount` 整数结果，禁止盲目依靠申请数组。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|--------------------------------------------|----------------|
| 内容面板数据源判定与强类型契约规范 | ContentPanel 必须公开 isMirrorSource() (返回是否为逻辑/镜像源数据) 与 isManagedContext() (返回当前是否处于已激活的托管库内可读写 SQLite DB 的可信生命周期内)。 | ✅ 符合（完全复用 `isMirrorSource()`） |

## 8. 待确认事项
无。
