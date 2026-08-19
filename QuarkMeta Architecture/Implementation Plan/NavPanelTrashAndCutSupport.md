# NavPanel Trash Shortcut & Operations Implementation Plan (目录导航回收站按钮与回收站完整交互无脑实施方案)

## Overview (概述)
本方案旨在为 QuarkMeta 纯磁盘直连模式补全回收站的入口与专属右键交互链条：
1. 在第一栏目录导航栏 (`NavPanel`) 顶部 Header 区域添加 **“回收站” 按钮**，点击触发 `requestOpenTrash()` 信号通知 `MainWindow`，调用 `m_contentPanel->loadCategory("trash")` 展示回收站项目。
2. 重构回收站数据模式 (`m_currentCategoryType == "trash"`) 下内容面板 (`ContentPanel`) 的右键菜单排列与响应逻辑，严格按照指定顺序排列：
   - **【还原】**：原路秒级归位到删除前的原始物理目录。
   - **【剪切】**：记录回收站中物理路径并标记为剪切，支持在任意磁盘文件夹执行“粘贴”实现跨目录恢复。
   - **【永久删除】**：彻底抹除物理文件及关联数据库记录。
   - **【还原全部】**：一键还原所有回收站项目。
   - **【清空回收站】**：一键清空整个回收站。

---

## Detailed Modifications (具体代码修改点与精准行号)

### 1. `src/ui/NavPanel.h` & `src/ui/NavPanel.cpp`
- **文件路径**：`src/ui/NavPanel.h`
- **修改内容**：
  在 `signals:` 区域增加 `requestOpenTrash()` 信号：
  ```cpp
  signals:
      void directorySelected(const QString& path);
      void requestOpenTrash(); // 新增：请求打开回收站
  ```

- **文件路径**：`src/ui/NavPanel.cpp` (约 117-130 行 Header 构建处)
- **修改内容**：
  在 Header Layout 中标题 `titleLabel` 右侧、`addStretch()` 之前，添加回收站按钮 `QPushButton* m_btnTrash`：
  ```cpp
  QPushButton* btnTrash = new QPushButton(header);
  btnTrash->setFixedSize(24, 24);
  btnTrash->setIcon(UiHelper::getIcon("trash", QColor("#e81123"), 16));
  btnTrash->setProperty("tooltipText", "打开回收站");
  btnTrash->installEventFilter(this);
  btnTrash->setStyleSheet(
      "QPushButton { background: transparent; border: none; border-radius: 4px; }"
      "QPushButton:hover { background: #3E3E42; }"
      "QPushButton:pressed { background: #4E4E52; }"
  );
  connect(btnTrash, &QPushButton::clicked, this, &NavPanel::requestOpenTrash);

  headerLayout->addWidget(titleLabel);
  headerLayout->addStretch();
  headerLayout->addWidget(btnTrash);
  ```

---

### 2. `src/ui/MainWindow.cpp`
- **文件路径**：`src/ui/MainWindow.cpp` (约 253 行 `m_navPanel` 信号连接处)
- **修改内容**：
  连接 `m_navPanel` 的 `requestOpenTrash` 信号：
  ```cpp
  connect(m_navPanel, &NavPanel::requestOpenTrash, this, [this]() {
      if (m_contentPanel) {
          m_contentPanel->loadCategory("trash"); // 加载回收站数据
      }
      if (m_addressBar) {
          m_addressBar->setPath("trash://");
      }
      m_currentPath = "trash://";
      updateNavButtons();
      updateStatusBar();
  });
  ```

---

### 3. `src/ui/ContentPanel.cpp`
- **文件路径**：`src/ui/ContentPanel.cpp` (约 1513-1655 行右键菜单构建处 `onCustomContextMenuRequested`)
- **修改内容**：
  重构 `if (m_currentCategoryType == "trash")` 分支的右键菜单构建逻辑：

  ```cpp
  if (m_currentCategoryType == "trash") {
      if (onItem) {
          // 1. 【还原】
          menu.addAction(UiHelper::getIcon("sync", QColor("#2ecc71"), 18), "还原")->setData(ActionRestore);

          // 2. 【剪切】(新增支持)
          menu.addAction(UiHelper::getIcon("cut", QColor("#EEEEEE"), 18), "剪切")->setData(ActionCut);

          // 3. 【永久删除】
          menu.addAction(UiHelper::getIcon("trash", QColor("#e81123"), 18), "永久删除")->setData(ActionSecureDelete);

          menu.addSeparator();

          // 4. 【还原全部】
          menu.addAction(UiHelper::getIcon("sync", QColor("#2ecc71"), 18), "还原全部")->setData(ActionRestoreAll);

          // 5. 【清空回收站】
          menu.addAction(UiHelper::getIcon("trash", QColor("#e81123"), 18), "清空回收站")->setData(ActionEmptyTrash);
      } else {
          // 空白处菜单：还原全部、清空回收站
          menu.addAction(UiHelper::getIcon("sync", QColor("#2ecc71"), 18), "还原全部")->setData(ActionRestoreAll);
          menu.addAction(UiHelper::getIcon("trash", QColor("#e81123"), 18), "清空回收站")->setData(ActionEmptyTrash);
      }

      // 显示右键菜单并响应 setData
      ...
      return;
  }
  ```

- **文件路径**：`src/ui/ContentPanel.cpp` (粘贴 ActionPaste 响应处，约 2895-2915 行)
- **修改内容**：
  在 `onPaste()` 或执行剪切移动时，当源项目来自于回收站且粘贴目标为有效正常目录时，调用 `DiskTrashService::restoreToDirectory(trashPath, targetDir)` 将回收站文件移动放回用户指定的目的地，并清除数据库记录。

---

## Verification Plan (验证方案)
1. **构建与编译验证**：
   在 Linux 容器环境运行命令验证代码无语法/链接错误：
   ```bash
   cd /repo && cmake -B build && cmake --build build
   ```
2. **功能验证**：
   - 检查 `NavPanel` 顶部 Header 界面，验证“回收站”按钮放置在标题右侧，样式与悬停效果贴合系统主题。
   - 点击“回收站”按钮，验证 `ContentPanel` 正确加载展示回收站数据。
   - 右键选中回收站项目，验证菜单项排列严格满足：`【还原】 -> 【剪切】 -> 【永久删除】 -> 【还原全部】 -> 【清空回收站】`。
