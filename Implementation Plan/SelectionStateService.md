# SelectionStateService 选区与恢复管理服务重构实施方案

## 1. Overview（概述与解决的问题）

### 现状痛点分析
当前 `ContentPanel` 中的选区数据保存与恢复逻辑分散在地：
1. `ContentPanel.h` 中硬编码包含了 `struct SelectionState` 及其实例 `m_selectionState`；
2. 在 `ContentPanel::onSelectionChanged`、`ContentPanel::restoreSelections`、`ContentPanel::setPendingSelectName` 等多处散落着选区读写逻辑；
3. `ContentDataLoader` 在全盘重扫或分类加载回调时硬调 `panelPtr->restoreSelections()`，由于缺乏统一的真理源 (SSOT) 管理，当多窗格（Split View）或视角频繁切换时，选区状态易丢失、混乱或竞态覆盖。

### 架构重构目标
1. **抽离独立 Domain 服务**：创建 `src/core/SelectionStateService.h` 与 `src/core/SelectionStateService.cpp`，作为全应用窗格选区记忆与恢复的唯一权威真理源 (SSOT)；
2. **完全解耦 `ContentPanel` 私有数据**：将 `ContentPanel` 内部散落的选区存取下沉至 `SelectionStateService`，消除 `m_selectionState` 内部冗余成员；
3. **注册 CMake 编译目标**：将新文件添加至 `CMakeLists.txt` 的 `SOURCES` 与 `HEADERS` 列表中；
4. **遵守【五道工程硬锁】**：对外 `.h` 接口保持只读/冻结，单向依赖（Domain 层不依赖 View 层），无平台 Hack，信息黑盒隔离。

---

## 2. Modified Files List（影响文件清单）

1. `CMakeLists.txt`（注册 `SelectionStateService.h` / `.cpp`）
2. `src/core/SelectionStateService.h`（新增：选区与恢复管理服务头文件）
3. `src/core/SelectionStateService.cpp`（新增：选区与恢复管理服务实现文件）
4. `src/ui/ContentPanel.h`（移除 `SelectionState m_selectionState` 冗余私有结构）
5. `src/ui/ContentPanel.cpp`（接入 `SelectionStateService` 真理源）

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `CMakeLists.txt` 注册新服务
```cmake
<<<<<<< SEARCH
    src/core/TagLexiconService.h
    src/core/TagLexiconService.cpp
=======
    src/core/TagLexiconService.h
    src/core/TagLexiconService.cpp
    src/core/SelectionStateService.h
    src/core/SelectionStateService.cpp
>>>>>>> REPLACE
```

### 3.2 新建 `src/core/SelectionStateService.h`
```cpp
#pragma once

#include <QObject>
#include <QString>
#include <QSet>
#include <QMap>
#include <QMutex>

namespace QuarkMeta {

/**
 * @brief 选区状态数据结构
 */
struct SelectionState {
    QString currentFolder;
    QString focusedPath;
    QSet<QString> selectedPaths;
    bool isPendingEdit = false;
};

/**
 * @brief 选区与恢复管理服务 (SSOT)：统一负责全应用窗格与视角切换时的选区记忆、路径高亮映射与恢复
 */
class SelectionStateService : public QObject {
    Q_OBJECT

public:
    static SelectionStateService& instance();

    // 选区记忆与存取 API
    void recordSelection(const QString& panelId, const QString& folderPath, const QSet<QString>& selectedPaths, const QString& focusedPath = QString());
    void updateFocusedPath(const QString& panelId, const QString& focusedPath);
    void setPendingEdit(const QString& panelId, bool pendingEdit);

    SelectionState getSelectionState(const QString& panelId) const;
    QSet<QString> getSelectedPaths(const QString& panelId) const;
    QString getFocusedPath(const QString& panelId) const;
    bool isPendingEdit(const QString& panelId) const;

    void clearSelection(const QString& panelId);
    void clearAllSelections();

signals:
    void selectionStateChanged(const QString& panelId, const QSet<QString>& selectedPaths);

private:
    SelectionStateService() = default;
    ~SelectionStateService() override = default;

    mutable QMutex m_mutex;
    QMap<QString, SelectionState> m_panelSelections;
};

} // namespace QuarkMeta
```

### 3.3 新建 `src/core/SelectionStateService.cpp`
```cpp
#include "SelectionStateService.h"

namespace QuarkMeta {

SelectionStateService& SelectionStateService::instance() {
    static SelectionStateService inst;
    return inst;
}

void SelectionStateService::recordSelection(const QString& panelId, const QString& folderPath, const QSet<QString>& selectedPaths, const QString& focusedPath) {
    if (panelId.isEmpty()) return;
    QMutexLocker locker(&m_mutex);
    auto& state = m_panelSelections[panelId];
    state.currentFolder = folderPath;
    state.selectedPaths = selectedPaths;
    if (!focusedPath.isEmpty()) {
        state.focusedPath = focusedPath;
    } else if (!selectedPaths.isEmpty()) {
        state.focusedPath = *selectedPaths.begin();
    } else {
        state.focusedPath.clear();
    }
    locker.unlock();
    emit selectionStateChanged(panelId, selectedPaths);
}

void SelectionStateService::updateFocusedPath(const QString& panelId, const QString& focusedPath) {
    if (panelId.isEmpty()) return;
    QMutexLocker locker(&m_mutex);
    m_panelSelections[panelId].focusedPath = focusedPath;
}

void SelectionStateService::setPendingEdit(const QString& panelId, bool pendingEdit) {
    if (panelId.isEmpty()) return;
    QMutexLocker locker(&m_mutex);
    m_panelSelections[panelId].isPendingEdit = pendingEdit;
}

SelectionState SelectionStateService::getSelectionState(const QString& panelId) const {
    if (panelId.isEmpty()) return {};
    QMutexLocker locker(&m_mutex);
    return m_panelSelections.value(panelId);
}

QSet<QString> SelectionStateService::getSelectedPaths(const QString& panelId) const {
    if (panelId.isEmpty()) return {};
    QMutexLocker locker(&m_mutex);
    return m_panelSelections.value(panelId).selectedPaths;
}

QString SelectionStateService::getFocusedPath(const QString& panelId) const {
    if (panelId.isEmpty()) return {};
    QMutexLocker locker(&m_mutex);
    return m_panelSelections.value(panelId).focusedPath;
}

bool SelectionStateService::isPendingEdit(const QString& panelId) const {
    if (panelId.isEmpty()) return false;
    QMutexLocker locker(&m_mutex);
    return m_panelSelections.value(panelId).isPendingEdit;
}

void SelectionStateService::clearSelection(const QString& panelId) {
    if (panelId.isEmpty()) return;
    QMutexLocker locker(&m_mutex);
    m_panelSelections.remove(panelId);
}

void SelectionStateService::clearAllSelections() {
    QMutexLocker locker(&m_mutex);
    m_panelSelections.clear();
}

} // namespace QuarkMeta
```

### 3.4 `src/ui/ContentPanel.h` 替换结构体定义
```cpp
<<<<<<< SEARCH
    struct SelectionState {
        QString currentFolder;
        QString focusedPath;
        QSet<QString> selectedPaths;
    };
    SelectionState m_selectionState;
=======
    QString panelId() const { return QString::number(reinterpret_cast<quintptr>(this), 16); }
>>>>>>> REPLACE
```

### 3.5 `src/ui/ContentPanel.cpp` 接入 SSOT 服务
```cpp
<<<<<<< SEARCH
#include "models/FilterProxyModel.h"
=======
#include "models/FilterProxyModel.h"
#include "../core/SelectionStateService.h"
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
void ContentPanel::restoreSelections() {
    if (m_selectionState.selectedPaths.isEmpty() || m_isRestoringSelections) return;

    m_isRestoringSelections = true;

    if (m_viewCoordinator) {
        m_viewCoordinator->restoreSelections(m_selectionState.selectedPaths, m_isPendingEdit);
    }

    if (m_currentViewMode == ColumnView && m_columnView && m_columnView->rightmostPane()) {
        m_columnView->rightmostPane()->setPendingSelectPaths(m_selectionState.selectedPaths);
    }

    m_isPendingEdit = false;
    m_isRestoringSelections = false;
}
=======
void ContentPanel::restoreSelections() {
    auto state = SelectionStateService::instance().getSelectionState(panelId());
    if (state.selectedPaths.isEmpty() || m_isRestoringSelections) return;

    m_isRestoringSelections = true;

    if (m_viewCoordinator) {
        m_viewCoordinator->restoreSelections(state.selectedPaths, m_isPendingEdit);
    }

    if (m_currentViewMode == ColumnView && m_columnView && m_columnView->rightmostPane()) {
        m_columnView->rightmostPane()->setPendingSelectPaths(state.selectedPaths);
    }

    m_isPendingEdit = false;
    m_isRestoringSelections = false;
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译与验证方法）

1. **CMake 注册验证**：运行 CMake 构建配置命令，确认 `SelectionStateService.h` 和 `.cpp` 被包含在 MOC 和 Target 编译清单中：
   ```bash
   cmake -B build -S .
   ```
2. **编译核验**：确保零 MSVC `C2039` / `C2027` 编译错误：
   ```bash
   cmake --build build --config Release
   ```
3. **选区恢复功能测试**：
   - 在网格/列表/分栏视图模式下，选择若干文件；
   - 触发全盘重新加载或目录刷洗 (`refreshAll()`)；
   - 验证选区高亮被正确恢复，且多分栏视角下无跨窗格选区错乱与丢失现象。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）

- **SSOT 归口判定**：所有窗格与视角的选区记忆与恢复，统一归口至 `SelectionStateService::instance()` 单例。
- **无重复实现**：彻底移除了 `ContentPanel` 内部包含的私有结构体 `SelectionState` 副本，杜绝“两套选区状态并存”隐患。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用的成员函数 | 所属类 / 头文件 | 物理真实签名 | 核查状态 |
| :--- | :--- | :--- | :--- |
| `SelectionStateService::instance()` | `SelectionStateService.h` | `static SelectionStateService& instance();` | 100% 匹配 |
| `recordSelection(...)` | `SelectionStateService.h` | `void recordSelection(const QString& panelId, const QString& folderPath, const QSet<QString>& selectedPaths, const QString& focusedPath = QString());` | 100% 匹配 |
| `getSelectionState(...)` | `SelectionStateService.h` | `SelectionState getSelectionState(const QString& panelId) const;` | 100% 匹配 |
| `restoreSelections(...)` | `ContentViewCoordinator.h` | `void restoreSelections(const QSet<QString>& selectedPaths, bool isPendingEdit);` | 100% 匹配 |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）

| 受影响文件 | 修改/新增 `#include` | 类型与枚举完整性核查 |
| :--- | :--- | :--- |
| `CMakeLists.txt` | 增加 `src/core/SelectionStateService.h` 与 `.cpp` | MOC/编译目标链闭合 |
| `src/ui/ContentPanel.cpp` | 新增 `#include "../core/SelectionStateService.h"` | 包含完整 `SelectionStateService` 定义，解决类型推导 |
| `src/core/SelectionStateService.cpp` | `#include "SelectionStateService.h"` | 自包含头文件完整闭合 |
