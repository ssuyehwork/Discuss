# Implementation Plan: Ctrl+N 新建 Markdown 与 QuickLookWindow 双击关闭及回环跳转实施方案

本实施方案旨在严格遵循工业级 C++/Qt 架构规范与 `Memories.md`（第 31、32 章），在 `QuarkMeta_Source` 桌面应用中完整实现：
1. **快捷键 `Ctrl+N` 全局绑定直接调用接口创建 Markdown 文件**：在 `AppShortcutController` 中注册非编辑态快捷键 `Ctrl+N`，并通过 `PanelMediator` 调度当前活跃内容面板执行 `target->createNewItem("md")`；
2. **彻底解决 `QuickLookWindow` 空白内容无法双击关闭的架构缺陷**：为预览界面内所有展示容器（`m_lblEmptyPrompt`、`m_container` 以及窗口自身）无差别统一挂载双击关闭事件拦截，杜绝受内容是否为空的干预；
3. **`QuickLookWindow` 删除项目后自动跳转下一个，若为末项则回环至首项并弹出 `ToolTipOverlay` 提示**：在 `PanelMediator` 中优化 QuickLook 删除后的候选定位逻辑，末项自动回环并友好提示“当前处于第一个项目”。

---

## 一、方案设计与根因分析 (Overview & Root Cause Analysis)

### 1.1 问题一根因与设计目标：Ctrl+N 创建 Markdown 文件
- **现状分析**：
  在 `QuarkMeta_Source/src/ui/AppShortcutController.cpp` 中，仅注册了 `Ctrl+Shift+N` 用于新建文件夹：
  ```cpp
  registerShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N), [this]() {
      if (!isEditingFocus()) {
          emit createNewFolderRequested();
      }
  });
  ```
  而上下文菜单中已有成熟稳定的“创建 Markdown”实现（`m_panel->createNewItem("md")`）。
- **设计目标**：
  1. 在 `AppShortcutController.h` 中新增信号 `void createNewMdRequested();`；
  2. 在 `AppShortcutController.cpp` 中注册 `QKeySequence(Qt::CTRL | Qt::Key_N)`，检查 `!isEditingFocus()` 后触发 `emit createNewMdRequested();`；
  3. 在 `PanelMediator.cpp` 中连接该信号至活跃内容面板：
     ```cpp
     connect(shortcutController, &AppShortcutController::createNewMdRequested, this, [this, contentPanel]() {
         ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
         if (target) {
             target->createNewItem("md");
         }
     });
     ```

### 1.2 问题二根因与设计目标：QuickLookWindow 双击关闭与删除后回环跳转
- **缺陷 A（双击空白文件无法关闭窗口）根因**：
  在 `QuickLookWindow::renderText` 中，当文件内容为空时，`m_textEdit` 被隐藏，转而显示 `m_lblEmptyPrompt`（“该项目内容为空”）。然而在 `setupUi()` 中，`m_lblEmptyPrompt` 没有调用 `installEventFilter(this)`；且在 `QuickLookWindow::eventFilter` 中，只检查了：
  ```cpp
  if ((watched == m_textEdit || (hasTextEditViewport && watched == m_textEdit->viewport()) || watched == m_graphicsView) && event->type() == QEvent::MouseButtonDblClick) {
      closePreview();
      return true;
  }
  ```
  当文件为空时，双击落在 `m_lblEmptyPrompt` 或主容器 `m_container` 上，事件未被任何逻辑处理，导致无法关闭。
- **缺陷 A 修复方案**：
  给 `m_lblEmptyPrompt`、`m_container` 均挂载 `installEventFilter(this)`；在 `eventFilter` 的双击拦截中加入 `watched == m_lblEmptyPrompt || watched == m_container || watched == this`，确保无论当前文件有无内容、是图片还是空文本，双击任意有效区域均立即无条件调用 `closePreview()`。
- **缺陷 B（删除项目后回环至首项并 Toast）根因与方案**：
  在 `PanelMediator.cpp` 的 `deleteRequested` 槽函数中，当向下 `delta = 1` 找不到有效文件时，原本向上回退；当向上也找不到或已是末尾时，直接关闭了 QuickLook。
  修复方案：
  1. 优先向下获取下一个有效可预览项目；
  2. 若向下未找到（说明被删除的是最后一个项目），则不直接关闭，而是从索引 `0` 开始正向扫描找到当前视图中的**第一个可预览项目**；
  3. 若找到该首项且不等于被删除项，调用 `QuickLookWindow::instance().preview(firstValid)`，并立即通过 `ToolTipOverlay::instance()->showToast("当前处于第一个项目", 2000, QColor("#3498db"))` 给予用户明确提示；
  4. 只有当整个目录下所有项目均已被删除或完全不存在任何可预览项目时，才调用 `QuickLookWindow::instance().closePreview()`。

---

## 二、影响文件清单 (Modified Files List)

1. `QuarkMeta_Source/src/ui/AppShortcutController.h`
2. `QuarkMeta_Source/src/ui/AppShortcutController.cpp`
3. `QuarkMeta_Source/src/ui/QuickLookWindow.cpp`
4. `QuarkMeta_Source/src/ui/PanelMediator.cpp`

---

## 三、精准替换块 (Detailed Line-by-Line Changes)

### 3.1 `QuarkMeta_Source/src/ui/AppShortcutController.h`

新增 `createNewMdRequested` 信号：
<<<<<<< SEARCH
    /**
     * @brief Ctrl+Shift+N 触发新建文件夹
     */
    void createNewFolderRequested();

    /**
     * @brief Ctrl+Shift+E 触发提取文本内容至剪贴板
     */
=======
    /**
     * @brief Ctrl+Shift+N 触发新建文件夹
     */
    void createNewFolderRequested();

    /**
     * @brief Ctrl+N 触发新建 Markdown 文件
     */
    void createNewMdRequested();

    /**
     * @brief Ctrl+Shift+E 触发提取文本内容至剪贴板
     */
>>>>>>> REPLACE

---

### 3.2 `QuarkMeta_Source/src/ui/AppShortcutController.cpp`

注册 `Ctrl+N` 快捷键：
<<<<<<< SEARCH
    // 9. Ctrl+Shift+N: 新建文件夹
    registerShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N), [this]() {
        if (!isEditingFocus()) {
            emit createNewFolderRequested();
        }
    });
=======
    // 9. Ctrl+Shift+N: 新建文件夹
    registerShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N), [this]() {
        if (!isEditingFocus()) {
            emit createNewFolderRequested();
        }
    });

    // 9.1 Ctrl+N: 新建 Markdown 文件
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_N), [this]() {
        if (!isEditingFocus()) {
            emit createNewMdRequested();
        }
    });
>>>>>>> REPLACE

---

### 3.3 `QuarkMeta_Source/src/ui/QuickLookWindow.cpp`

在 `setupUi` 中为 `m_container` 和 `m_lblEmptyPrompt` 安装事件过滤器：
<<<<<<< SEARCH
    // 空文本提示标签
    m_lblEmptyPrompt = new QLabel(m_container);
    m_lblEmptyPrompt->setAlignment(Qt::AlignCenter);
    m_lblEmptyPrompt->setObjectName("QLEmptyPromptLabel");
    m_lblEmptyPrompt->hide();
    layout->addWidget(m_lblEmptyPrompt);
=======
    // 空文本提示标签
    m_lblEmptyPrompt = new QLabel(m_container);
    m_lblEmptyPrompt->setAlignment(Qt::AlignCenter);
    m_lblEmptyPrompt->setObjectName("QLEmptyPromptLabel");
    m_lblEmptyPrompt->hide();
    m_lblEmptyPrompt->installEventFilter(this);
    m_container->installEventFilter(this);
    layout->addWidget(m_lblEmptyPrompt);
>>>>>>> REPLACE

在 `eventFilter` 中支持无条件双击关闭（包含空内容标签与背景容器）：
<<<<<<< SEARCH
    if ((watched == m_textEdit || (hasTextEditViewport && watched == m_textEdit->viewport()) || watched == m_graphicsView) && event->type() == QEvent::MouseButtonDblClick) {
        closePreview();
        return true;
    }
=======
    if ((watched == m_textEdit || (hasTextEditViewport && watched == m_textEdit->viewport()) ||
         watched == m_graphicsView || watched == m_lblEmptyPrompt ||
         watched == m_container || watched == this) && event->type() == QEvent::MouseButtonDblClick) {
        closePreview();
        return true;
    }
>>>>>>> REPLACE

---

### 3.4 `QuarkMeta_Source/src/ui/PanelMediator.cpp`

显式包含 `ModelContract.h` 确保数据角色符号闭合：
<<<<<<< SEARCH
#include "AppShortcutController.h"
#include "QuickLookWindow.h"
=======
#include "AppShortcutController.h"
#include "QuickLookWindow.h"
#include "../core/ModelContract.h"
>>>>>>> REPLACE

连接 `createNewMdRequested`：
<<<<<<< SEARCH
        connect(shortcutController, &AppShortcutController::createNewFolderRequested, this, [this, contentPanel]() {
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
            if (target) {
                target->createNewItem("folder");
            }
        });
=======
        connect(shortcutController, &AppShortcutController::createNewFolderRequested, this, [this, contentPanel]() {
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
            if (target) {
                target->createNewItem("folder");
            }
        });

        connect(shortcutController, &AppShortcutController::createNewMdRequested, this, [this, contentPanel]() {
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
            if (target) {
                target->createNewItem("md");
            }
        });
>>>>>>> REPLACE

优化 `QuickLookWindow` 删除项目后的跳转与首项回环 Toast 提示逻辑：
<<<<<<< SEARCH
                if (!nextValid.isEmpty()) {
                    QuickLookWindow::instance().preview(nextValid);
                } else {
                    delta = -1;
                    QString prevValid;
                    while (true) {
                        QString candidate = target->getAdjacentFilePath(path, delta);
                        if (candidate.isEmpty()) break;
                        if (!QFileInfo(candidate).isDir() && UiHelper::canPreviewFile(candidate)) {
                            prevValid = candidate;
                            break;
                        }
                        delta--;
                    }
                    if (!prevValid.isEmpty()) {
                        QuickLookWindow::instance().preview(prevValid);
                    } else {
                        QuickLookWindow::instance().closePreview();
                    }
                }
                target->refreshAll();
=======
                if (!nextValid.isEmpty()) {
                    QuickLookWindow::instance().preview(nextValid);
                } else {
                    // 若下一个无有效文件，说明被删除的是最后一个项目：回环跳转至第一个可预览项目
                    QString firstValid;
                    QAbstractItemView* view = target->activeItemView();
                    if (view && view->model()) {
                        QAbstractItemModel* model = view->model();
                        int rowCount = model->rowCount();
                        for (int r = 0; r < rowCount; ++r) {
                            QModelIndex idx = model->index(r, 0);
                            if (idx.data(SectionHeaderRole).toBool()) continue;
                            QString p = idx.data(PathRole).toString();
                            if (p != path && !p.isEmpty() && !QFileInfo(p).isDir() && UiHelper::canPreviewFile(p)) {
                                firstValid = p;
                                break;
                            }
                        }
                    }

                    if (!firstValid.isEmpty()) {
                        QuickLookWindow::instance().preview(firstValid);
                        ToolTipOverlay::instance()->showToast("当前处于第一个项目", 2000, QColor("#3498db"));
                    } else {
                        QuickLookWindow::instance().closePreview();
                    }
                }
                target->refreshAll();
>>>>>>> REPLACE

---

## 四、构建与运行时验收标准 (Build & Verification Steps)

### 4.1 编译指令
```bash
cmake --build ArcMeta_Source/ArcMeta_Build/x64-release --config Release
```

### 4.2 运行时验收用例
1. **Ctrl+N 新建 Markdown 验证**：
   - 在主界面文件列表中按下 `Ctrl+N`；
   - 界面上立即新建一个名为 `新建 Markdown.md` 的项目并自动高亮进入行内重命名模式；
   - 输入文本（如在搜索框打字）时按下 `Ctrl+N`，正常输入字符，不误触发新建。
2. **QuickLookWindow 空白文件双击关闭验证**：
   - 预览一个大小为 0 字节的空白文本文件，界面显示“该项目内容为空”；
   - 鼠标左键双击该界面任意区域：QuickLookWindow 立即响应并平滑关闭。
3. **QuickLookWindow 删除末项回环跳转与 Toast 提示验证**：
   - 打开某个目录下最后一个图片/文件的 QuickLookWindow；
   - 右键点击“删除”，项目被移入回收站；
   - QuickLookWindow 自动回环预览当前目录下的第一个文件，并弹出 Toast 提示：“当前处于第一个项目”。

---

## 五、官方 API 复用与防另起炉灶 (Official API Reuse)
1. **新建文件复用**：严格复用 `ContentPanel::createNewItem("md")` 与 `FileCreationService::createNewItem`。
2. **预览支持判断**：严格复用 `UiHelper::canPreviewFile(p)`。
3. **视觉反馈**：严格复用 `ToolTipOverlay::instance()->showToast`。

---

## 六、数据单一事实来源声明表 (Data SSOT Declaration Table)

| 数据项 | 唯一真理源 (SSOT) | 变更通知通道 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 新建文件驱动 | `FileCreationService` | 文件系统变动 / `refreshAll()` | Controller -> Model/View |
| 快速预览状态 | `QuickLookWindow::m_currentPath` | `preview()` / `closePreview()` | Window 内部状态 |

---

## 七、接口签名与访问级别一致性核查 (Interface Consistency Check)

1. `AppShortcutController::createNewMdRequested()`：`public` 信号；
2. `ContentPanel::createNewItem(const QString& type)`：`public` 成员函数，签名一致；
3. `ContentPanel::activeItemView()`：`public` 成员函数，签名一致；
4. `QuickLookWindow::closePreview()`：`public` 槽函数，签名一致；
5. `ToolTipOverlay::showToast(const QString& text, int durationMs, const QColor& bgColor)`：`public` 接口，签名一致。

---

## 八、头文件包含链与类型完整性检查 (Header Chain Check)

1. `QuarkMeta_Source/src/ui/AppShortcutController.cpp`：已包含 `AppShortcutController.h`，类型闭合；
2. `QuarkMeta_Source/src/ui/QuickLookWindow.cpp`：已包含 `QuickLookWindow.h`、`QLabel`、`QPlainTextEdit`、`QuickLookGraphicsView.h`，类型闭合；
3. `QuarkMeta_Source/src/ui/PanelMediator.cpp`：显式包含 `../core/ModelContract.h`（包含 `SectionHeaderRole` 与 `PathRole` 定义）、`ToolTipOverlay.h`、`QuickLookWindow.h`、`UiHelper.h`，类型全部闭合。

---

## 九、合规与质量自检核对表 (Compliance Self-Check Table)

| 检查项 | 状态 | 规范与位置说明 |
| :--- | :--- | :--- |
| 1. 单一事实来源 (SSOT) 遵守 | 满足 | 快捷键归口 AppShortcutController，新建归口 FileCreationService |
| 2. 避免另起炉灶 (Search First) | 满足 | 复用既有 createNewItem("md") 和 ToolTipOverlay |
| 3. 语言规范（全中文） | 满足 | 实施方案全文除标识符外全部采用规范中文 |
| 4. 实施方案物理隔离 | 满足 | 存放于 `Implementation Plan/` 目录下 |
| 5. 命名与映射规范 | 满足 | 命名清晰描述业务功能与主导类 |
| 6. 代码替换块 Git Merge Diff 规范 | 满足 | 采用标准 SEARCH / REPLACE 块，原代码唯一匹配 |
| 7. 接口签名冻结与只读契约 | 满足 | 不修改既有 public API 签名 |
| 8. 访问级别一致性检查 | 满足 | 所有调用成员均为 public 级别 |
| 9. 内存与指针生命周期安全 | 满足 | 无悬空指针，无 Lambda 循环强引用 |
| 10. 交互设计零篡改 | 满足 | 严格实现用户明确要求的快捷键与回环跳转 |

---

## 十、行为零篡改与数值零篡改声明表 (Behavior Preservation Table)

| 既有行为/参数 | 原始设计 | 本次重构设计 | 结论 |
| :--- | :--- | :--- | :--- |
| Ctrl+Shift+N 快捷键 | 新建文件夹 | 保持完全一致 | 100% 行为零篡改 |
| 非空文件双击关闭 QuickLook | 双击关闭窗口 | 保持双击关闭窗口，扩展至空文件 | 100% 行为零篡改 |
| 删除中间项目后跳转 | 向下跳转下一个有效项 | 保持向下跳转下一个有效项 | 100% 行为零篡改 |
| Toast 提示颜色 | `#3498db` 蓝色通知 | 保持统一主题色 `#3498db` | 100% 数值零篡改 |
