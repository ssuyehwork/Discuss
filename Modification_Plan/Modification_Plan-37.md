# QuickLookWindow 滚动条样式对齐考古规范 —— Modification_Plan-37.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
在空格键打开的快速预览界面（QuickLookWindow）中，目前 `QuickLookGraphicsView` 和 `QPlainTextEdit` 使用的滚动条样式（高度 4px，宽度 4px，Handle 颜色 `#444`）未对齐系统全局的“考古”标准样式，破坏了界面的视觉一致性，属于上一代 AI Jules 引入的非标样式。
本方案旨在将该滚动条样式调整为完全符合全局“考古”设计规范的标准样式。

## 2. 问题定位
- **定位模块 1（src/ui/QuickLookWindow.cpp - QuickLookGraphicsView 构造函数）**：
  `QuickLookGraphicsView` 的水平和垂直滚动条目前被显式重写了 QSS 样式：
  ```cpp
  horizontalScrollBar()->setStyleSheet(R"(
      QScrollBar:horizontal { height: 4px; background: transparent; }
      QScrollBar::handle:horizontal { background: #444; border-radius: 2px; }
      QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { border: none; background: none; }
  )");
  verticalScrollBar()->setStyleSheet(R"(
      QScrollBar:vertical { width: 4px; background: transparent; }
      QScrollBar::handle:vertical { background: #444; border-radius: 2px; }
      QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { border: none; background: none; }
  )");
  ```
  这与 `Memories.md` 第 10 条快速预览规范中的“宽度 10px、圆角 3px、背景透明、Handle 颜色对齐 `BorderColor` (#333333)”不一致。
- **定位模块 2（src/ui/QuickLookWindow.cpp - setupUi 文本框初始化）**：
  `m_textEdit` 对象的垂直滚动条目前亦被重写了类似的 QSS：
  ```cpp
  m_textEdit->verticalScrollBar()->setStyleSheet(R"(
      QScrollBar:vertical { width: 4px; background: transparent; }
      QScrollBar::handle:vertical { background: #444; border-radius: 2px; }
  )");
  ```
  这同样违反了考古统一原则。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 按下空格键打开预览界面后的滚动条样式，违背了考古 | 重新配置 `QuickLookGraphicsView` 和 `QPlainTextEdit` 滚动条，宽度 10px、高度 10px、圆角 3px、背景透明、Handle 颜色使用 `StyleLibrary` 中的 `BorderColor`（其色值为 `#333333`）。 | ✅ 一致 |

## 4. 详细解决方案

### 4.1 引入 StyleLibrary.h
在 `src/ui/QuickLookWindow.cpp` 开头（若无）引入样式库文件：
```cpp
#include "StyleLibrary.h"
```

### 4.2 统一替换滚动条样式
1. **重构 `QuickLookGraphicsView` 滚动条样式**：
   将其构造函数中的水平及垂直滚动条样式修改为符合考古规范：
   - 水平滚动条（对应用户原话：“宽度 10px、圆角 3px、背景透明、Handle 颜色对齐 BorderColor (#333333)”）：
     ```qss
     QScrollBar:horizontal { height: 10px; background: transparent; }
     QScrollBar::handle:horizontal { background: #333333; border-radius: 3px; }
     QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { border: none; background: none; }
     QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: none; }
     ```
   - 垂直滚动条（对应用户原话：“宽度 10px、圆角 3px、背景透明、Handle 颜色对齐 BorderColor (#333333)”）：
     ```qss
     QScrollBar:vertical { width: 10px; background: transparent; }
     QScrollBar::handle:vertical { background: #333333; border-radius: 3px; }
     QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { border: none; background: none; }
     QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
     ```

2. **重构 `m_textEdit`（QPlainTextEdit）滚动条样式**：
   在 `QuickLookWindow::setupUi()` 中将 `m_textEdit` 垂直与水平滚动条更新为完全对齐规范的 QSS 样式（同上）。

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] 模块/文件：`src/ui/QuickLookWindow.cpp` (具体包含 `QuickLookGraphicsView` 的构造函数和 `QuickLookWindow::setupUi()`，且只修改对应的滚动条样式)

**明确禁止越界修改的范围：**
- [ ] 预览交互/逻辑控制等非滚动条样式逻辑——不修改。

## 6. 实现准则与预警【核心】
1. 必须精确包含 `#include "StyleLibrary.h"` 或直接使用 `#333333` 确保 Handle 颜色与系统整体一致。
2. 必须核对并确认滚动条四周的 `add-line`、`sub-line` 以及页面背景区域（`add-page`, `sub-page`）被安全设为透明或无，避免原生 Qt 滚动条的三角形箭头按钮和滑道底色闪现破坏极简视觉。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| 快速预览 (QuickLook) 规范 | 预览窗口内的滚动条样式必须严格遵循全局规范：宽度 10px、圆角 3px、背景透明、Handle 颜色对齐 `BorderColor` (#333333)。 | ✅ 符合。本方案正是为了使预览窗口内所有滚动条（包含图片视口、文本框等）均满足上述尺寸与色值规范。 |
