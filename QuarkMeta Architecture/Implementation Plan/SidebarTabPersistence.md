# SidebarTabPersistence.md - 侧边栏“收藏夹/库” Tab 选中状态 AppConfig 持久化实施方案

## Chapter 1: Pre-Draft Evidence（第 1 关：起草前核查）

### 1. 已读文件清单
- `src/ui/SidebarContainerWidget.h`（已用工具物理读取全文件）
- `src/ui/SidebarContainerWidget.cpp`（已用工具物理读取全文件）
- `src/core/AppConfig.h`（已核查持久化读写签名）

### 2. 物理头文件签名原文摘录
- `AppConfig.h`:
  `QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant()) const;`
  `void setValue(const QString& key, const QVariant& value);`
  `void sync();`
- `SidebarContainerWidget.h`:
  `explicit SidebarContainerWidget(QWidget* parent = nullptr);`
  `void initUi();`

### 3. 既有 API 检索记录
- 使用 `git grep "AppConfig"` 检索到全工程使用 `"Sidebar/"` 或 `"MainWindow/"` 命名空间管理持久化配置。全应用通过 `AppConfig::instance()` 通道统一读写持久化配置。

---

## Chapter 2: Overview（概述与解决的问题）

### 核心问题
当前侧边栏 `SidebarContainerWidget` 在构建 UI 时，`m_tabBar`（“收藏夹”与“库”）默认固定选中第 0 项（“收藏夹”）。用户在运行期间切换至“库”Tab 后，由于缺乏 `AppConfig` 持久化读写逻辑，软件重新启动或重建组件时该状态彻底丢失，强行还原至“收藏夹”，存在 UI 选中状态持久化遗漏缺陷。

### 重构方案
1. **自动恢复上次 Tab 索引**：在 `SidebarContainerWidget::initUi()` 初始化末尾，从 `AppConfig::instance()` 读取 `"Sidebar/CurrentTabIndex"`（默认值为 `0`），调用 `m_tabBar->setCurrentIndex(savedIndex)` 恢复上一次离开时的选择。
2. **切换即时持久化**：在 `m_tabBar` 的 `currentChanged` 信号响应回调中，增加 `AppConfig::instance().setValue("Sidebar/CurrentTabIndex", index)` 并调用 `sync()` 持久化落地，保证 SSOT 状态一致。

---

## Chapter 3: Modified Files List（影响文件清单）
1. `src/ui/SidebarContainerWidget.cpp`（引入 `#include "../core/AppConfig.h"`，添加索引恢复与持久化保存逻辑）

---

## Chapter 4: Detailed Line-by-Line Changes（精准替换块）

### 1. `src/ui/SidebarContainerWidget.cpp`
```
<<<<<<< SEARCH
#include "SidebarContainerWidget.h"
#include "UiHelper.h"
=======
#include "SidebarContainerWidget.h"
#include "UiHelper.h"
#include "../core/AppConfig.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    connect(m_tabBar, &QTabBar::currentChanged, this, [this](int index) {
        m_stackedWidget->setCurrentIndex(index);
        if (index == 1 && m_libraryPanel) {
            m_libraryPanel->loadLibrary();
        }
        emit sidebarTabChanged(index);
    });
}
=======
    connect(m_tabBar, &QTabBar::currentChanged, this, [this](int index) {
        m_stackedWidget->setCurrentIndex(index);
        if (index == 1 && m_libraryPanel) {
            m_libraryPanel->loadLibrary();
        }
        AppConfig::instance().setValue("Sidebar/CurrentTabIndex", index);
        AppConfig::instance().sync();
        emit sidebarTabChanged(index);
    });

    int savedIndex = AppConfig::instance().getValue("Sidebar/CurrentTabIndex", 0).toInt();
    if (savedIndex >= 0 && savedIndex < m_tabBar->count()) {
        m_tabBar->setCurrentIndex(savedIndex);
    }
}
>>>>>>> REPLACE
```

---

## Chapter 5: Build & Verification Steps（编译命令与验证方法）

### 编译步骤
```bash
cmake -B build -G "Ninja"
cmake --build build --config Release
```

### 验证方法
1. **侧边栏 Tab 切换持久化验证**：
   - 启动软件，点击侧边栏顶部的“库”Tab。
   - 关闭软件，重新启动软件。
   - **预期结果**：软件启动后侧边栏自动处于“库”Tab 选中状态，相关库分类列表正常加载。

---

## Chapter 6: SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用自查）
- 严格走既有全局配置通道 `AppConfig::instance()` 进行读写，与其他 UI 状态持久化方式保持 100% 一致。

---

## Chapter 7: Header API Signature Verification（头文件 API 物理签名核查表）

| 调用成员 / 函数 | 所在头文件 | 物理精确签名 | 核查状态 |
|---|---|---|---|
| `AppConfig::getValue` | `src/core/AppConfig.h` | `QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant()) const;` | 物理存在一致 |
| `AppConfig::setValue` | `src/core/AppConfig.h` | `void setValue(const QString& key, const QVariant& value);` | 物理存在一致 |
| `AppConfig::sync` | `src/core/AppConfig.h` | `void sync();` | 物理存在一致 |

---

## Chapter 8: Header Inclusion Chain & Type Completeness Check（头文件包含链核查表）
- `SidebarContainerWidget.cpp` 中新增 `#include "../core/AppConfig.h"`，类型声明完备闭合。

---

## Chapter 9: Compliance Self-Check Table（第 2 关：交付前自检）

| 检查项 | 来源条款 | 结论 | 对应方案位置 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | `SidebarTabPersistence.md` 对应 `SidebarContainerWidget` |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | `QuarkMeta Architecture/Implementation Plan/SidebarTabPersistence.md` |
| 所有 SEARCH 块代码在仓库真实存在 | 3.2 | 满足 | 第 4 章 SEARCH 块逐字存在于源码中 |
| 第 1~10 章齐全 | 3.4 | 满足 | 第 1~10 章齐全 |
| 已复用既有 SSOT 入口，无另起炉灶 | 2.4 | 满足 | 第 6 章 |

---

## Chapter 10: Zero-Value-Alteration Check（数值零篡改核对）
- 本方案不涉及任何 UI 尺寸或样式参数变动，填写：**不涉及**。
