# 右键菜单调整、Base36 ID 彻底根除与 LoadingWindow 引入实施方案 (Menu & Base36 Purge Implementation Plan)

## 1. Overview（概述与解决的问题）

本实施方案旨在解决以下三个核心需求，确保应用完全契合 QuarkMeta 纯磁盘独立化架构与用户体验规范：
1. **右键菜单项位置调换**：调换内容面板（`ContentPanel`）右键菜单中“排序”与“删除”的位置，确保“删除”（移入回收站/永久删除）严格位于右键菜单的最下方，防止用户误触。
2. **彻底根除 Base36 ID 机制**：拔除内存数据库托管模式下残余的 Base36 ID 生成与反查逻辑（`ShellHelper::generateBase36Id`、`MetadataManager::extractBase36Id` 等），实现绝对纯净的磁盘路径管理。
3. **引用并集成 LoadingWindow**：在 `main.cpp` 程序启动流程中包含 `LoadingWindow.h` 并展示启动加载界面，提升软件启动与初始化过程中的用户体验。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/ContentPanel.cpp`
2. `src/util/ShellHelper.h`
3. `src/util/ShellHelper.cpp`
4. `src/meta/MetadataManager.h`
5. `src/meta/MetadataManager.cpp`
6. `src/util/AssetImporter.cpp`
7. `src/main.cpp`
8. `QuarkMeta Architecture/QuarkMeta-Architecture-Planning.md`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/ContentPanel.cpp`
调整右键菜单逻辑，将“排序”菜单项放置在常规组之后，将“删除”菜单项移至整个右键菜单的最底部。

```
<<<<<<< SEARCH
        // 2026-06-xx 按照用户要求：在回收站中不显示二级删除菜单
        if (m_currentCategoryType != "trash") {
            QMenu* delMenu = menu.addMenu("删除");
            UiHelper::applyMenuStyle(delMenu);
            delMenu->addAction("移入回收站")->setData(ActionDelete);
            delMenu->addAction("永久删除")->setData(ActionSecureDelete);
        } else {
            menu.addAction(UiHelper::getIcon("trash", QColor("#e81123"), 18), "永久删除")->setData(ActionSecureDelete);
        }

    } else {
        // [空白处菜单]
        QMenu* newMenu = menu.addMenu("新建...");
        UiHelper::applyMenuStyle(newMenu);
        newMenu->addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "创建文件夹")->setData(ActionNewFolder);
        newMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE")), "创建 Markdown")->setData(ActionNewMd);
        newMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE")), "创建纯文本文件 (txt)")->setData(ActionNewTxt);

        menu.addSeparator();

        QAction* actBatchCreate = menu.addAction(UiHelper::getIcon("add", QColor("#EEEEEE")), "批量创建项目...");
        actBatchCreate->setData(ActionBatchCreate);
        // 6.1 磁盘目录模式独占 改为 全模式解锁（回收站除外）
        if (m_currentCategoryType == "trash") {
            actBatchCreate->setEnabled(false);
            actBatchCreate->setToolTip("回收站中不支持批量创建");
        }

        menu.addSeparator();
        QAction* actPaste = menu.addAction("粘贴");
        actPaste->setData(ActionPaste);
        actPaste->setEnabled(!m_currentPath.isEmpty() && m_currentPath != "computer://");

        menu.addSeparator();
        menu.addAction("刷新")->setData(ActionRefresh);
    }

    menu.addSeparator();

    // 注入“排序”二级子菜单
    QMenu* sortMenu = menu.addMenu("排序");
=======
    } else {
        // [空白处菜单]
        QMenu* newMenu = menu.addMenu("新建...");
        UiHelper::applyMenuStyle(newMenu);
        newMenu->addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "创建文件夹")->setData(ActionNewFolder);
        newMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE")), "创建 Markdown")->setData(ActionNewMd);
        newMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE")), "创建纯文本文件 (txt)")->setData(ActionNewTxt);

        menu.addSeparator();

        QAction* actBatchCreate = menu.addAction(UiHelper::getIcon("add", QColor("#EEEEEE")), "批量创建项目...");
        actBatchCreate->setData(ActionBatchCreate);
        // 6.1 磁盘目录模式独占 改为 全模式解锁（回收站除外）
        if (m_currentCategoryType == "trash") {
            actBatchCreate->setEnabled(false);
            actBatchCreate->setToolTip("回收站中不支持批量创建");
        }

        menu.addSeparator();
        QAction* actPaste = menu.addAction("粘贴");
        actPaste->setData(ActionPaste);
        actPaste->setEnabled(!m_currentPath.isEmpty() && m_currentPath != "computer://");

        menu.addSeparator();
        menu.addAction("刷新")->setData(ActionRefresh);
    }

    menu.addSeparator();

    // 注入“排序”二级子菜单
    QMenu* sortMenu = menu.addMenu("排序");
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    orderGroup->addAction(actAsc);
    orderGroup->addAction(actDesc);

    // 恢复之前的自定义逻辑结束
=======
    orderGroup->addAction(actAsc);
    orderGroup->addAction(actDesc);

    // 🚨 按照用户要求：确保“删除”选项严格位于右键菜单的最下方（仅在选中项目时显示）
    if (currentIndex.isValid()) {
        menu.addSeparator();
        if (m_currentCategoryType != "trash") {
            QMenu* delMenu = menu.addMenu("删除");
            UiHelper::applyMenuStyle(delMenu);
            delMenu->addAction("移入回收站")->setData(ActionDelete);
            delMenu->addAction("永久删除")->setData(ActionSecureDelete);
        } else {
            menu.addAction(UiHelper::getIcon("trash", QColor("#e81123"), 18), "永久删除")->setData(ActionSecureDelete);
        }
    }
>>>>>>> REPLACE
```

---

### 3.2 `src/util/ShellHelper.h` & `src/util/ShellHelper.cpp`
从 `ShellHelper` 中移除 `generateBase36Id()` 函数定义与实现。

```
<<<<<<< SEARCH
    /**
     * @brief 生成 13 位唯一 Base36 ID (基于毫秒时间戳 + 计数器)
     */
    static QString generateBase36Id();
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
QString ShellHelper::generateBase36Id() {
    static std::atomic<uint32_t> counter{0};
    qint64 msecs = QDateTime::currentMSecsSinceEpoch();
    uint32_t count = counter.fetch_add(1) % 46656; // 36^3 = 46656

    auto toBase36 = [](qint64 val, int width) -> QString {
        const char chars[] = "0123456789abcdefghijklmnopqrstuvwxyz";
        QString res;
        while (val > 0) {
            res.prepend(chars[val % 36]);
            val /= 36;
        }
        while (res.length() < width) {
            res.prepend('0');
        }
        return res;
    };

    return toBase36(msecs, 10) + toBase36(count, 3);
}
=======
>>>>>>> REPLACE
```

---

### 3.3 `src/meta/MetadataManager.cpp`
清理静态方法 `extractBase36Id` 及调用的分支。

```
<<<<<<< SEARCH
// 🚨 内存数据库模式唯一ID体系重构：路径级 Base36 ID 静态提取解析器
static std::string extractBase36Id(const std::wstring& path) {
    if (path.empty()) return "";

    // 转换为标准斜杠便于解析
    std::wstring p = path;
    for (auto& c : p) { if (c == L'\\') c = L'/'; }

    // 匹配包含 .arc/ 容器路径或特定识别格式
    // 托管资产容器文件夹名格式恒为 13 位 Base36 字符串 (如 00ms73182x000)
    size_t lastSlash = p.find_last_of(L'/');
    std::wstring fileName = (lastSlash != std::wstring::npos) ? p.substr(lastSlash + 1) : p;

    // 去除 .arc 后缀
    if (fileName.length() >= 4 && fileName.substr(fileName.length() - 4) == L".arc") {
        fileName = fileName.substr(0, fileName.length() - 4);
    }

    // 校验是否为 13 位 Base36 字符串
    if (fileName.length() == 13) {
        bool isBase36 = true;
        for (wchar_t c : fileName) {
            if (!((c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z'))) {
                isBase36 = false;
                break;
            }
        }
        if (isBase36) {
            std::string idStr(fileName.begin(), fileName.end());
            std::transform(idStr.begin(), idStr.end(), idStr.begin(), ::tolower);
            return idStr;
        }
    }
    return "";
}
=======
>>>>>>> REPLACE
```

---

### 3.4 `src/main.cpp`
引入 `LoadingWindow.h` 并将其在软件启动初始化阶段实例化与展示。

```
<<<<<<< SEARCH
#include "ui/MainWindow.h"
=======
#include "ui/MainWindow.h"
#include "ui/LoadingWindow.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    // 重构 5：多段启动。MainWindow 放置于栈上局部作用域，利用 RAII 自动且安全析构，规避 Double Free
    QuarkMeta::MainWindow w;
    w.show();
=======
    // 显示启动加载窗口
    LoadingWindow loadingWin;
    loadingWin.show();
    loadingWin.updateStatus("正在初始化系统核心组件...");
    qApp->processEvents();

    // 重构 5：多段启动。MainWindow 放置于栈上局部作用域，利用 RAII 自动且安全析构，规避 Double Free
    QuarkMeta::MainWindow w;
    loadingWin.onInitializationFinished();
    w.show();
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
   - **右键菜单测试**：在第三栏内容视图中右键任意项目，确认“删除”菜单项严格处于最底部，且“排序”二级菜单处于正常功能区。
   - **Base36 ID 根除验证**：全局搜索代码中不再存在任何 Base36 生成与解析点，全磁盘直连与 JSON 元数据读写流畅。
   - **LoadingWindow 验证**：启动程序，观察在主窗口显示前弹出 LoadingWindow 提示界面并随后平滑进入主界面。
