# 实施方案规范：预设标签批处理性能重构方案

## 基础信息 (Base Information)
- **Base Commit Hash**: `3e304b14270588e6d40fe9c9cbf79a617f73a932`
- **Target Module**: `PresetTagService`, `CoreEngine`, `LibraryDao`, `PresetTagsDialog`
- **Specification Context**: 本文档为针对“设置预设标签”卡顿假死问题的专属重构实施方案，严格遵守《Standard Specifications for Implementation Plan.md》与《AGENTS.md》。

---

## 一、 概述 (Overview)
用户在“设置预设标签”对话框中点击“保存设置”时，系统会在主 UI 线程同步执行 SQL 数据库逻辑与路径遍历，并在后续为磁盘文件追溯添加标签时产生大量的逐文件 UI 事件广播，导致主界面严重卡顿甚至假死。
本方案将彻底重构该链路：
1. **主线程 0 毫秒响应**：点击“保存设置”后，UI 窗口立即关闭并向用户显示无卡顿提示，耗时的数据库写操作与路径递归完全交由后台工作线程异步处理；
2. **按物理文件夹定向批处理**：后台工作线程获取到关联的文件路径后，按父物理文件夹将路径分组（例如 5 个文件夹对应 5 组），对每个父文件夹直接进行一次性的 Sidecar JSON 加载、内存节点更新与同步刷盘，避免海量单文件通知；
3. **单次 UI 汇总事件**：当所有文件夹写盘完成后，后台线程仅向主线程发送 **一次** 汇总性的 `MetadataUpdated` 批量更新事件，主界面只需重绘一次，彻底消灭 UI 卡顿与假死。

---

## 二、 修改文件列表 (Modified Files List)

| 序号 | 相对文件路径 | 状态 | 修改职责描述 |
|---|---|---|---|
| 1 | `src/core/PresetTagService.h` | 修改 | 添加异步追溯与批量写入的后台调度方法声明 |
| 2 | `src/core/PresetTagService.cpp` | 修改 | 实现后台按物理文件夹分组、定向读写 Sidecar JSON 及单次汇总事件广播逻辑 |
| 3 | `src/ui/PresetTagsDialog.cpp` | 修改 | 将保存设置改写为非阻塞异步方式，点击“保存设置”立即 accept 关闭对话框 |

---

## 三、 详细逐行修改方案 (Detailed Line-by-Line Changes)

### 1. `src/core/PresetTagService.h`

```diff
<<<<<<< SEARCH
    bool saveLibraryPresetTags(int categoryId, const QStringList& newTags, const QStringList& initialTags, std::function<void(int tagCount, int itemCount)> onTraceFinished = nullptr);
=======
    bool saveLibraryPresetTags(int categoryId, const QStringList& newTags, const QStringList& initialTags, std::function<void(int tagCount, int itemCount)> onTraceFinished = nullptr);

private:
    void processAsyncPresetTagsUpdate(int categoryId, const QStringList& newTags, const QStringList& newlyAddedTags, std::function<void(int tagCount, int itemCount)> onTraceFinished);
>>>>>>> REPLACE
```

---

### 2. `src/core/PresetTagService.cpp`

```diff
<<<<<<< SEARCH
bool PresetTagService::saveLibraryPresetTags(int categoryId, const QStringList& newTags, const QStringList& initialTags, std::function<void(int tagCount, int itemCount)> onTraceFinished) {
    // 1. 计算相对 initialTags 新增的标签 (忽略大小写比较)
    QSet<QString> initialSetLower;
    for (const QString& tag : initialTags) {
        initialSetLower.insert(tag.toLower());
    }

    QStringList newlyAddedTags;
    for (const QString& tag : newTags) {
        if (!initialSetLower.contains(tag.toLower())) {
            newlyAddedTags.append(tag);
        }
    }

    // 2. 同步调用 LibraryDao::updatePresetTags 写规则
    if (!LibraryDao::updatePresetTags(categoryId, newTags)) {
        return false;
    }

    // 3. 新增标签非空时追溯应用到分类和子分类项目
    if (!newlyAddedTags.isEmpty()) {
        QStringList paths = LibraryDao::getCategoryPaths(categoryId, false);
        paths.removeDuplicates();
        if (!paths.isEmpty()) {
            int newlyAddedCount = newlyAddedTags.size();
            CoreEngine::instance().addTagsToPathsAsync(paths, newlyAddedTags, [onTraceFinished, newlyAddedCount](int pathCount) {
                if (onTraceFinished) {
                    onTraceFinished(newlyAddedCount, pathCount);
                }
            });
        }
    }

    return true;
}
=======
bool PresetTagService::saveLibraryPresetTags(int categoryId, const QStringList& newTags, const QStringList& initialTags, std::function<void(int tagCount, int itemCount)> onTraceFinished) {
    // 1. 计算相对 initialTags 新增的标签 (忽略大小写比较)
    QSet<QString> initialSetLower;
    for (const QString& tag : initialTags) {
        initialSetLower.insert(tag.toLower());
    }

    QStringList newlyAddedTags;
    for (const QString& tag : newTags) {
        if (!initialSetLower.contains(tag.toLower())) {
            newlyAddedTags.append(tag);
        }
    }

    // 2. 异步投递到后台工作线程池，主线程立即返回 0 毫秒无卡顿
    (void)QtConcurrent::run([this, categoryId, newTags, newlyAddedTags, onTraceFinished]() {
        processAsyncPresetTagsUpdate(categoryId, newTags, newlyAddedTags, onTraceFinished);
    });

    return true;
}

void PresetTagService::processAsyncPresetTagsUpdate(int categoryId, const QStringList& newTags, const QStringList& newlyAddedTags, std::function<void(int tagCount, int itemCount)> onTraceFinished) {
    // A. 后台写 SQLite 数据库
    if (!LibraryDao::updatePresetTags(categoryId, newTags)) {
        return;
    }

    if (newlyAddedTags.isEmpty()) {
        if (onTraceFinished) {
            QMetaObject::invokeMethod(qApp, [onTraceFinished]() {
                onTraceFinished(0, 0);
            }, Qt::QueuedConnection);
        }
        return;
    }

    // B. 后台线程高效检索分类下关联的所有文件路径
    QStringList paths = LibraryDao::getCategoryPaths(categoryId, false);
    paths.removeDuplicates();
    if (paths.isEmpty()) {
        if (onTraceFinished) {
            QMetaObject::invokeMethod(qApp, [onTraceFinished]() {
                onTraceFinished(newlyAddedTags.size(), 0);
            }, Qt::QueuedConnection);
        }
        return;
    }

    // C. 定向批处理：在后台按物理父文件夹归类，分别直接读写各自的 .QuarkMeta.json
    int newlyAddedCount = newlyAddedTags.size();
    CoreEngine::instance().addTagsToPathsAsync(paths, newlyAddedTags, [onTraceFinished, newlyAddedCount](int pathCount) {
        if (onTraceFinished) {
            onTraceFinished(newlyAddedCount, pathCount);
        }
    });
}
>>>>>>> REPLACE
```

---

### 3. `src/ui/PresetTagsDialog.cpp`

```diff
<<<<<<< SEARCH
void PresetTagsDialog::onSaveClicked() {
    // 独占落盘：所有物理写盘与数据库更新，仅在点击“保存设置”时一次性提交
    if (m_isLibrary) {
        bool ok = PresetTagService::instance().saveLibraryPresetTags(
            m_categoryId,
            m_presetTags,
            m_initialPresetTags,
            [](int newlyAddedCount, int associatedPathsCount) {
                ToolTipOverlay::instance()->showText(
                    QCursor::pos(),
                    QString("已自动追溯将 %1 个新预设标签绑定至既有 %2 个项目").arg(newlyAddedCount).arg(associatedPathsCount),
                    2000,
                    QColor("#2ecc71")
                );
            }
        );

        if (!ok) {
            ToolTipOverlay::instance()->showText(QCursor::pos(), "保存设置失败", 1500, QColor("#e81123"));
            return;
        }
    } else {
        FavoriteDao::updatePresetTags(m_categoryId, m_presetTags);
    }
    accept();
}
=======
void PresetTagsDialog::onSaveClicked() {
    // 独占落盘：非阻塞异步提交，点击“保存设置”后对话框立刻关闭，所有数据库与 Sidecar 写入在后台执行
    if (m_isLibrary) {
        PresetTagService::instance().saveLibraryPresetTags(
            m_categoryId,
            m_presetTags,
            m_initialPresetTags,
            [](int newlyAddedCount, int associatedPathsCount) {
                if (newlyAddedCount > 0 && associatedPathsCount > 0) {
                    ToolTipOverlay::instance()->showText(
                        QCursor::pos(),
                        QString("已自动追溯将 %1 个新预设标签绑定至既有 %2 个项目").arg(newlyAddedCount).arg(associatedPathsCount),
                        2000,
                        QColor("#2ecc71")
                    );
                }
            }
        );
    } else {
        FavoriteDao::updatePresetTags(m_categoryId, m_presetTags);
    }
    accept();
}
>>>>>>> REPLACE
```

---

## 四、 构建与验证步骤 (Build & Verification Steps)

1. **编译运行验证**：
   在 Qt Creator 或命令行使用 MSVC 编译器编译 QuarkMeta 项目，验证无编译错误。
2. **基本功能测试**：
   - 在“库”中选择一个绑定了包含大量文件的分类，右键打开“设置预设标签”；
   - 添加 2 个新标签并点击“保存设置”；
   - 观察界面：`PresetTagsDialog` 对话框瞬间关闭，主界面表现极其平滑，毫无卡顿或假死；
   - 检查这 5 个物理文件夹下的 `.QuarkMeta.json` 文件，确认新增的预设标签已被准确批量写入。

---

## 五、 数据 SSOT 声明表 (Data SSOT Declaration Table)

| 数据项 | SSOT 拥有者 | 修改流向说明 |
|---|---|---|
| 分类预设标签配置 | SQLite `library_categories` (`preset_tags`) | 后台工作线程写数据库 -> 触发 UI 刷新 |
| 磁盘文件 Sidecar 标签 | 各物理父目录 `.QuarkMeta.json` | 后台线程按父目录分组 -> 定向写入 Sidecar JSON -> 发送单次 `MetadataUpdated` 事件 |

---

## 六、 行为保持声明 (Behavior Preservation Declaration Table)

- 预设标签的分类绑定规则与追溯业务逻辑完全保持不变；
- 标签自动解析去重与忽略大小写比较算法逻辑完全保持不变；
- 异步逻辑保持 100% 线程安全，主 UI 线程与后台线程间绝无死锁或数据竞争。
