# StatusBarTextPadding.md - 状态栏文本底部截断与下沉微调实施方案

- **Base Commit Hash**: `0a8b105ff83e5fef1269d53d8b749cb9053ad646`
- **Prerequisite Plans**: 无
- **Supersedes**: 无

---

## 已读物理证据清单 (Gate 1 Evidence)
1. **物理读取文件列表**：
   - `resources/style.qss`
2. **头文件原样签名核查**：
   - 不涉及 C++ `.h` 接口修改。
3. **既有 API 检索日志**：
   - `grep -rn "StatusBarLeft" resources/style.qss`：精准检索到 `QLabel#StatusBarLeft` 样式节点。

---

## Chapter 1: Overview（概述与根因分析）

### 1. 根因分析
当状态栏数字通过 HTML 富文本（`color:#ff551c; font-weight:bold;`）渲染时，Qt 的富文本排版引擎（`QTextDocument`）与普通纯文本排版的字体基线（Baseline）计算不一致。加粗数字的 Descent（字符下沉量）导致文字整体偏下，贴向 32px 状态栏容器的物理底边，产生视觉上的**底部截断与文字偏沉感**。

### 2. 方案设计
在 `resources/style.qss` 样式表中，为 `QLabel#StatusBarLeft` 及 `QLabel#StatusBarLeft[indexing='true']` 显式补齐 `padding-bottom: 2px;` 底部内边距缓冲，使富文本整体向上抬升 2 像素，在 32px 垂直空间内呈现绝对精准的垂直居中视觉美感。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `resources/style.qss` （修改）

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 1. `resources/style.qss`
```
<<<<<<< SEARCH
QLabel#StatusBarLeft {
    font-size: 11px;
    color: #888888;
    background: transparent;
}
QLabel#StatusBarLeft[indexing='true'] {
    font-size: 11px;
    color: #378ADD;
    background: transparent;
    font-weight: bold;
}
=======
QLabel#StatusBarLeft {
    font-size: 11px;
    color: #888888;
    background: transparent;
    padding-bottom: 2px;
}
QLabel#StatusBarLeft[indexing='true'] {
    font-size: 11px;
    color: #378ADD;
    background: transparent;
    font-weight: bold;
    padding-bottom: 2px;
}
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 1. 构建步骤
```bash
cmake --build build --config Debug
```

### 2. 运行时验收标准
1. **视觉居中效果**：启动软件，观察底部状态栏数字与项目统计文案，文字底部不再贴边或被截断，呈现完美的垂直居中效果。
2. **主题样式无退化**：在状态栏索引状态与普通就绪状态下，文本格式与颜色保持稳定正常。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- 复用外联 QSS 样式表 `resources/style.qss` 统一控制控件 Padding，严禁在 C++ 中内联硬编码 `setStyleSheet`。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 样式控制 |
| :--- | :--- | :--- |
| 状态栏标签内边距 | `resources/style.qss` `#StatusBarLeft` | QSS 选择器统一作用 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

- 本方案不修改任何 C++ 头文件与成员函数签名，接口只读/冻结状态 100% 保留。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- 仅修改 QSS 样式表，不涉及 C++ 头文件包含链。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项 | 结论 | 关联位置/说明 |
| :--- | :--- | :--- | :--- |
| 1 | 方案位于物理隔离目录 | 满足 | `StatusBarTextPadding.md` |
| 2 | SEARCH 块原文存在且唯一 | 满足 | 精准匹配 `resources/style.qss:340-351` |
| 3 | 10 大章节完整 | 满足 | Chapter 1 ~ 10 全齐 |
| 4 | 无 Win32 / Qt::Popup Hack | 满足 | 纯正 QSS 样式属性 |
| 5 | 无主/后台线程 UI 违规 | 满足 | 样式表解析安全 |
| 6 | 样式与数值 100% 照抄微调 | 满足 | 仅补充 `padding-bottom: 2px;` |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

- **交互行为**：状态栏所有控件与功能保持 100% 相同。
- **外观数值**：字体大小、颜色完全保持原样，仅补充 2px 底部安全 Padding。
