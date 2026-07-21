# 预览窗快捷键 ToolTip 屏幕居中淡入淡出、切图加固及选中双向联动 —— Modification_Plan-39.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
在快速预览界面（QuickLookWindow）中：
1. 之前的对话要求：按下评分（1-5 键）和颜色标签（Alt + 1-9）快捷键时，ToolTip 提示需要在屏幕上方居中，且自带淡入淡出（Fade In/Out）效果。此部分已由上游代码自动合并（`ToolTipOverlay`、`MainWindow.cpp` 评分与颜色气泡动画居中已就绪）。
2. 目前的切图/导航功能仍无法正常使用，原因在于 `QPlainTextEdit` 文本框和 `QGraphicsView` 视口在拥有焦点时，会把方向键（Up/Down/Left/Right）等键盘按键拦截用于自己内部的滚动条滚动，导致 `QuickLookWindow` 顶层无法截获，切图信号没有发出。
3. 之前要求的切图联动（上一张/下一张时，主界面 `ContentPanel` 的选中高亮项和可见区域同步联动），已由上游实现了 `ContentPanel::selectAndScrollToPath` API 并在 `MainWindow` 中连接，但由于按键未被截获所以未能触发。

## 2. 问题定位
- **定位模块（src/ui/QuickLookWindow.cpp - setupUi / eventFilter）**：
  `m_graphicsView` 未安装 `QuickLookWindow` 的事件过滤器，导致当图片预览获得焦点时，方向按键被其内部滚动条消费。
  在 `eventFilter` 中对 `m_textEdit` 和 `m_graphicsView` 的 KeyPress 事件增加对 `Space`、`Escape`、`Ctrl+W`、方向键、星级数字、`Alt+` 颜色数字键的物理优先截获。拦截后直接手工转发给 `keyPressEvent(keyEvent)`，然后返回 `true` 阻止子控件内部消费。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 在执行“QuickLookWindow”里的这些快捷键时，通过 ToolTipOverlay 淡进淡出的显示出来，位置应该在靠齐屏幕上方居中 | 已由上游代码合并（`ToolTipOverlay` 支持淡入淡出、`MainWindow.cpp` 支持上方居中）。 | ✅ 一致 |
| 2    | 切图/导航的功能 如同摆设，按下按键之后，没有切换到下一个或上一个文件 | 在 `QuickLookWindow` 中注册对 `m_graphicsView` 的过滤器，在 `eventFilter` 里优先拦截方向键，并委托 `keyPressEvent` 派发 `prevRequested` 和 `nextRequested`；在 `MainWindow` 里已连接调用 `ContentPanel::selectAndScrollToPath` 自动起效。 | ✅ 一致 |

## 4. 详细解决方案

### 4.1 安装过滤器与强化截获功能
1. **安装过滤器**：
   在 `src/ui/QuickLookWindow.cpp` 的 `QuickLookWindow::setupUi()` 中增加对 `m_graphicsView` 的事件过滤器安装：
   ```cpp
   m_graphicsView->installEventFilter(this);
   ```
2. **重构 `QuickLookWindow::eventFilter` 截获功能**：
   ```cpp
   bool QuickLookWindow::eventFilter(QObject* watched, QEvent* event) {
       if ((watched == m_textEdit || watched == m_graphicsView) && event->type() == QEvent::KeyPress) {
           QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
           bool intercept = false;
           int key = keyEvent->key();
           Qt::KeyboardModifiers mods = keyEvent->modifiers();
           
           if (key == Qt::Key_Space || key == Qt::Key_Escape) {
               intercept = true;
           } else if (key == Qt::Key_W && (mods & Qt::ControlModifier)) {
               intercept = true;
           } else if (key == Qt::Key_Up || key == Qt::Key_Left || key == Qt::Key_Down || key == Qt::Key_Right) {
               intercept = true;
           } else if (key >= Qt::Key_1 && key <= Qt::Key_5 && !(mods & Qt::AltModifier)) {
               intercept = true;
           } else if ((mods & Qt::AltModifier) && key >= Qt::Key_1 && key <= Qt::Key_9) {
               intercept = true;
           }
           
           if (intercept) {
               keyPressEvent(keyEvent);
               return true; // 彻底物理截断，防止被 PlainTextEdit 或 QGraphicsView 吞没
           }
       }

       if (event->type() == QEvent::WindowDeactivate) {
           if (m_ignoreDeactivate) {
               return true;
           }
           closePreview();
       }
       return QWidget::eventFilter(watched, event);
   }
   ```

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] `src/ui/QuickLookWindow.cpp` (物理安装过滤器、拦截并转发按键)

**明确禁止越界修改的范围：**
- [ ] 其他非预览模块——不修改。

## 6. 实现准则与预警【核心】
1. 在 `eventFilter` 拦截后必须直接返回 `true`，防止事件继续向下分发而被子控件处理。
2. 确保在 keyPressEvent 中处理完逻辑后妥善处理 `accept()`。

## 7. Memories.md 合规检查
不涉及。
