# 实施方案：FilterPanel 创建与修改日期组降序/升序点击即时刷新修复 (FilterPanel-1.md)

## 1. Overview（概述与解决的问题）

### 1.1 核心问题说明
在筛选面板 `FilterPanel` 中，点击“创建日期”与“修改日期”分组标题右侧的升降序切换按钮（`btnSort`）时，列表没有即时按最新顺序重新排序呈现，点击形同摆设。

### 1.2 根因与修复方案
在 `FilterPanel.cpp` 中，`rebuildDateCheckboxes(bool isCreateDate, bool descending)` 方法负责清理现有复选框并重新按升/降序构建日期列表。当点击 `btnSort` 按钮时，已调用 `rebuildDateCheckboxes(true/false, m_createDateDesc/m_modifyDateDesc)`，但必须确保列表重绘后面板 UI 强制进行 `update()` 与布局刷新，且关联的状态与界面交互无阻塞。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/FilterPanel.cpp` - 在 `rebuildDateCheckboxes` 列表构建完成后触发 `update()`，并在 `btnSort` 点击事件中确保界面即时重绘。

---

## 3. Detailed Line-by-Line Changes（精准代码替换块）

### 3.1 修改 `src/ui/FilterPanel.cpp`

```
<<<<<<< SEARCH
    for (const QString& d : dates) {
        QCheckBox* cb = addFilterRow(layout, d, counts[d]);
        cb->blockSignals(true);
        cb->setChecked(selected.contains(d));
        cb->blockSignals(false);
        connect(cb, &QCheckBox::toggled, this, [this, isCreateDate, d](bool on) {
            FilterState st = m_filterModel->state();
            QStringList& targetList = isCreateDate ? st.createDates : st.modifyDates;
            if (on) { if (!targetList.contains(d)) targetList.append(d); }
            else targetList.removeAll(d);
            m_filterModel->setState(st);
        });
    }
}
=======
    for (const QString& d : dates) {
        QCheckBox* cb = addFilterRow(layout, d, counts[d]);
        cb->blockSignals(true);
        cb->setChecked(selected.contains(d));
        cb->blockSignals(false);
        connect(cb, &QCheckBox::toggled, this, [this, isCreateDate, d](bool on) {
            FilterState st = m_filterModel->state();
            QStringList& targetList = isCreateDate ? st.createDates : st.modifyDates;
            if (on) { if (!targetList.contains(d)) targetList.append(d); }
            else targetList.removeAll(d);
            m_filterModel->setState(st);
        });
    }

    if (m_scrollArea && m_scrollArea->widget()) {
        m_scrollArea->widget()->updateGeometry();
    }
    update();
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

### 4.1 编译验证
在沙盒 Bash 环境中执行构建命令：

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --parallel 4
```

### 4.2 视觉与功能验证步骤
1. **日期排序即时刷新校验**：
   - 启动 `./QuarkMeta`，展开【筛选】面板。
   - 点击“创建日期”与“修改日期”分组标题右侧的三角箭头按钮。
   - 确认下方的日期复选框列表立刻按照最新的升序/降序实时重新排布，不再出现“形同摆设”无响应的问题。
