# 盘符栏彻底根除与“标签管理”SVG按钮引入实施方案 (Drive Bar Purge & Tag Manager SVG Button Implementation Plan)

## 1. Overview（概述与解决的问题）

本实施方案旨在彻底根除顶部无用/冗余的本地盘符按钮栏（`C:`, `G:`, `H:` 等），并在其原始位置（顶部快捷工具栏）引入符合系统暗黑主题标准的 **“标签管理”矢量按钮**：
1. **盘符栏代码彻底根除**：清理 `MainWindow.cpp` / `MainWindow.h` 中通过 `QDir::drives()` 动态生成 `DriveButton` 盘符列表及映射表 `m_driveButtons` 的所有旧代码，杜绝冗余代码与空间浪费。
2. **“标签管理” SVG 矢量按钮引入**：在原盘符栏位置新建 `m_btnTagManager` 按钮，图标**严格使用 SVG 矢量渲染器** `UiHelper::getIcon("tag", QColor("#1abc9c"), 18)`，外观与配色完全契合系统暗黑风格。点击按钮后，直接触发弹出全功能无边框标签管理对话框 `TagManagerDialog::showDialog(this, m_currentPath, false)`。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/MainWindow.h`
2. `src/ui/MainWindow.cpp`
3. `QuarkMeta Architecture/QuarkMeta-Architecture-Planning.md`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/MainWindow.h`
在 `MainWindow.h` 中移除盘符映射表 `m_driveButtons` 声明，新增 `m_btnTagManager` 按钮指针。

```
<<<<<<< SEARCH
    QWidget* m_driveBarWidget = nullptr;
    QHBoxLayout* m_driveBarLayout = nullptr;
    QMap<QString, class DriveButton*> m_driveButtons;
=======
    QWidget* m_driveBarWidget = nullptr;
    QHBoxLayout* m_driveBarLayout = nullptr;
    QPushButton* m_btnTagManager = nullptr;
>>>>>>> REPLACE
```

---

### 3.2 `src/ui/MainWindow.cpp`
在 `MainWindow.cpp` 的 `initDriveBar()` 函数中，抹除遍历 `QDir::drives()` 生成盘符按钮的代码，改为创建带有 SVG 图标的“标签管理”按钮，并绑定点击触发 `TagManagerDialog::showDialog` 弹出逻辑。

```
<<<<<<< SEARCH
#include "DriveButton.h"
=======
#include "DriveButton.h"
#include "TagManagerDialog.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    auto drives = QDir::drives();
    for (const QFileInfo& drive : drives) {
        QString letter = drive.absolutePath().left(2);
        if (letter.endsWith("/")) letter = letter.left(1) + ":";
        
        DriveButton* btn = new DriveButton(letter, m_driveBarWidget);
        m_driveButtons[letter] = btn;
        m_driveBarLayout->addWidget(btn);

        connect(btn, &QPushButton::clicked, this, [this, letter]() {
            unifiedNavigateTo(letter + "/");
        });
    }
    m_driveBarLayout->addStretch();
=======
    // 🚨 按照用户要求：彻底根除盘符按钮生成逻辑，替换为使用 SVG 矢量图标的“标签管理”按钮
    m_btnTagManager = new QPushButton(UiHelper::getIcon("tag", QColor("#1abc9c"), 18), " 标签管理", m_driveBarWidget);
    m_btnTagManager->setFixedHeight(32);
    m_btnTagManager->setCursor(Qt::PointingHandCursor);
    m_btnTagManager->setStyleSheet(QString(
        "QPushButton { background-color: %1; border: 1px solid %2; border-radius: 4px; padding: 0 12px; color: %3; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background-color: %4; border-color: #1abc9c; color: #FFFFFF; }"
        "QPushButton:pressed { background-color: %5; }"
    ).arg(qssColor(BackgroundSecondary))
     .arg(qssColor(BorderColor))
     .arg(qssColor(TextMain))
     .arg(qssColor(BackgroundHover))
     .arg(qssColor(BackgroundActive)));

    connect(m_btnTagManager, &QPushButton::clicked, this, [this]() {
        TagManagerDialog::showDialog(this, m_currentPath, false);
    });

    m_driveBarLayout->addWidget(m_btnTagManager);
    m_driveBarLayout->addStretch();
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **编译确认**：
   在命令行运行 CMake 编译，验证 `MOC` 与全工程无符号缺失错误：
   ```bash
   cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ```
2. **功能验证**：
   - **盘符移除确认**：启动界面后，观察顶部栏原 `C:` `G:` `H:` `I:` `Z:` 盘符按钮全部消失。
   - **标签管理 SVG 按钮验证**：观察顶部栏左侧成功出现带绿色 SVG 标签矢量图标的“标签管理”按钮。
   - **点击触发验证**：点击“标签管理”按钮，确认立刻弹出现无边框标签管理窗口（`TagManagerDialog`），功能正常。
