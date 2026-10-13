# Implementation Plan - FileCollisionDialog-2.md

## Overview
本实施方案旨在解决文件同名冲突对话框 (`FileCollisionDialog`) 中的 UI 视觉与交互体验问题：
1. **按钮文字截断**：调整 `FileCollisionDialog.cpp` 中胶囊按钮的 `minimumWidth` 以及 `resources/style.qss` 中的 padding 与文本对齐方式，解决“同存”、“替换”、“跳过”、“取消”按钮内文字被边界挤压截断缺陷。

---

## Modified Files List
1. `src/ui/FileCollisionDialog.cpp`
2. `resources/style.qss`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/FileCollisionDialog.cpp`
调整胶囊按钮宽度限制为 `76px`，确保中文字符舒适展露不被截断：

```
<<<<<<< SEARCH
    auto createCapsuleBtn = [this, area](const QString& text, CollisionResolveAction action) {
        QPushButton* btn = new QPushButton(text, area);
        btn->setFixedHeight(28);
        btn->setMinimumWidth(72);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setObjectName("CollisionOptionBtn");
        connect(btn, &QPushButton::clicked, this, [this, action]() {
            m_selectedAction = action;
            accept();
        });
        return btn;
    };
=======
    auto createCapsuleBtn = [this, area](const QString& text, CollisionResolveAction action) {
        QPushButton* btn = new QPushButton(text, area);
        btn->setFixedHeight(28);
        btn->setMinimumWidth(76);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setObjectName("CollisionOptionBtn");
        connect(btn, &QPushButton::clicked, this, [this, action]() {
            m_selectedAction = action;
            accept();
        });
        return btn;
    };
>>>>>>> REPLACE
```

---

### 2. `resources/style.qss`
规整 `CollisionOptionBtn` 按钮 Padding 与居中文本：

```
<<<<<<< SEARCH
QPushButton#CollisionOptionBtn {
    background-color: #2D2D30;
    color: #EEEEEE;
    border: 1px solid #3E3E42;
    border-radius: 4px;
    padding: 8px 12px;
    text-align: left;
}
=======
QPushButton#CollisionOptionBtn {
    background-color: #2D2D30;
    color: #EEEEEE;
    border: 1px solid #3E3E42;
    border-radius: 4px;
    padding: 2px 10px;
    text-align: center;
}
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. **编译验证**：
   运行 CMake 编译，确保无样式文件语法或代码编译错误：
   ```bash
   cmake --build build
   ```

2. **功能与视觉对账**：
   - 唤出 `FileCollisionDialog` 弹窗，核查“同存”、“替换”、“跳过”、“取消”四个按钮字样无截断，居中完整显示。

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- **样式表集中统一**：所有样式修改均收拢至 `resources/style.qss`，禁止 C++ 内联样式；
- **参数零篡改**：按钮底色 `#2D2D30` 与高亮边框 `#3E3E42` 完全原样保持。

---

## Header API Signature Verification

| 调用的类 / 成员函数 | `.h` 文件源头 | 物理精确签名 |
| :--- | :--- | :--- |
| `FileCollisionDialog::FileCollisionDialog(...)` | `src/ui/FileCollisionDialog.h` | `explicit FileCollisionDialog(const QString& firstFileName, const QString& targetDirName, int totalCount, QWidget* parent = nullptr);` |

---

## Header Inclusion Chain & Type Completeness Check

- `src/ui/FileCollisionDialog.cpp`:
  - 包含了 `FileCollisionDialog.h`、`UiHelper.h`、`QVBoxLayout`、`QHBoxLayout`、`QLabel`、`QFileInfo`、`QDir`。
  - 所有用到的类与枚举类型定义闭环完整。
