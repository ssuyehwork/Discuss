# ContentPaneSplitManager Step 2 Implementation Plan

## 1. Overview
本方案为【窗格树架构重构】的第二步实施方案，主要完成以下核心内容：
1. **全局唯一落点判定函数与高亮 Overlay (`evaluateDropAtPosition`)**：
   - 建立全工程唯一的判定函数 `evaluateDropAtPosition(globalPos, sourcePane)`，输入鼠标全局坐标，精准判定目标窗格及 上/下/左/右 侧（沿用 25%/75% 阈值）。
   - 当落点有效时返回蓝色半透明高亮矩形（高亮区域对应新窗格占据的一半区域）；若落在自身、同位置无意义落点或目标区域小于 `kMinPaneWidth` / `kMinPaneHeight`，则判定为无效，不显示高亮。释放时若空间不足，使用 `ToolTipOverlay::instance()->showText(QCursor::pos(), "空间不足，无法放置到此处", 2000, QColor("#e81123"))` 进行标准红色提示。
2. **按住标题栏 QDrag 拖动窗格 (`application/x-quarkmeta-panemove`)**：
   - 多窗格模式下（`isSplitMode()` 为 true），支持按住标题栏空白处或标题文字区拖动。达到 `startDragDistance()` 启动 QDrag，采用专属 MIME 类型 `application/x-quarkmeta-panemove`。
   - 松开后直接在窗格树中提取 `sourceLeaf` 节点，放到目标侧，执行 `normalizeTree`。窗格内部 Controller/Model/View 状态 100% 原样保留，绝不重新创建 `ContentPanel` 或重新加载数据。
3. **标签页拖入与 MIME 物理隔离**：
   - 标签页拖入（`application/x-quarkmeta-tab`）与窗格拖动共享 `ContentPanel` 拖放路径及 `evaluateDropAtPosition` 判定函数。
   - 文件/文件夹拖放（`text/uri-list`）与窗格/标签页拖动严格隔离，文件拖放绝不唤起分屏高亮 Overlay。
4. **彻底清理废弃的方向切换代码**：
   - 物理彻底删除旧的方向切换逻辑及声明 `evaluateOrientationToggle`。

---

## 2. Modified Files List
- `src/ui/ContentHeaderWidget.h`
- `src/ui/controllers/ContentPaneSplitManager.h`
- `src/ui/controllers/ContentPaneSplitManager.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/ContentHeaderWidget.h`

```
<<<<<<< SEARCH
private:
    void initUi();
    class ContentPanel* owningPanel() const;
    bool evaluateOrientationToggle(const QPoint& currentPos, Qt::Orientation& targetOri) const;

    QHBoxLayout* m_layout = nullptr;
=======
private:
    void initUi();
    class ContentPanel* owningPanel() const;

    QHBoxLayout* m_layout = nullptr;
>>>>>>> REPLACE
```

### File 2: `src/ui/controllers/ContentPaneSplitManager.h`

```
<<<<<<< SEARCH
    void updateDragOverlayGlobal(const QPoint& globalPos, ContentPanel* sourcePane = nullptr);
    void updateDragOverlay(const QPoint& pos);
    void hideDragOverlay();
=======
    void updateDragOverlayGlobal(const QPoint& globalPos, ContentPanel* sourcePane = nullptr);
    void hideDragOverlay();
>>>>>>> REPLACE
```

### File 3: `src/ui/controllers/ContentPaneSplitManager.cpp`

```
<<<<<<< SEARCH
void ContentPaneSplitManager::updateDragOverlay(const QPoint& pos) {
    QPoint globalPos = m_panel->mapToGlobal(pos);
    updateDragOverlayGlobal(globalPos, nullptr);
}

void ContentPaneSplitManager::hideDragOverlay() {
=======
void ContentPaneSplitManager::hideDragOverlay() {
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. **CMake 构建与编译**：
   ```bash
   cmake --build build
   ```
2. **落点与高亮实时验证**：
   - 拖拽窗格标题栏或标签页，在目标窗格的 25%/75% 边缘悬停，验证蓝色半透明矩形精准覆盖半侧区域。
   - 拖至中心区域、自身或空间不足位置时，高亮 Overlay 自动隐藏；在空间不足区域释放时出现红色 `ToolTipOverlay` 提示。
3. **彻底清理与全局检索验证**：
   运行 `grep` 指令，验证无遗留的 `evaluateOrientationToggle` 或硬编码 230/4 字面量：
   ```bash
   grep -rn "evaluateOrientationToggle" src/
   ```

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- [x] **单一定位判定函数**：窗格拖动与标签页拖入 100% 共用唯一的 `evaluateDropAtPosition` 函数。
- [x] **死代码彻底清理**：移除旧的方向切换逻辑 `evaluateOrientationToggle` 及无用声明，零残余逻辑。

---

## 6. Header API Signature Verification
- `ContentPaneSplitManager::evaluateDropAtPosition(const QPoint&, ContentPanel*)` ➔ `SplitEvaluationResult`（匹配 `ContentPaneSplitManager.h`）
- `ContentPaneSplitManager::movePane(ContentPanel*, ContentPanel*, Qt::Orientation, bool)` ➔ `void`（匹配 `ContentPaneSplitManager.h`）

---

## 7. Header Inclusion Chain & Type Completeness Check
- `#include "ContentPaneSplitManager.h"` 与 `#include "ToolTipOverlay.h"` 闭合包含，包含链无断裂。
