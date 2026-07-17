# 内容容器右键“排序”主选项及子选项切换方案 —— Modification_Plan-10.md

## 1. 任务背景
在目前的界面上，内容容器（`ContentPanel`）的排序逻辑存在较大的体验盲区。列表视图只能依靠点击列头切换排序，且没有提供原本缺失的“创建日期”、“尺寸”等核心排序纬度。同时，卡片网格视图完全无法在界面上切换数据的任何排序方式（写死在名称升序上）。

为了打通不同展现模式下排序的全局一致性，本方案拟在内容容器（`ContentPanel`）的右键菜单中注入二级菜单“排序”，其子菜单包括 7 种属性单选选项、1 条分割线及 2 种方向单选选项，支持在列表和网格等所有视图下无缝重排，实时见效。

---

## 2. 问题定位与底层断层
- **列头局限**：目前的排序高度绑定在 `FilterProxyModel::lessThan` 与 `QSortFilterProxyModel::lessThan` 的多列默认比较上，而数据库元数据字段中的物理“创建日期”（`ctime`）、图片的“宽度与高度”（`width/height`）及“星级评分（`rating`）”并没有被完整映射为列表列的 DisplayRole 以进行直观排序。
- **网格模式无入口**：卡片网格模式使用的 `QListView` 并没有列头提供点击事件。需要使排序数据不单纯依赖表格的“排序列号 (columnId)”，而是由一个全局的排序状态字段（`SortType`）接管，实现属性级解耦。

---

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 右键菜单增加主选项为“排序”的二级子菜单 | 在 `onCustomContextMenuRequested` 空白与项目右键菜单上均动态创建 `QMenu* sortMenu = menu.addMenu("排序")` | ✅ 一致 |
| 2    | 子选项包含：名称、创建日期、修改日期、扩展名、大小、尺寸、评分 | 注入上述 7 个单选勾选 QAction，利用 `QActionGroup` 维护互斥联动选中状态，并从配置文件同步恢复 | ✅ 一致 |
| 3    | 升序、降序 | 注入上述 2 个单选勾选 QAction，带分割线，配合 ActionGroup 实现互斥选中 | ✅ 一致 |

---

## 4. 详细解决方案

### 4.1 引入右键排序状态属性
在 `src/ui/ContentPanel.h` 中，声明排序维度枚举：

```cpp
namespace ArcMeta {

class ContentPanel : public QFrame {
    // ...
public:
    enum SortType {
        SortByName,
        SortByCreateDate,
        SortByModifyDate,
        SortByExtension,
        SortBySize,
        SortByDimension,
        SortByRating
    };
    
    // 供外部代理模型（FilterProxyModel）访问的接口
    SortType currentSortType() const { return m_sortType; }
    Qt::SortOrder currentSortOrder() const { return m_sortOrder; }

private:
    SortType m_sortType = SortByName;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
};

}
```

在 `ContentPanel` 构造函数中，从 `AppConfig` 读取恢复用户上一次所选定的排序偏好，并在切换时执行写盘同步：
```cpp
    m_sortType = static_cast<SortType>(AppConfig::instance().getValue("ContentPanel/RightClickSortType", SortByName).toInt());
    m_sortOrder = static_cast<Qt::SortOrder>(AppConfig::instance().getValue("ContentPanel/RightClickSortOrder", Qt::AscendingOrder).toInt());
```

---

### 4.2 重写 `FilterProxyModel::lessThan` 算法
对 `FilterProxyModel::lessThan` 进行底层算法扩充，不再单纯依赖 `QSortFilterProxyModel::lessThan`。在完成了第一权重（文件夹置顶）与第二权重（置顶优先）的筛选后，直接根据 `m_sortType` 属性，定位并提取两侧 `ItemRecord` 的对应字段进行直接、高效的类型敏感对比。这不仅保证了大小、尺寸、日期的绝对高精度数值对比（杜绝格式化为字符串后造成的字母序错乱），也填补了此前缺失的“创建日期”和“尺寸”的排序空白：

**核心算法实现逻辑：**
```cpp
bool FilterProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const {
    // 1. 第一级：文件夹与子分类始终置顶
    QString leftType = source_left.data(TypeRole).toString();
    QString rightType = source_right.data(TypeRole).toString();
    bool leftIsDir = (leftType == "folder" || leftType == "category");
    bool rightIsDir = (rightType == "folder" || rightType == "category");
    if (leftIsDir != rightIsDir) {
        if (sortOrder() == Qt::AscendingOrder) return leftIsDir;
        else return !leftIsDir;
    }

    // 2. 第二级：置顶优先（Pinned Priority）
    QVariant leftPinnedVar = source_left.data(PinnedRole);
    if (!leftPinnedVar.isValid()) leftPinnedVar = source_left.data(IsLockedRole);
    QVariant rightPinnedVar = source_right.data(PinnedRole);
    if (!rightPinnedVar.isValid()) rightPinnedVar = source_right.data(IsLockedRole);
    bool leftPinned = leftPinnedVar.toBool();
    bool rightPinned = rightPinnedVar.toBool();
    if (leftPinned != rightPinned) {
        if (sortOrder() == Qt::AscendingOrder) return leftPinned;
        else return !leftPinned;
    }

    // 3. 第三级：由右键选择的 m_sortType 驱动的七维精确物理属性对位排序（对应用户原话：“名称、创建日期、修改日期、扩展名、大小、尺寸、评分”）
    const auto* sourceModelPtr = qobject_cast<const FerrexVirtualDbModel*>(sourceModel());
    if (!sourceModelPtr) return QSortFilterProxyModel::lessThan(source_left, source_right);

    const auto& records = sourceModelPtr->allRecords();
    int leftRow = source_left.row();
    int rightRow = source_right.row();
    if (leftRow < 0 || leftRow >= (int)records.size() || rightRow < 0 || rightRow >= (int)records.size()) {
        return QSortFilterProxyModel::lessThan(source_left, source_right);
    }

    const auto& leftRec = records[leftRow];
    const auto& rightRec = records[rightRow];

    auto* contentPanel = qobject_cast<ContentPanel*>(parent());
    ContentPanel::SortType sType = contentPanel ? contentPanel->currentSortType() : ContentPanel::SortByName;

    switch (sType) {
        case ContentPanel::SortByName: {
            QString lName = leftRec.isCategory ? leftRec.categoryName : QFileInfo(leftRec.path).fileName();
            QString rName = rightRec.isCategory ? rightRec.categoryName : QFileInfo(rightRec.path).fileName();
            return lName.localeAwareCompare(rName) < 0;
        }
        case ContentPanel::SortByCreateDate: {
            // 对比 ctime (创建时间戳)
            return leftRec.ctime < rightRec.ctime;
        }
        case ContentPanel::SortByModifyDate: {
            // 对比 mtime (修改时间戳)
            return leftRec.mtime < rightRec.mtime;
        }
        case ContentPanel::SortByExtension: {
            // 对比文件后缀名
            return leftRec.suffix.localeAwareCompare(rightRec.suffix) < 0;
        }
        case ContentPanel::SortBySize: {
            // 对比文件大小 (文件夹或子分类默认视为 0)
            long long lSize = (leftRec.isCategory || leftRec.isDir) ? -1 : leftRec.size;
            long long rSize = (rightRec.isCategory || rightRec.isDir) ? -1 : rightRec.size;
            return lSize < rSize;
        }
        case ContentPanel::SortByDimension: {
            // 对比图片的总尺寸 (宽 x 高，无尺寸信息视为 0)
            long long lDim = (long long)leftRec.width * leftRec.height;
            long long rDim = (long long)rightRec.width * rightRec.height;
            return lDim < rDim;
        }
        case ContentPanel::SortByRating: {
            // 对比文件评分
            return leftRec.rating < rightRec.rating;
        }
    }

    return QSortFilterProxyModel::lessThan(source_left, source_right);
}
```

---

### 4.3 构建并在右键菜单中注入“排序”二级子菜单
在 `ContentPanel::onCustomContextMenuRequested`（`src/ui/ContentPanel.cpp`）里，无论是选中项还是空白处右键菜单，都注入排序操作，并配合 `QActionGroup` 在子菜单上添加带对勾（check）的单选联动：

```cpp
    QMenu* sortMenu = menu.addMenu("排序");
    UiHelper::applyMenuStyle(sortMenu);

    // 属性单选组
    QActionGroup* typeGroup = new QActionGroup(this);
    auto addTypeAct = [&](const QString& label, ContentPanel::SortType type) {
        QAction* act = sortMenu->addAction(label);
        act->setCheckable(true);
        act->setChecked(m_sortType == type);
        typeGroup->addAction(act);
        connect(act, &QAction::triggered, [this, type]() {
            m_sortType = type;
            AppConfig::instance().setValue("ContentPanel/RightClickSortType", static_cast<int>(type));
            
            // 实时触发全量无效化与排序重计算
            m_proxyModel->invalidate();
            m_proxyModel->sort(0, m_sortOrder);
        });
    };

    addTypeAct("名称", ContentPanel::SortByName);
    addTypeAct("创建日期", ContentPanel::SortByCreateDate);
    addTypeAct("修改日期", ContentPanel::SortByModifyDate);
    addTypeAct("扩展名", ContentPanel::SortByExtension);
    addTypeAct("大小", ContentPanel::SortBySize);
    addTypeAct("尺寸", ContentPanel::SortByDimension);
    addTypeAct("评分", ContentPanel::SortByRating);

    sortMenu->addSeparator();

    // 方向单选组
    QActionGroup* orderGroup = new QActionGroup(this);
    auto addOrderAct = [&](const QString& label, Qt::SortOrder order) {
        QAction* act = sortMenu->addAction(label);
        act->setCheckable(true);
        act->setChecked(m_sortOrder == order);
        orderGroup->addAction(act);
        connect(act, &QAction::triggered, [this, order]() {
            m_sortOrder = order;
            AppConfig::instance().setValue("ContentPanel/RightClickSortOrder", static_cast<int>(order));
            
            m_proxyModel->invalidate();
            m_proxyModel->sort(0, order);
        });
    };

    addOrderAct("升序", Qt::AscendingOrder);
    addOrderAct("降序", Qt::DescendingOrder);
```

---

## 5. 修改边界声明【红线】

**本次方案涉及范围：**
- [ ] 模块/文件：`src/ui/ContentPanel.h` 与 `src/ui/ContentPanel.cpp`
  - 声明 `SortType` 枚举与 `m_sortType`/`m_sortOrder` 变量
  - 扩充并重写 `FilterProxyModel::lessThan` 算法
  - 在右键菜单中注入主选项“排序”二级子菜单、QAction 属性勾选联动，以及切换时的 `invalidate()` 和 `sort()` 排序触发逻辑。

**明确禁止越界修改的范围：**
- [ ] 严禁修改除了排序和右键展现外的其他不相关的 UI 样式及不相关的数据库操作逻辑。

---

## 6. 实现准则与预警【核心】
1. **尺寸和日期的高精度数值比较**：
   - 避免将日期、大小、尺寸格式化为字符串后再比较。本设计直接对 `ctime` / `mtime` (时间戳整数)、`size` (文件大小字节)、`width * height` (像素总积) 进行数值比较，彻底规避字典序排序漏洞，确保排序逻辑 100% 正确且开箱即用。
2. **列表与网格的一致性重排**：
   - 在网格模式下切换排序也应实时生效。本设计中，在菜单选择触发后，显式调用了 `m_proxyModel->invalidate()` 彻底刷新底层排序缓存，并调用 `m_proxyModel->sort(0, m_sortOrder)`，这保证了网格视图模式（`m_gridView`）也能在第一帧同步产生极速、无闪烁重排重组效果。

---

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 双轨机制 | 分类模式与物理模式功能范围各就其位。排序执行范围与 UI 顶部 Focus Line 实时对齐。 | ✅ 符合。本次修改完全是在容器层和过滤层中对已载入的数据行进行视觉排序，与数据源获取机制、作用域和双轨机制无任何冲突。 |
