# 添加 Monitoring-1.svg 图标实施方案 (Add Monitoring-1 SVG Icon Implementation Plan)

**方案版本**：v1.0  
**日期**：2026-11-07  
**需求描述**：在 `SvgIcons.h` 中添加 `Monitoring-1.svg` 矢量图标，配置 `fill="currentColor"` 满足动态调色需求。

---

## 已读物理证据清单（第 1 关：Gate 1）

| 文件 | 确认已读 | 关键物理签名 |
|---|---|---|
| `QuarkMeta_Source/src/ui/SvgIcons.h` | ✅ | `const QMap<QString, QString> icons = { ... {"Compare.svg", ...} };`（第 15-3224 行） |
| `QuarkMeta_Source/src/ui/SvgIconRenderer.cpp` | ✅ | `svgData.replace("currentColor", color.name());`（第 24 行）表明图标通过 `currentColor` 进行动态调色 |

**防重复造轮子与根因检索记录**：
- `SvgIcons::icons` 映射表为 QuarkMeta 中所有动态矢量图标的唯一 SSOT 真理源；
- `SvgIconRenderer::renderIcon` 通过将 `currentColor` 替换为目标颜色的十六进制值实现动态上色。

---

## Chapter 1: Overview（概述与根因分析）

### 设计目标
将用户提供的监控/目录监听相关的矢量 SVG 图标添加至 `QuarkMeta_Source/src/ui/SvgIcons.h` 中，设置键名为 `"Monitoring-1.svg"`，并在根 `<svg>` 标签中添加 `fill="currentColor"`，使应用层能够通过 `UiHelper::getIcon("Monitoring-1.svg", QColor(...))` 自由动态调色。

---

## Chapter 2: Modified Files List（影响文件清单）

| 操作类型 | 文件相对路径 |
|---|---|
| **修改** | `QuarkMeta_Source/src/ui/SvgIcons.h` |
| 不涉及 | `CMakeLists.txt`（无文件增删） |

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `QuarkMeta_Source/src/ui/SvgIcons.h` — 在图标映射表中追加 `Monitoring-1.svg`

```
<<<<<<< SEARCH
	<polyline class="st0" points="613,460 781.9,460 781.9,386.9 895,500 781.9,613.1 781.9,540 613,540 	"/>
</g>
</svg>)svg"},
    };
}

#endif // SVGICONS_H
=======
	<polyline class="st0" points="613,460 781.9,460 781.9,386.9 895,500 781.9,613.1 781.9,540 613,540 	"/>
</g>
</svg>)svg"},
        {"Monitoring-1.svg", R"svg(<?xml version="1.0" encoding="utf-8"?>
<!-- Generator: Adobe Illustrator 23.0.0, SVG Export Plug-In . SVG Version: 6.00 Build 0)  -->
<svg fill="currentColor" version="1.1" id="图层_1" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" x="0px" y="0px"
	 viewBox="0 0 1000 1000" style="enable-background:new 0 0 1000 1000;" xml:space="preserve">
<path d="M999.5,328.9v109.7h-80.1v-79.8c0-19.8-16-35.8-35.8-35.8H553.2L361.7,813.1h274.5v80.1H86.6c-47.6,0-86.1-38.5-86.1-86l0,0
	V192.8c0-47.5,38.5-86,86-86l0,0h328.1c16.4,0,32.5,6.5,42.8,19.2l0.1,0.1l93.4,115.5l1.1,1.4h361.5
	C961,242.9,999.5,281.4,999.5,328.9L999.5,328.9z"/>
<g>
	<path d="M837.7,668.7"/>
	<path d="M741.9,486.6c-142.6,0-258.1,128.6-258.1,128.6s115.6,128.6,258.1,128.6c142.6,0,258.1-128.6,258.1-128.6
		S884.4,486.6,741.9,486.6z M741.9,689.3c-82,0-148.5-74-148.5-74s66.5-74,148.5-74s148.5,74,148.5,74S823.9,689.3,741.9,689.3z"/>
	<circle cx="741.9" cy="615.3" r="49.9"/>
</g>
</svg>)svg"},
    };
}

#endif // SVGICONS_H
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 编译验证步骤
```bash
# 因沙盒环境中不包含 Qt6 库，跳过实际编译步骤
```

### 运行时验收标准

| 场景 | 操作步骤 | 预期结果 |
|---|---|---|
| 图标键命中 | 调用 `UiHelper::getIcon("Monitoring-1.svg", QColor("#3498db"))` | 返回有效的 `QIcon`，正确渲染出带有监控标识的文件夹图标 |
| 动态调色 | 传入不同 `QColor`（如 `#EEEEEE` 或 `#3498db`） | 图标颜色正确动态变化为所传入的色彩 |

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

| 功能 | 复用的官方 SSOT 入口 | 位置 |
|---|---|---|
| 图标字典 | `SvgIcons::icons` | `SvgIcons.h` |
| 图标渲染引擎 | `SvgIconRenderer::renderIcon` | `SvgIconRenderer.cpp` |

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | SSOT 来源 | 派生/消费点 | 同步方向 |
|---|---|---|---|
| 矢量图标 SVG 数据 | `SvgIcons::icons["Monitoring-1.svg"]` | `SvgIconRenderer` / `UiHelper` | 只读查询 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

| 调用 | 物理签名位置 | 访问级别 |
|---|---|---|
| `SvgIcons::icons` | `SvgIcons.h`，public static const | ✅ public |

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

| 类型/API | `#include` 状态 |
|---|---|
| `QMap<QString, QString>` | `SvgIcons.h` 头部已包含 `<QMap>` 与 `<QString>` ✅ |

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 编号 | 检查项 | 状态 | 位置/说明 |
|---|---|---|---|
| 1 | 方案已通过需求确认 | ✅ 满足 | 用户明确表示“明白用户意图后，直接实施即可” |
| 2 | 物理读取所有涉及文件（Gate 1） | ✅ 满足 | 见已读物理证据清单 |
| 3 | SEARCH 块原文真实存在且唯一匹配 | ✅ 满足 | 逐行核对真实存在且唯一匹配 |
| 4 | 新增接口声明、定义、调用点签名一致 | ✅ 满足 | 静态数据映射项扩展 |
| 5 | 被修改的 `#include` 间接包含链闭合 | ✅ 满足 | 无 `#include` 增删 |
| 6 | 新建/删除文件已在 CMakeLists.txt 同步 | ✅ 不涉及 | 无文件增删 |
| 7 | 跨类调用访问级别均为 public | ✅ 满足 | 均为 public |
| 8 | 方案未触及范围外文件 | ✅ 满足 | 仅修改 `SvgIcons.h` |
| 9 | 未采用笨拙浪费计算资源的手法 | ✅ 满足 | 常量哈希映射 O(1) 查找 |
| 10 | 代码中无打补丁行为 | ✅ 满足 | 直接在全局图标真理源扩充 |
| 11 | 遵循 Memories.md 规范 | ✅ 满足 | 符合 100% 矢量图标规范 |
| 12 | ToolTipOverlay 不追随鼠标 | ✅ 不涉及 | 未调用 ToolTipOverlay |
| 13 | 100% 语义化矢量图标 | ✅ 满足 | 纯 SVG 矢量定义 |
| 14 | 异步回调按值捕获 | ✅ 不涉及 | 无异步回调 |
| 15 | 无 Lambda 循环引用 | ✅ 不涉及 | 无 Lambda |
| 16 | 后台线程 UI 操作回归主线程 | ✅ 不涉及 | 纯静态数据字典 |
| 17 | Qt 信号槽使用 C++11 函数指针语法 | ✅ 不涉及 | 无信号槽 |
| 18 | 对外 `.h` 签名冻结 | ✅ 满足 | 不改变任何已有公开函数签名 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 行为零篡改声明

| 既有行为 | 本方案影响 | 结论 |
|---|---|---|
| 既有所有图标的读取与渲染 | 仅新增 1 个字典键值对，已有所有图标完全不受影响 | ✅ 100% 原样保留 |

### 数值零篡改声明

| 数值类型 | 值 | 来源对齐 |
|---|---|---|
| viewBox | `0 0 1000 1000` | 原样照抄用户提供的 SVG 属性 |
| path / circle 路径坐标 | 100% 原样照抄 | 无任何坐标变动 |
