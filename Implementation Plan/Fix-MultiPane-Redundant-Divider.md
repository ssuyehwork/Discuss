# 实施方案：多窗格模式冗余分割线消除

> **方案编号**：Fix-MultiPane-Redundant-Divider
> **影响文件**：`resources/style.qss`、`src/ui/controllers/ContentPaneSplitManager.cpp`
> **风险等级**：低（纯视觉层修复，不触碰任何业务逻辑）

---

## 一、架构三问（门禁锁）

### 1. SSOT 溯源
- **分割线视觉规范真理源**：`Memories.md §13.5`——"分屏状态下，当前被激活/聚焦的那个窗格需要有明确的**边框高亮**；**单窗格模式下不需要、也不应该显示**这条焦点边框"
- **栏间切缝真理源**：`style.qss` 注释（L107）——"由 QSplitter 5px 句柄原生提供栏间 5px 物理间距"
- **样式修改真理源**：`resources/style.qss`（QSS 负责静态样式声明）

### 2. 黑盒完整性
仅修改 QSS 样式规则与 `m_primaryPaneContainer` 的 `objectName`，不改变任何信号、槽、业务逻辑或接口签名。

### 3. 根因溯源（已完成物理核查）
见第二节。

---

## 二、根因完整证据链

### 根因 A：`m_primaryPaneContainer` 与 `ContentPanel` 同名 `#EditorContainer` → 嵌套双层边框

**物理证据：**
```cpp
// ContentPanel.cpp:57 — ContentPanel 自身命名
setObjectName("EditorContainer");  // 自身 border: 1px solid #333333

// ContentPaneSplitManager.cpp:521-522 — 开启分屏时的包装盒
m_primaryPaneContainer = new QFrame(m_panel);
m_primaryPaneContainer->setObjectName("EditorContainer");  // 包装盒也叫 EditorContainer！
```

**叠加效果：**
分屏开启后，主窗格的实际 Widget 树变为：

```
ContentPanel[#EditorContainer, border:1px]       ← 宿主面板（isHostPanel=true → border:none ✅）
  └─ QSplitter
       ├─ QFrame[#EditorContainer, border:1px]   ← m_primaryPaneContainer（包装盒，BORDER 未消除 ❌）
       │    └─ ContentPanel[#EditorContainer, border:1px]  ← 主窗格本体（BORDER 冗余 ❌）
       └─ QWidget[无 objectName]
            └─ ContentPanel[#EditorContainer, border:1px]  ← 副窗格本体（BORDER 冗余 ❌）
```

### 根因 B：副窗格 `ContentPanel` 没有任何 QSS 条件消除其边框

副窗格的 `ContentPanel` 同样名为 `#EditorContainer`，继承 `border: 1px solid #333333`，且没有 `isHostPanel` 属性，现有补丁 `#EditorContainer[isHostPanel="true"] { border: none; }` 无法覆盖它。

### 根因 C：QSplitter handle 缺少 `vertical` 方向定义

QSplitter 句柄只有 `:horizontal` 的样式，无 `:vertical` 定义，垂直分屏时句柄背景回退为系统默认，颜色不一致。

### 根因 D：多窗格模式下焦点激活边框实现不完整

`Memories.md §13.5` 要求分屏状态下激活窗格有明确边框高亮，但当前 QSS 只在 `QLabel#ContentHeaderTitle[activePane="true"]` 上变色，**窗格容器本身的边框高亮未在 QSS 中声明**（代码里 `setPaneActiveProperty` 设置了 `activePane` 属性，但 QSS 里缺对应的容器边框规则）。

---

## 三、能力对照表（修复前 vs 修复后）

| 场景 | 修复前 | 修复后 |
|---|---|---|
| 单窗格 `ContentPanel` 边框 | 1px ✅ | 1px ✅（不变） |
| 分屏：两窗格相邻处视觉宽度 | 1px+5px+1px = **7px（变粗）** ❌ | **5px（只有 QSplitter 句柄）** ✅ |
| 分屏：激活窗格高亮 | 仅标题文字变色，容器无边框高亮 ❌ | 容器整体 `border: 1px solid #378ADD`（蓝色高亮）✅ |
| 分屏：非激活窗格边框 | 有 1px 灰边框（与单窗格无区别）❌ | `border: none`（完全无边框，由 QSplitter 提供切缝）✅ |
| 垂直分屏句柄颜色 | 系统默认（不可控）❌ | `#1E1E1E` 与水平一致 ✅ |
| `m_primaryPaneContainer` 同名问题 | `objectName = "EditorContainer"` → 继承边框 ❌ | `objectName = "PrimaryPaneContainer"` → 不继承边框规则 ✅ |

---

## 四、行为变化清单（⚠️ 已明确列出，必须用户确认后方可实施）

1. **【行为变化】多窗格模式下，非激活窗格不再有灰色边框（`border: none`）**
   → 视觉上窗格间的分隔完全由 QSplitter 5px 句柄承担，干净清晰。

2. **【行为变化】多窗格模式下，激活窗格新增蓝色边框高亮（`border: 1px solid #378ADD`）**
   → 与现有激活蓝色（`#378ADD`，已用于 QSplitter handle:hover）保持品牌一致。

3. **【行为变化】`m_primaryPaneContainer` 的 `objectName` 由 `"EditorContainer"` 改为 `"PrimaryPaneContainer"`**
   → 该名称仅用于内部 QSS 选择器，不对外暴露任何 API 接口，不影响业务逻辑。QSS 中同步新增对应的背景色规则，视觉完全不变。

4. **【不变】单窗格模式**下 `ContentPanel` 的 `border: 1px solid #333333` 完全不变。

5. **【不变】QSplitter 句柄 5px 物理间距**不变，仅补充 `:vertical` 方向定义。

---

## 五、修改范围（外科手术级，严格限定）

### 文件 1：`resources/style.qss`

#### 改动 1-A：补全 `QSplitter::handle:vertical` 定义

**位置**：第 99–105 行，`QSplitter::handle:horizontal` 规则块之后

**修改前：**
```css
QSplitter::handle:horizontal {
    background-color: #1E1E1E;
    width: 5px;
}
QSplitter::handle:horizontal:hover {
    background-color: #378ADD;
}
```

**修改后：**
```css
QSplitter::handle:horizontal {
    background-color: #1E1E1E;
    width: 5px;
}
QSplitter::handle:horizontal:hover {
    background-color: #378ADD;
}
QSplitter::handle:vertical {
    background-color: #1E1E1E;
    height: 5px;
}
QSplitter::handle:vertical:hover {
    background-color: #378ADD;
}
```

---

#### 改动 1-B：将 `#EditorContainer[isHostPanel="true"]` 补丁替换为完整的多窗格边框管理规则

**位置**：第 142–144 行，现有 `isHostPanel` 补丁块

**修改前：**
```css
#EditorContainer[isHostPanel="true"] {
    border: none;
}
```

**修改后：**
```css
/* 多窗格宿主面板：外层边框由内部 QSplitter 句柄承担，宿主自身无边框 */
#EditorContainer[isHostPanel="true"] {
    border: none;
}

/* 多窗格模式下：非激活窗格无边框（避免与 QSplitter 5px 句柄双重叠加变粗） */
#EditorContainer[activePane="false"] {
    border: none;
}

/* 多窗格模式下：激活窗格显示蓝色边框高亮，帮助用户识别当前操作窗格 */
#EditorContainer[activePane="true"] {
    border: 1px solid #378ADD;
}

/* 主窗格包装盒：仅继承背景色，无独立边框（边框统一由内部 ContentPanel 管理） */
#PrimaryPaneContainer {
    background-color: #1E1E1E;
    border: none;
    margin: 0px;
    padding: 0px;
}
```

---

### 文件 2：`src/ui/controllers/ContentPaneSplitManager.cpp`

#### 改动 2-A：`splitPane()` 中 `m_primaryPaneContainer` 的 `objectName` 修改

**位置**：第 521–522 行

**修改前：**
```cpp
m_primaryPaneContainer = new QFrame(m_panel);
m_primaryPaneContainer->setObjectName("EditorContainer");
```

**修改后：**
```cpp
m_primaryPaneContainer = new QFrame(m_panel);
m_primaryPaneContainer->setObjectName("PrimaryPaneContainer");
```

---

#### 改动 2-B：`importNodeRecursive()` 中 `m_primaryPaneContainer` 的 `objectName` 修改

**位置**：第 435–436 行（`importNodeRecursive` 分支，状态恢复路径）

**修改前：**
```cpp
m_primaryPaneContainer = new QFrame(parentWidget);
m_primaryPaneContainer->setObjectName("EditorContainer");
```

**修改后：**
```cpp
m_primaryPaneContainer = new QFrame(parentWidget);
m_primaryPaneContainer->setObjectName("PrimaryPaneContainer");
```

---

#### 改动 2-C：`splitPane()` 开启分屏时，为主窗格 `ContentPanel` 设置初始 `activePane` 属性

**位置**：第 513–517 行（`m_isSplit = true;` 之后的属性刷新块）

**修改前：**
```cpp
m_isSplit = true;
m_panel->setProperty("isHostPanel", "true");
m_panel->style()->unpolish(m_panel);
m_panel->style()->polish(m_panel);
```

**修改后：**
```cpp
m_isSplit = true;
m_panel->setProperty("isHostPanel", "true");
// 初始化 activePane 属性，确保 QSS 选择器在 refreshActiveIndicators() 前有明确状态
m_panel->setProperty("activePane", "false");
m_panel->style()->unpolish(m_panel);
m_panel->style()->polish(m_panel);
```

---

## 六、三道关自检

### 第一关：物理签名核查
- [x] 无任何 `.h` 文件被修改
- [x] 无任何公开接口签名被改变
- [x] `objectName` 改动为内部实现细节，不暴露给调用方

### 第二关：架构五锁核查
- [x] 【契约锁】对外接口签名冻结：未改任何 `.h`
- [x] 【依赖锁】单向依赖流动：修改仅在 View 层（QSS + 视图容器 objectName）
- [x] 【静态红线扫描】无平台 Hack：无 Win32 API，无 `Qt::Popup` 篡改
- [x] 【黑盒隔离锁】仅修改接口（QSS 选择器 + objectName），不窥探私有实现
- [x] 【门禁锁三问】已答：SSOT 为 style.qss + Memories.md §13.5，黑盒完整，根因已溯源

### 第三关：行为变化 100% 事先确认
- [ ] 非激活窗格 `border: none` — 已列出，**等待用户确认**
- [ ] 激活窗格 `border: 1px solid #378ADD` — 已列出，**等待用户确认**
- [ ] `PrimaryPaneContainer` objectName 变更 — 已列出，**等待用户确认**
- [x] 单窗格模式视觉完全不变 — 已确认（无改动）
- [x] 无任何静默变更

---

## 七、修改后预期视觉效果

```
单窗格模式（完全不变）：
┌──────────────────────────────────────────────────┐
│  ContentPanel  border: 1px solid #333333          │
└──────────────────────────────────────────────────┘

双窗格模式（修复后）：
┌──────────────────────┬─────┬──────────────────────┐
│ ContentPanel         │     │ ContentPanel          │
│ activePane="true"    │ 5px │ activePane="false"    │
│ border:1px #378ADD   │句柄 │ border: none          │
└──────────────────────┴─────┴──────────────────────┘
      激活蓝边框          QSplitter      非激活无边框
      （总宽 = 1px 蓝框 + 5px 句柄，干净不叠加）
```

---

## 八、回滚方案

全部改动为纯 QSS 文本替换 + 两处 C++ 字符串常量修改，任何时刻可通过 Git revert 单文件回滚，无数据风险。

---

*方案起草完毕。所有行为变化已在第四节完整列出，等待用户审阅并明确批准后方可实施。*
