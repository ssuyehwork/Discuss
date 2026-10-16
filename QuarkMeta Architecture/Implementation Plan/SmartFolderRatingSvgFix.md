# Implementation Plan: SmartFolderRatingSvgFix.md

## 1. Overview（概述与解决的问题）
更正 `SmartFolderDialog` 智能文件夹规则配置窗口中星级下拉框项的渲染方式。
彻底摒弃 Unicode 文字字符 `★` 与硬编码文本 `"1星"`，严格遵守 `Memories.md` 第 0 节顶层红线【图标格式限制与星级矢量图标 SSOT 契约】：
> 全应用所有 UI 元素与控件只允许使用 SVG 格式矢量图标，严禁使用任何文本或特殊符号（如 Unicode 符号 `★`、键盘字符等）替代/充当图标！
> 星级必须统一调用 `UiHelper::getIcon("star_filled", ...)` 渲染生成精美的 QIcon 矢量图标填充至 QComboBox 项中。

---

## 2. Modified Files List（影响文件清单）
1. `src/ui/dialogs/SmartFolderDialog.cpp`

---

## 3. Detailed Line-by-Line Changes（包含精准替换块）

### `src/ui/dialogs/SmartFolderDialog.cpp`

```
<<<<<<< SEARCH
        m_valueEdit->hide();
        m_valueCombo->clear();
        m_valueCombo->addItem("1 星", "1");
        m_valueCombo->addItem("2 星", "2");
        m_valueCombo->addItem("3 星", "3");
        m_valueCombo->addItem("4 星", "4");
        m_valueCombo->addItem("5 星", "5");
        m_valueCombo->show();
=======
        m_valueEdit->hide();
        m_valueCombo->clear();
        
        // 遵循 SVG 矢量图标 SSOT 契约，生成 1~5 颗连续 star_filled 矢量图标 QIcon
        for (int r = 1; r <= 5; ++r) {
            int starSize = 12;
            int spacing = 2;
            int totalW = r * starSize + (r - 1) * spacing;
            QPixmap pix(totalW, starSize);
            pix.fill(Qt::transparent);
            QPainter painter(&pix);
            QPixmap starPix = UiHelper::getIcon("star_filled", QColor("#F39C12"), starSize).pixmap(starSize, starSize);
            for (int i = 0; i < r; ++i) {
                painter.drawPixmap(i * (starSize + spacing), 0, starPix);
            }
            painter.end();
            m_valueCombo->addItem(QIcon(pix), "", QString::number(r));
        }
        m_valueCombo->show();
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）
1. **编译命令**：
   ```bash
   cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
   cmake --build build --parallel
   ```
2. **验证步骤**：
   - 运行程序，打开“智能文件夹规则配置”对话框，类型选择“星级”；
   - 检查最右侧下拉选择框，每一项均呈现精致的 1~5 颗橙黄亮色 `star_filled` **矢量图标**（如 `RatingGroup` 呈现的效果），无任何文本或 Unicode 符号混杂，100% 绝对物理对齐。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）
- 100% 复用 `UiHelper::getIcon("star_filled", QColor("#F39C12"), starSize)` 矢量渲染管线，与 `RatingGroup.cpp` 中的矢量图标拼接通道保持物理完全一致，杜绝任何另起炉灶。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）
| 类名 / 模块 | 调用的 API | 所在头文件 | 验证结果 |
| :--- | :--- | :--- | :--- |
| `UiHelper` | `getIcon(const QString& key, const QColor& color, int size)` | `src/ui/UiHelper.h` | 存在且物理一致 |
| `QComboBox` | `addItem(const QIcon& icon, const QString& text, const QVariant& userData)` | `<QComboBox>` | Qt 官方 API |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）
| 修改文件 | 包含的头文件 | 检查情况 |
| :--- | :--- | :--- |
| `src/ui/dialogs/SmartFolderDialog.cpp` | `#include <QPainter>` | 需要显式补充 `#include <QPainter>` |
