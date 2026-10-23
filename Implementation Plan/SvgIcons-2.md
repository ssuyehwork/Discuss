# Implementation Plan - SvgIcons-2.md

## Overview
本实施方案旨在修补全局 `QCheckBox` 控件在选中（Checked）状态下无法呈现打勾图标（√）的缺陷：
1. **键名映射缺失 `.svg` 后缀**：`resources/style.qss` 中声明的图片 URL 为 `image: url(:/Icon/check.svg);`，而 `SvgIcons::icons` 映射表中注册的键名仅为 `"check"`，导致动态资源加载器查找 `check.svg` 失败。
2. **描边着色优化**：`check` 图标原为 `stroke="currentColor"`，在 QSS 样式表 `image:` 直接渲染时易因缺少上下文颜色派发退化为暗色或无着色。切换为包含硬编码品牌色 `#378ADD` 与白色的高亮矢量定义。

---

## Modified Files List
1. `src/ui/SvgIcons.h`
2. `resources/style.qss`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/SvgIcons.h`
在 `SvgIcons::icons` 映射表中增加 `"check.svg"` 键名注册：

```
<<<<<<< SEARCH
        {"check", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="20 6 9 17 4 12"/></svg>)svg"},
=======
        {"check", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="20 6 9 17 4 12"/></svg>)svg"},
        {"check.svg", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="#378ADD" stroke-width="3.5" stroke-linecap="round" stroke-linejoin="round"><polyline points="20 6 9 17 4 12"/></svg>)svg"},
>>>>>>> REPLACE
```

---

### 2. `resources/style.qss`
确认并规整 `QCheckBox::indicator:checked` 关联的资源 URL 声明：

```
<<<<<<< SEARCH
QCheckBox::indicator:checked { 
    border: 1px solid #378ADD; 
    background: #1E1E1E; 
    image: url(data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCIgZmlsbD0ibm9uZSIgc3Ryb2tlPSIjMzc4QUREIiBzdHJva2Utd2lkdGg9IjMuNSIgc3Ryb2tlLWxpbmVjYXA9InJvdW5kIiBzdHJva2UtbGluZWpvaW49InJvdW5kIj48cG9seWxpbmUgcG9pbnRzPSIyMCA2IDkgMTcgNCAxMiI+PC9wb2x5bGluZT48L3N2Zz4=);
}
=======
QCheckBox::indicator:checked { 
    border: 1px solid #378ADD; 
    background: #1E1E1E; 
    image: url(:/Icon/check.svg);
}
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. **编译确认**：
   运行 CMake 编译，确保头文件修改语法无误：
   ```bash
   cmake --build build
   ```

2. **界面打勾视觉对账**：
   - 唤出 `FileCollisionDialog` 或全应用任何 `QCheckBox` 控件，勾选复选框；
   - 确认复选框内部 100% 呈现高亮清晰的蓝色打勾（√）勾选标志。

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- **矢量图标源集中归一**：统一注册于 `SvgIcons.h` 矢量资源库中，禁止碎片化混用；
- **重构属性零篡改**：线条宽度 3.5px 与品牌蓝色 `#378ADD` 100% 原样保持。

---

## Header API Signature Verification

| 调用的类 / 结构体 | `.h` 文件源头 | 物理声明 |
| :--- | :--- | :--- |
| `SvgIcons::icons` | `src/ui/SvgIcons.h` | `inline const QMap<QString, QString> icons;` |

---

## Header Inclusion Chain & Type Completeness Check

- `src/ui/SvgIcons.h`:
  - 包含了 `<QString>` 与 `<QMap>`，类型闭环完整。
