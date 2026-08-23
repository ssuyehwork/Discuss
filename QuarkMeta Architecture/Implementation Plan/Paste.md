### 粘贴（Paste）功能全链路根治重构实施方案

---

### 一、 核心重构目标

1. **精准动态置灰**：在右键菜单与快捷键中，根据剪贴板内容、目标路径有效性、读写权限以及同目录剪切冲突，**毫秒级精准控制“粘贴”菜单项的启用与置灰（Enabled/Disabled）**。
2. **彻底消灭静默覆盖**：移除底层 Windows Shell 的 `FOF_NOCONFIRMATION` 强制覆盖标志，支持冲突弹窗或自动重命名避让，保护现有文件。
3. **元数据与缩略图 100% 同步漫游**：无论是复制粘贴还是剪切粘贴，自动将源文件的 `.QuarkMeta.json` 元数据与本地高清缩略图完整同步至目标目录。

---

### 二、 具体代码实施细节

#### 1. 在 `ContentPanel.h` 中增加全局粘贴能力判定函数
**文件：`src/ui/ContentPanel.h`**

```cpp
public:
    /**
     * @brief 判定当前上下文是否允许执行粘贴操作（用于菜单置灰与快捷键拦截）
     */
    bool canPaste() const;
```

---

#### 2. 在 `ContentPanel.cpp` 中实现严密的置灰判定与菜单挂载
**文件：`src/ui/ContentPanel.cpp`**

- **实现 `canPaste()` 逻辑判定**：
```cpp
bool ContentPanel::canPaste() const {
    // 1. 目标目录必须是真实物理目录，且不是“此电脑”、“回收站”或“搜索结果”
    if (m_currentPath.isEmpty() || m_currentPath == "computer://" || m_currentPath == "trash://" ||
        m_currentCategoryType == "trash" || m_currentCategoryType == "path_list") {
        return false;
    }

    // 2. 目标目录必须在物理磁盘上存在且具备写入权限
    QFileInfo destInfo(m_currentPath);
    if (!destInfo.exists() || !destInfo.isDir() || !destInfo.isWritable()) {
        return false;
    }

    // 3. 检查系统剪贴板是否有有效的文件 URL
    const QMimeData* mime = QApplication::clipboard()->mimeData();
    if (!mime || !mime->hasUrls() || mime->urls().isEmpty()) {
        return false;
    }

    // 4. 提取剪贴板来源路径，确保至少有 1 个真实物理文件存在
    bool isCut = false;
    if (mime->hasFormat("Preferred DropEffect")) {
        QByteArray effect = mime->data("Preferred DropEffect");
        if (!effect.isEmpty() && (effect.at(0) & 0x02)) isCut = true;
    }

    QString nativeDest = QDir::toNativeSeparators(m_currentPath);
    bool hasValidSource = false;
    bool isSameDirCut = true;

    for (const QUrl& url : mime->urls()) {
        QString localPath = QDir::toNativeSeparators(url.toLocalFile());
        if (localPath.isEmpty()) continue;

        QFileInfo srcInfo(localPath);
        if (srcInfo.exists()) {
            hasValidSource = true;
            // 如果存在剪切且来源父目录与当前目录不同，则不是原地剪切
            if (QDir::toNativeSeparators(srcInfo.absolutePath()) != nativeDest) {
                isSameDirCut = false;
            }
        }
    }

    if (!hasValidSource) return false;

    // 5. 如果是剪切操作，且所有文件都在当前目录内（原地剪切），则禁用粘贴
    if (isCut && isSameDirCut) {
        return false;
    }

    return true;
}
```

- **在右键菜单中挂载动态置灰状态（`onCustomContextMenuRequested`）**：
```cpp
    // 在右键项目与空白处菜单中，统一将 actPaste 的可用性与 canPaste() 绑定
    QAction* actPaste = menu.addAction("粘贴");
    actPaste->setData(ActionPaste);
    actPaste->setEnabled(canPaste()); // 🚨 物理绑定：不满足条件时自动显示为灰色
```

- **在快捷键事件过滤器中加固拦截（`eventFilter`）**：
```cpp
    if (keyEvent->modifiers() & Qt::ControlModifier && keyEvent->key() == Qt::Key_V) {
        if (canPaste()) {
            performPaste();
        }
        return true;
    }
```

---

#### 3. 改造 `ShellHelper.cpp`：彻底消灭静默物理覆盖
**文件：`src/util/ShellHelper.cpp`**

- **移除 `FOF_NOCONFIRMATION`，允许系统冲突交互与安全避让**：
```cpp
bool ShellHelper::copyOrMoveItems(const QStringList& sourcePaths, const QString& destDir, bool isMove) {
#ifdef Q_OS_WIN
    if (sourcePaths.isEmpty() || destDir.isEmpty()) return false;
    
    std::wstring from;
    for (const QString& p : sourcePaths) {
        from += QDir::toNativeSeparators(p).toStdWString() + L'\0';
    }
    from += L'\0';

    std::wstring to = QDir::toNativeSeparators(destDir).toStdWString() + L'\0' + L'\0';

    SHFILEOPSTRUCTW fileOp = { 0 };
    fileOp.wFunc = isMove ? FO_MOVE : FO_COPY;
    fileOp.pFrom = from.c_str();
    fileOp.pTo = to.c_str();
    // 🚨 核心改动：移除 FOF_NOCONFIRMATION，遇到同名冲突由系统弹出确认或允许用户选择保留两者，绝不静默覆写！
    fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMMKDIR;
    bool ok = (SHFileOperationW(&fileOp) == 0 && !fileOp.fAnyOperationsAborted);

    if (ok) {
        for (const QString& p : sourcePaths) {
            QFileInfo info(p);
            QString newPath = QDir(destDir).filePath(info.fileName());

            // 🚨 无论 Copy 还是 Move，自动触发整包元数据与缩略图原子漫游！
            QuarkMetaJson::roamItemMetadata(p, newPath, isMove);
            DiskMediaExtractor::roamThumbnailCache(p, newPath, isMove);
        }
    }
    return ok;
#else
    return false;
#endif
}
```

---

### 三、 预期效果

1. **菜单智能感知**：在没有复制文件、剪切原地粘贴、处于此电脑/回收站/只读目录时，“粘贴”选项**100% 自动呈现灰色且不可点击**。
2. **文件安全有保障**：粘贴到已有同名文件的文件夹时，**绝不再发生静默物理覆盖**。
3. **数据完整漫游**：粘贴完成后，目标目录的新文件**瞬间继承源文件的星级、颜色、标签、尺寸与高清缩略图**。