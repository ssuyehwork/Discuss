# 预览窗快捷键 ToolTip 屏幕居中淡入淡出、切图加固及选中双向联动 —— Modification_Plan-39.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
在快速预览界面（QuickLookWindow）中：
1. 目前按下评分（1-5 键）和颜色标签（Alt + 1-9）快捷键时，屏幕左上角固定的 ToolTip 提示位置偏移（固定的 `QPoint(50, 50)`），且缺少淡进淡出（Fade In/Out）动画，视觉突兀，不符合高精要求。
2. 目前的切图/导航功能无法使用，原因在于 `QPlainTextEdit` 文本框和 `QGraphicsView` 视口在拥有焦点时，会把方向键（Up/Down/Left/Right）等键盘按键拦截用于自己内部的滚动条滚动，导致 `QuickLookWindow` 顶层无法截获，切图信号没有发出。
3. 预览切图（上一张/下一张）时，主界面 `ContentPanel` 的选中高亮项和可见区域未与预览对应的文件同步发生联动变化。

## 2. 问题定位
- **定位模块 1（src/ui/ToolTipOverlay.h / src/ui/ToolTipOverlay.cpp）**：
  `ToolTipOverlay` 目前是不透明、无动画的弹出。需要加入 `QPropertyAnimation`，通过操作 `windowOpacity` 属性支持 150ms 的淡入淡出。同时，新增精确定位参数 `exactPosition`，使外界可以直接精确指定中心点。
- **定位模块 2（src/ui/QuickLookWindow.cpp - eventFilter / keyPressEvent）**：
  `m_graphicsView` 未安装 `QuickLookWindow` 的事件过滤器，导致当图片预览获得焦点时，按键被其底层吞没。
  在 `eventFilter` 中对 `m_textEdit` 和 `m_graphicsView` 的 KeyPress 事件增加对 `Space`、`Escape`、`Ctrl+W`、方向键、星级数字、`Alt+` 颜色数字键的物理优先截获。拦截后直接手工转发给 `keyPressEvent(keyEvent)`，然后返回 `true` 阻止子控件内部处理。
- **定位模块 3（src/ui/ContentPanel.h / src/ui/ContentPanel.cpp）**：
  `ContentPanel` 缺少从路径选中并滚动到对应视图行号的联动 API `selectAndScrollToPath(const QString& path)`。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 在执行“QuickLookWindow”里的这些快捷键时，通过 ToolTipOverlay 淡进淡出的显示出来，位置应该在靠齐屏幕上方居中 | 在 `ToolTipOverlay` 中支持 `windowOpacity` 150ms 属性动画淡入淡出，计算当前活动屏幕的水平中心，计算 HTML 气泡的理想尺寸后，通过 `exactPosition` 将其定位到靠齐屏幕上方居中。 | ✅ 一致 |
| 2    | 切图/导航的功能 如同摆设，按下按键之后，没有切换到下一个或上一个文件 | 在 `QuickLookWindow` 注册对 `m_graphicsView` 的过滤器，在 `eventFilter` 里优先拦截方向键，并委托 `keyPressEvent` 派发 `prevRequested` 和 `nextRequested`；在 `MainWindow` 里联动调用 `ContentPanel::selectAndScrollToPath`。 | ✅ 一致 |

## 4. 详细解决方案

### 4.1 ToolTipOverlay 引入淡入淡出与精确定位
1. **更新 `src/ui/ToolTipOverlay.h`**：
   - 包含 `#include <QPropertyAnimation>`。
   - 新增成员 `QPropertyAnimation* m_fadeAnim = nullptr;`。
   - 新增私有函数 `void fadeOutAndHide();`。
   - 更改 `showText` 的声明，在最后添加 `bool exactPosition = false` 参数。
2. **重构 `src/ui/ToolTipOverlay.cpp`**：
   - 构造函数初始化属性动画：
     ```cpp
     m_fadeAnim = new QPropertyAnimation(this, "windowOpacity", this);
     ```
   - 在 `showText` 中增加支持：
     - 如果在淡出，先停止动画。
     - 若 `exactPosition` 为真，则直接定位 `pos = globalPos`，不做任何偏移和边缘反转裁剪。
     - 显示前先设 `setWindowOpacity(0.0)`，调用 `show()`，然后启动 150ms 淡入动画到 `1.0`。
   - 在 `m_hideTimer` 触发时，不再直接调用 `hide`，而是通过连接的槽调用 `fadeOutAndHide()` 进行 150ms 淡出至 `0.0`，在动画 `finished` 信号里执行 `QWidget::hide()`。

### 4.2 拦截子控件按键事件，修复切图导航
1. **安装过滤器**：
   在 `QuickLookWindow::setupUi()` 中增加对 `m_graphicsView` 的事件过滤器安装：
   ```cpp
   m_graphicsView->installEventFilter(this);
   ```
2. **强化 `QuickLookWindow::eventFilter` 截获功能**：
   ```cpp
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
   ```

### 4.3 ContentPanel 选中双向联动实现
1. **声明接口**：
   在 `src/ui/ContentPanel.h` 的 public 节下声明：
   ```cpp
   void selectAndScrollToPath(const QString& path);
   ```
2. **实现接口**：
   在 `src/ui/ContentPanel.cpp` 中遍历 `m_proxyModel`，匹配 `PathRole` 等于传入路径。若匹配成功，从当前正在显示的视图（网格/列表）中设置选区：
   ```cpp
   void ContentPanel::selectAndScrollToPath(const QString& path) {
       if (!m_proxyModel) return;
       for (int i = 0; i < m_proxyModel->rowCount(); ++i) {
           QModelIndex proxyIdx = m_proxyModel->index(i, 0);
           if (proxyIdx.data(PathRole).toString() == path) {
               QAbstractItemView* view = (m_viewStack->currentWidget() == m_treeView) ? 
                   static_cast<QAbstractItemView*>(m_treeView) : static_cast<QAbstractItemView*>(m_gridView);
               if (view) {
                   view->scrollTo(proxyIdx);
                   view->setCurrentIndex(proxyIdx);
                   view->selectionModel()->select(proxyIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
               }
               break;
           }
       }
   }
   ```
3. **在 MainWindow 中建立同步联动**：
   当 `prevRequested` 或 `nextRequested` 时，在预览下一张图的同时，调用 `m_contentPanel->selectAndScrollToPath(next);` 使得主界面列表高亮也随切图完美平移。

### 4.4 快捷键提示信息全局居中展现
在 `src/ui/MainWindow.cpp` 的 `ratingRequested` 和 `colorRequested` 连接槽中，增加居中定位公式：
```cpp
    // 居中位置计算
    QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen) screen = QGuiApplication::primaryScreen();
    QRect screenGeom = screen ? screen->geometry() : QRect(0, 0, 1920, 1080);

    QTextDocument doc;
    doc.setHtml(msg);
    doc.setDefaultStyleSheet("body, div, p, span, b, i { color: #EEEEEE !important; font-family: 'Microsoft YaHei', 'Segoe UI'; font-size: 9pt; }");
    doc.setDocumentMargin(0);
    qreal idealW = doc.idealWidth();
    if (idealW > 450) idealW = 450;
    int w = static_cast<int>(idealW) + 24;
    
    int centerX = screenGeom.x() + screenGeom.width() / 2;
    int targetX = centerX - w / 2;
    int targetY = screenGeom.y() + 50; // 靠齐屏幕上方居中 (留 50px 顶部安全间距)
    
    ToolTipOverlay::instance()->showText(QPoint(targetX, targetY), msg, 1500, borderColor, true);
```

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] `src/ui/ToolTipOverlay.h` 与 `src/ui/ToolTipOverlay.cpp`
- [ ] `src/ui/QuickLookWindow.cpp` (物理安装过滤器、拦截转发按键)
- [ ] `src/ui/ContentPanel.h` 与 `src/ui/ContentPanel.cpp` (添加 `selectAndScrollToPath` API)
- [ ] `src/ui/MainWindow.cpp` (在 `ratingRequested`/`colorRequested`/`prevRequested`/`nextRequested` 连接槽内调用新计算的居中定位，以及同步联动 ContentPanel)

## 6. 实现准则与预警【核心】
1. 属性动画必须在主线程调用（`showText` 已有主线程确保保护）。
2. 在 `exactPosition` 开启时，跳过 ToolTipOverlay 原有的大块偏移量，确保传递的值即为最终显示原点。

## 7. Memories.md 合规检查
不涉及。
