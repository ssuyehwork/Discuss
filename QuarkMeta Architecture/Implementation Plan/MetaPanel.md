# MetaPanel 深度纯化与安全闭环实施方案

## Overview
本实施方案旨在对 `MetaPanel`（属性面板）进行深度纯化与安全闭环重构：
1. **纯 View 架构解耦**：彻底剥离 `MetaPanel` 中的直接写盘/数据库操作代码，使其充当纯粹的 Presentation View，仅对外发射标准 Qt 信号。
2. **Delta 差集增量打标**：标签变动改为基于 `QSet` 差集计算，仅对新增与删除的标签发射单点请求，杜绝多选打标时覆盖擦除其他文件私有标签的严重缺陷。
3. **多选与只读状态机守卫**：多选（>1 项）时强制禁用单文件重命名框 `m_nameEdit`；加密文件（`.amenc`）或回收站（`trash://`）文件触发全量只读保护。
4. **FlowLayout 内存生命周期**：废除对象池，统一采用 Qt 标准父子对象与 `deleteLater()` 进行内存管理，彻底根除悬空指针隐患。
5. **安全 URL 校验**：接入 `QUrl::fromUserInput` 与 `http/https` 协议白名单校验。

## Modified Files List
- `src/ui/MetaPanel.h`
- `src/ui/MetaPanel.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/MetaPanel.h`

<<<<<<< SEARCH
    void setNote(const std::wstring& note);
    void setURL(const std::wstring& url);
    void setURL(const std::wstring& url);
    void setRating(int rating, bool fromUser = false);
    void setColor(const std::wstring& color, bool fromUser = false);
    void setPinned(bool pinned) { Q_UNUSED(pinned); }

signals:
    void tagAddRequested(const QStringList& paths, const QString& tag);
    void tagRemoveRequested(const QStringList& paths, const QString& tag);
    void metadataChanged(int rating, const std::wstring& color);
    void noteEdited(const QStringList& paths, const QString& newNote);
    void linkEdited(const QStringList& paths, const QString& newLink);
    void primaryColorChanged(const QString& path, const QColor& color);
    void tagsChanged(const QStringList& paths, const QStringList& tags);
    void searchByColor(const QColor& color);
    void renameRequested(const QString& oldPath, const QString& newPath);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void initUi();
    void updateControlsState(bool hasSelection);
    void adjustFlowHeights();
    void addInfoRow(QVBoxLayout* layout, const QString& label, QLabel*& valueLabel);
    QWidget* createCollapsibleSection(const QString& title, QWidget* contentWidget, bool defaultExpanded = true);

    QVBoxLayout* m_mainLayout = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_container = nullptr;
    QVBoxLayout* m_containerLayout = nullptr;

    // 1. 顶部预览与色板区 (有则显，无则完全隐藏)
    QWidget* m_topPreviewBox = nullptr;
    QLabel* m_lblImagePreview = nullptr;
    FlowLayout* m_paletteFlowLayout = nullptr;

    // 2. 文件名编辑区 (大字高亮)
    ElasticEdit* m_nameEdit = nullptr;

    // 3. 备注说明区 (可折叠)
    ElasticEdit* m_noteEdit = nullptr;

    // 4. 关联网址区 (可折叠，一体化容器与独立跳转按钮)
    QWidget* m_linkBox = nullptr;
    QLineEdit* m_linkEdit = nullptr;
    QAction* m_actOpenLink = nullptr;

    // 5. 星级评级 + 颜色色标条 (8 色圆点)
    QWidget* m_ratingColorBox = nullptr;
    QList<QPushButton*> m_starBtns;
    QList<QPushButton*> m_colorBtns;
    int m_currentRating = 0;
    std::wstring m_currentColor;

    // 6. 标签管理区 (可折叠)
    QWidget* m_tagBox = nullptr;
    QWidget* m_tagContainer = nullptr;
    FlowLayout* m_tagFlowLayout = nullptr;
    QPushButton* m_btnAddTagBig = nullptr;
    QPushButton* m_btnAddTagSmall = nullptr;
    QPointer<TagSelectorOverlay> m_tagSelectorOverlay;

    // 7. 基础物理属性区 (可折叠)
    QWidget* m_infoSectionWidget = nullptr;
    QLabel* lblType = nullptr;
    QLabel* lblSize = nullptr;
    QLabel* lblDimensions = nullptr;
    QLabel* lblCtime = nullptr;
    QLabel* lblMtime = nullptr;
    QLabel* lblAtime = nullptr;
    QLabel* lblEncrypted = nullptr;

    // 8. 物理路径区 (可折叠)
    QLineEdit* m_pathEdit = nullptr;
    QPushButton* m_btnCopyPath = nullptr;
    QPushButton* m_btnOpenLocation = nullptr;

    QStringList m_selectedPaths;
    QList<TagPill*> m_tagPool;
    QList<ColorPill*> m_colorPool;
    QTimer* m_adjustTimer = nullptr;
    bool m_isInternalUpdating = false;
    bool m_isUserEditing = false;

private slots:
    void onTagDeleted(const QString& text);
    void setAsPrimaryColor(const QColor& color);
    void openTagSelectorOverlay(QWidget* targetAnchor);
=======
    void setNote(const std::wstring& note);
    void setURL(const QString& url);
    void setURL(const std::wstring& url);
    void setRating(int rating, bool fromUser = false);
    void setColor(const std::wstring& color, bool fromUser = false);
    void setPinned(bool pinned) { Q_UNUSED(pinned); }

signals:
    void tagAddRequested(const QStringList& paths, const QString& tag);
    void tagRemoveRequested(const QStringList& paths, const QString& tag);
    void metadataChanged(int rating, const std::wstring& color);
    void noteEdited(const QStringList& paths, const QString& newNote);
    void linkEdited(const QStringList& paths, const QString& newLink);
    void primaryColorChanged(const QString& path, const QColor& color);
    void tagsChanged(const QStringList& paths, const QStringList& tags);
    void searchByColor(const QColor& color);
    void renameRequested(const QString& oldPath, const QString& newPath);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void initUi();
    void updateControlsState(bool hasSelection, bool isMultiSelection, bool isReadOnly);
    void adjustFlowHeights();
    void addInfoRow(QVBoxLayout* layout, const QString& label, QLabel*& valueLabel);
    QWidget* createCollapsibleSection(const QString& title, QWidget* contentWidget, bool defaultExpanded = true);

    QVBoxLayout* m_mainLayout = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_container = nullptr;
    QVBoxLayout* m_containerLayout = nullptr;

    // 1. 顶部预览与色板区
    QWidget* m_topPreviewBox = nullptr;
    QLabel* m_lblImagePreview = nullptr;
    QWidget* m_paletteContainer = nullptr;
    FlowLayout* m_paletteFlowLayout = nullptr;

    // 2. 文件名编辑区
    ElasticEdit* m_nameEdit = nullptr;

    // 3. 备注说明区
    ElasticEdit* m_noteEdit = nullptr;

    // 4. 关联网址区
    QWidget* m_linkBox = nullptr;
    QLineEdit* m_linkEdit = nullptr;
    QAction* m_actOpenLink = nullptr;

    // 5. 星级评级 + 颜色色标条
    QWidget* m_ratingColorBox = nullptr;
    QList<QPushButton*> m_starBtns;
    QList<QPushButton*> m_colorBtns;
    int m_currentRating = 0;
    std::wstring m_currentColor;

    // 6. 标签管理区
    QWidget* m_tagBox = nullptr;
    QWidget* m_tagContainer = nullptr;
    FlowLayout* m_tagFlowLayout = nullptr;
    QPushButton* m_btnAddTagBig = nullptr;
    QPushButton* m_btnAddTagSmall = nullptr;
    QPointer<TagSelectorOverlay> m_tagSelectorOverlay;

    // 7. 基础物理属性区
    QWidget* m_infoSectionWidget = nullptr;
    QLabel* lblType = nullptr;
    QLabel* lblSize = nullptr;
    QLabel* lblDimensions = nullptr;
    QLabel* lblCtime = nullptr;
    QLabel* lblMtime = nullptr;
    QLabel* lblAtime = nullptr;
    QLabel* lblEncrypted = nullptr;

    // 8. 物理路径区
    QLineEdit* m_pathEdit = nullptr;
    QPushButton* m_btnCopyPath = nullptr;
    QPushButton* m_btnOpenLocation = nullptr;

    QStringList m_selectedPaths;
    QSet<QString> m_currentTagsSet;
    QTimer* m_adjustTimer = nullptr;
    bool m_isInternalUpdating = false;
    bool m_isUserEditing = false;
    bool m_isReadOnlyMode = false;

private slots:
    void onTagDeleted(const QString& text);
    void setAsPrimaryColor(const QColor& color);
    void openTagSelectorOverlay(QWidget* targetAnchor);
>>>>>>> REPLACE
```

## Build & Verification Steps
1. 编译验证：无需额外 cmake 配置修改，纯 C++ 头文件与实现逻辑更新。
2. 逻辑验证：运行测试选区切换、Delta 打标及 URL 白名单解析。
