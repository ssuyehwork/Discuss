# 全域空白处拖拽“还原+移动”交互重构实施方案 (FramelessWindowHelper-1.md)

## 1. Overview
根据 `Memories.md` 中更新的“窗口空白处拖拽与还原交互规范”，扩展 `FramelessWindowHelper` 对无边框主窗口内任意非交互空白区域（含标题栏、各面板背景底板）的拖拽感知与响应机制。

当主窗口处于最大化状态且用户在主窗口内任意非交互控件的空白区域按下鼠标左键拖拽时，统一触发“还原 + 移动”（Restore + Move）逻辑：
1. `WM_NCHITTEST` 阶段：检测光标位置下的子控件，若为非交互控件（`isInteractiveWidget == false`），统一返回 `HTCAPTION`；
2. `WM_NCLBUTTONDOWN` 阶段：捕获 `HTCAPTION` 按下事件。若窗口当前处于最大化状态，捕获光标在当前最大化工作区中的横向比例 `factorX` 和纵向比例 `factorY`；
3. 调用 `showNormal()` 将窗口还原为正常尺寸后，依据 `factorX` 与 `factorY` 精准计算还原后窗口的 TopLeft 坐标（`newX = globalX - normalWidth * factorX`, `newY = globalY - normalHeight * factorY`），确保光标相对窗口位置保持绝对一致，绝无突兀跳跃；
4. 触发 Windows 原生拖拽（`SendMessageW(..., WM_NCLBUTTONDOWN, HTCAPTION, ...)`），实现全域空白平滑跟随；若窗口原本就是普通/还原状态，则直接触发原生平滑纯移动。

---

## 2. Modified Files List
- `src/ui/FramelessWindowHelper.h`
- `src/ui/FramelessWindowHelper.cpp`

---

## 3. Detailed Line-by-Line Changes

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
    // 2. 原生窗口全域空白处拖动与最大化“还原+移动”处理
    if (msg->message == WM_NCLBUTTONDOWN && msg->wParam == HTCAPTION) {
        if (m_window->isMaximized()) {
            POINT screenPt = { GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam) };
            QPoint globalPos(screenPt.x, screenPt.y);

            // 计算当前光标在最大化工作区横向与纵向上的精确比例
            QRect maxGeom = m_window->geometry();
            double factorX = static_cast<double>(globalPos.x() - maxGeom.left()) / qMax(1, maxGeom.width());
            double factorY = static_cast<double>(globalPos.y() - maxGeom.top()) / qMax(1, maxGeom.height());

            // 执行还原
            m_window->showNormal();

            // 根据双向比例精确计算还原尺寸下光标所对应的 TopLeft 坐标（无跳变）
            QRect normalGeom = m_window->geometry();
            int newX = globalPos.x() - static_cast<int>(normalGeom.width() * factorX);
            int newY = globalPos.y() - static_cast<int>(normalGeom.height() * factorY);

            m_window->move(newX, newY);

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

        // 允许全窗口任意非交互空白区域（标题栏及各面板底板空白处）响应拖拽与还原
        if (m_window && !m_window->isFullScreen()) {
            QWidget* childAtPt = m_window->childAt(localPos);
            if (!isInteractiveWidget(childAtPt, m_titleBar, m_window)) {
                *result = HTCAPTION;
                return true;
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
3. 将主窗口最大化；
4. 在主窗口内任意非交互空白区域（如顶部标题栏空白处、中部或底部面板底板空白处）按住鼠标左键拖拽；
5. 验证：主窗口立即还原为正常尺寸，光标无任何跳变，精准保持在点击处的比例点并跟随鼠标平滑移动；
6. 在正常/还原尺寸下，按住主窗口任意空白区域拖拽，验证其平滑移动。
