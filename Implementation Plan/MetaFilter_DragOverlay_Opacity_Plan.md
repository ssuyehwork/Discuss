# Implementation Plan - MetaPanel & FilterPanel Drag Overlay Opacity Adjustment

## Chapter 1: Overview（概述与需求分析）

### 1. 现象与需求分析
用户要求单独针对应用在**元数据面板（`MetaPanel`）**与**筛选器面板（`FilterPanel`）**上的拖拽迁移遮罩层（`DragOverlayWidget`）的透明度参数进行精确调整：
- 原样式 `QWidget#OrientationPreviewWidget, QWidget#DragOverlayWidget` 统一使用了 `rgba(0, 122, 255, 0.25)`（25% 不透明度 / 75% 半透明）。
- 现要求将元数据面板与筛选器面板上的拖拽遮罩透明度独立调整为 **`50% 不透明度 / 50% 半透明`**（即 `rgba(0, 122, 255, 0.50)`），同时与其他场景（如分屏方向预览 `OrientationPreviewWidget`）解耦隔离，避免相互混淆影响。

### 2. 重构设计与解决目标
1. 保持分屏预览遮罩 `QWidget#OrientationPreviewWidget` 25% 不透明度不变。
2. 为元数据面板与筛选器面板上的拖拽遮罩 `QWidget#DragOverlayWidget` 配置独立的 QSS 样式 `rgba(0, 122, 255, 0.50)`。
3. 遵循样式统一收敛规则，全部样式规则集中在 `resources/style.qss` 中声明，禁止在 C++ 控件代码中内联硬编码。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `resources/style.qss`
2. `Memories.md`

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `resources/style.qss`
将 `QWidget#DragOverlayWidget` 从统一选择器中拆离，并为元数据与筛选器面板的拖拽遮罩设置 `rgba(0, 122, 255, 0.50)`（50% 不透明度 / 50% 半透明）：

```
<<<<<<< SEARCH
/* 分屏拖拽与方向预览遮罩层静态样式 */
QWidget#OrientationPreviewWidget, QWidget#DragOverlayWidget {
    background-color: rgba(0, 122, 255, 0.25);
    border: 2px solid #007AFF;
}
=======
/* 分屏拖拽与方向预览遮罩层静态样式 */
QWidget#OrientationPreviewWidget {
    background-color: rgba(0, 122, 255, 0.25);
    border: 2px solid #007AFF;
}

/* 元数据面板与筛选器面板拖拽迁移遮罩层静态样式 (50% 不透明度 / 50% 半透明) */
QWidget#DragOverlayWidget {
    background-color: rgba(0, 122, 255, 0.50);
    border: 2px solid #007AFF;
}
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 1. 构建命令
```bash
cmake -B build -S .
cmake --build build -j4
```

### 2. 运行时验收标准
1. 启动 QuarkMeta 应用。
2. 拖拽文件至右侧元数据面板（`MetaPanel`）或左侧筛选器面板（`FilterPanel`）。
3. 观察面板上弹出的遮罩层（`DragOverlayWidget`）：背景呈现为 50% 半透明深蓝色（`rgba(0, 122, 255, 0.50)`），边框为 2px 实线 `#007AFF`。
4. 验证分屏拖拽预览等其他场景遮罩层样式不受任何负面影响。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- 继承使用 `resources/style.qss` 静态 QSS 集中声明体系，不添加内联 C++ 样式代码。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 派生/视图副本 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 拖拽遮罩样式 | `resources/style.qss` | `QWidget#DragOverlayWidget` | QSS 加载 → 控件自动渲染生效 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

- 不改动任何 C++ `.h` / `.cpp` 接口或函数签名。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- 仅涉及 QSS 样式表配置，不涉及 C++ 头文件包含链改变。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 是否满足 | 位置 / 说明 |
| :--- | :--- | :--- | :--- |
| 1 | 是否物理阅读了涉及的 `.h`/`.cpp`/`.qss` 文件？ | 是 | 已全量核查 `resources/style.qss` |
| 2 | SEARCH 块是否在目标文件中真实且唯一存在？ | 是 | 精准匹配 939~942 行 |
| 3 | 调用的外部成员函数是否在 `.h` 中真实存在？ | 是 | 不涉及 C++ 成员调用 |
| 4 | 新增/重构接口的声明定义调用点是否一致？ | 是 | 无接口声明变动 |
| 5 | 头文件包含链改动后 `.cpp` 类型是否闭合？ | 是 | 无 C++ 类型变动 |
| 6 | 新建/删除文件是否在 `CMakeLists.txt` 中注册？ | 是 | 未增删文件 |
| 7 | 跨类调用的成员访问级别是否允许？ | 是 | 不涉及 C++ 成员 |
| 8 | 是否未触及方案范围外的文件？ | 是 | 仅修改 `resources/style.qss` |
| 9 | 方案中是否有未解答的假设或模糊需求？ | 否 | 50% 透明度参数极其明确 |
| 10 | 是否无内联 QSS 样式硬编码违规？ | 是 | 集中写于 `resources/style.qss` |
| 11 | 是否完整包含 10 大必备章节？ | 是 | 完整包含 10 大章节 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 修改点 | 既有行为/数值 | 新行为/数值 | 是否零篡改/符合用户共识 |
| :--- | :--- | :--- | :--- |
| 拖拽遮罩不透明度 | `rgba(0, 122, 255, 0.25)` | `rgba(0, 122, 255, 0.50)` | 精准调整为 50% 不透明度，完全对齐用户指令 |
