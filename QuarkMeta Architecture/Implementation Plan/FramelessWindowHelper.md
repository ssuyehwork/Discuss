# 无边框最大化窗口拖拽“还原+移动”交互重构实施方案 (FramelessWindowHelper.md)

## 1. Overview
根据 `Memories.md` 中定义的“标题栏拖拽与还原交互规范”，重构 `FramelessWindowHelper` 模块对 Windows 原生消息 `WM_NCLBUTTONDOWN` / `WM_MOUSEMOVE` 及 Qt 事件的处理。

当无边框主窗口处于最大化状态且用户在标题栏非交互区域按下鼠标左键开始拖拽时，自动触发“还原 + 移动”（Restore + Move）逻辑：
1. 捕获按下的全局坐标并记录当前最大化工作区比例；
2. 调用 `showNormal()` 将窗口恢复为普通尺寸；
3. 根据鼠标按下的相对比例精准计算还原后窗口的新 TopLeft 坐标，确保标题栏在鼠标光标正下方平滑跟随拖拽；
4. 若窗口本身处于正常/还原状态，则维持原有的纯移动逻辑。

---

## 2. Modified Files List
- `src/ui/FramelessWindowHelper.h`
- `src/ui/FramelessWindowHelper.cpp`

---

## 3. Detailed Line-by-Line Changes

### `src/ui/FramelessWindowHelper.h`

<<<<<<< SEARCH
    bool m_isResizing = false;
    int m_resizeDir = 0;
    QPoint m_resizeStartGlobalPos;
    QRect m_resizeStartGeometry;
=======
    bool m_isResizing = false;
    bool m_isDraggingMaximized = false;
    int m_resizeDir = 0;
    QPoint m_resizeStartGlobalPos;
    QRect m_resizeStartGeometry;
    QPoint m_dragNormalOffset;
>>>>>>> REPLACE

---

### `src/ui/FramelessWindowHelper.cpp`

<<<<<<< SEARCH
    // 2. 原生标题栏拖动识别：坚决杜绝抢占顶部 8px 缩放热区
    if (msg->message == WM_NCHITTEST) {
        POINT screenPt = { GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam) };
        QPoint localPos = m_window->mapFromGlobal(QPoint(screenPt.x, screenPt.y));

        if (!m_window->isMaximized() && !m_window->isFullScreen() && localPos.y() <= kBaseResizeMargin) {
            return false; // 顶部 8px 放行给 Qt
        }

        if (m_titleBar && !m_window->isFullScreen()) {
            QRect titleRect = QRect(m_titleBar->mapTo(m_window, QPoint(0, 0)), m_titleBar->size());
            if (titleRect.contains(localPos)) {
                QWidget* childAtPt = m_window->childAt(localPos);
                if (!isInteractiveWidget(childAtPt, m_titleBar, m_window)) {
                    *result = HTCAPTION;
                    return true;
                }
            }
        }

        return false;
    }
=======
    // 2. 原生标题栏拖动与最大化“还原+移动”处理
    if (msg->message == WM_NCLBUTTONDOWN && msg->wParam == HTCAPTION) {
        if (m_window->isMaximized()) {
            POINT screenPt = { GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam) };
            QPoint globalPos(screenPt.x, screenPt.y);

            // 计算当前光标在最大化宽度上的比例
            QRect maxGeom = m_window->geometry();
            double factorX = static_cast<double>(globalPos.x() - maxGeom.left()) / maxGeom.width();

            // 执行还原
            m_window->showNormal();

            // 精确计算还原后尺寸下的鼠标 TopLeft 偏移
            QRect normalGeom = m_window->geometry();
            int newX = globalPos.x() - static_cast<int>(normalGeom.width() * factorX);
            int newY = globalPos.y() - 15; // 居中挂载在 34px 标题栏中上部

            m_window->move(newX, newY);
            m_isDraggingMaximized = true;

            // 触发系统原生拖拽
            ReleaseCapture();
            SendMessageW(msg->hwnd, WM_NCLBUTTONDOWN, HTCAPTION, msg->lParam);
            *result = 0;
            return true;
        }
    }

    if (msg->message == WM_NCHITTEST) {
        POINT screenPt = { GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam) };
        QPoint localPos = m_window->mapFromGlobal(QPoint(screenPt.x, screenPt.y));

        if (!m_window->isMaximized() && !m_window->isFullScreen() && localPos.y() <= kBaseResizeMargin) {
            return false; // 顶部 8px 放行给 Qt
        }

        if (m_titleBar && !m_window->isFullScreen()) {
            QRect titleRect = QRect(m_titleBar->mapTo(m_window, QPoint(0, 0)), m_titleBar->size());
            if (titleRect.contains(localPos)) {
                QWidget* childAtPt = m_window->childAt(localPos);
                if (!isInteractiveWidget(childAtPt, m_titleBar, m_window)) {
                    *result = HTCAPTION;
                    return true;
                }
            }
        }

        return false;
    }
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. 使用 CMake 编译项目：
   ```bash
   cmake --build --preset x64-Debug
   ```
2. 运行 `QuarkMeta.exe`；
3. 双击标题栏或点击最大化按钮，使窗口处于最大化状态；
4. 鼠标左键按住标题栏空白区域向下方/左右拉动；
5. 验证：窗口立即还原为常规正常尺寸，且光标精准悬停在标题栏对应的横向比例位置，跟随鼠标平滑移动；
6. 验证：当窗口原本就是正常/还原尺寸时，按住标题栏拖拽仅平滑移动窗口，不触发额外的尺寸突变。
