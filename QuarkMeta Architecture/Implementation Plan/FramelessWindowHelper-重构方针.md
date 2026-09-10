长期来看，"显式枚举参数"这个方向更合理，直接定了，理由很简单：你要的是"以后能加新角色"这种扩展性，靠内部私有方法拆分没法让调用者说清楚自己是谁；只有让调用者显式声明"我是什么角色"，未来加新角色才是"加一个枚举值 + 一张配置表"这种低成本操作，不需要动现有调用点。

**设计方案（先讲清楚，不动代码）：**

**1. 新增角色枚举，写在 `FramelessWindowHelper.h`：**
```cpp
enum class WindowRole {
    Primary,   // 主窗口：完整标题栏、最大化/还原、系统菜单、边缘缩放
    Tool,      // 悬浮工具窗：无标题栏、不可最大化、仅边缘缩放
    Dialog     // 对话框：有自定义标题栏、可拖拽，是否可最大化由具体对话框决定
    // 未来新增角色（比如 Toast、独立预览窗）只需要在这里加一行 + 下面配置表加一行
};
```

**2. `apply()` 签名改为：**
```cpp
static FramelessWindowHelper* apply(QWidget* window, WindowRole role, QWidget* titleBar = nullptr);
```
三个现有调用点都要跟着改：
- `MainWindow`：`FramelessWindowHelper::apply(this, WindowRole::Primary, m_titleBarWidget);`
- `TagSelectorOverlay`：`FramelessWindowHelper::apply(this, WindowRole::Tool);`（不再传 `titleBar`，因为它本来就没有）
- `FramelessDialog`（新增，之前完全没接入）：`FramelessWindowHelper::apply(this, WindowRole::Dialog, m_titleBarWidgetMember);`

**3. 每种角色对应一张清晰的"能力配置表"，取代现在散落各处的 `if (m_titleBar)` 隐式判断：**

| 能力 | Primary | Tool | Dialog |
|---|---|---|---|
| 原生样式（WS_CAPTION/MAXIMIZEBOX/MINIMIZEBOX/SYSMENU） | ✅ | ❌（仅 WS_THICKFRAME） | ✅ |
| 边缘缩放 | ✅ | ✅ | ✅ |
| 标题栏区域原生拖拽（HTCAPTION） | ✅ | ❌ | ✅ |
| 双击标题栏最大化/还原 | ✅ | ❌ | ❌（对话框不需要） |
| 标题栏最大化图标同步 | ✅ | ❌ | ❌ |

**4. 收编 `FramelessDialog` 的具体动作（这是本次改动量最大的部分）：**

现在 `FramelessDialog::mousePressEvent` 里那段 `ReleaseCapture() + SendMessageW(WM_NCLBUTTONDOWN, HTCAPTION, 0)` 的手写实现，本质上就是 `FramelessWindowHelper` 里 `WM_NCHITTEST` 返回 `HTCAPTION` 之后 Windows 自动帮你做的事——完全是重复实现。收编之后：
- 删掉 `FramelessDialog::mousePressEvent/mouseMoveEvent/mouseReleaseEvent` 里手写的拖拽逻辑（非交互控件判断 `isInteractiveWidget` 也一并删除，`FramelessWindowHelper` 里已经有一份一模一样的）。
- `FramelessDialogBase.h` 里那个在构造函数局部变量里创建的 `titleBar`（现在只是个局部变量，没有存成员），需要提升为成员变量，才能传给 `FramelessWindowHelper::apply()`。
- `FramelessDialog` 需要新增 `nativeEvent` 覆写（目前完全没有），接入 `handleNativeEvent`。
- `Dialog` 角色不需要 `WS_MAXIMIZEBOX/MINIMIZEBOX`（对话框通常不允许最大化，`FramelessDialog::setVisibleButtons()` 本来就能控制按钮显隐，但原生系统能力也不该给），双击标题栏最大化这一条对 `Dialog` 角色直接禁用。

**这次不动、留到你确认后再说的：**
- `TitleBarWidget::setWindowMaximized` 那个 `invokeMethod` 警告——顺手能修，但不在这次角色收编的范围内，按你之前"问题范围是什么就只回答那个范围"的要求，先不碰。

这个方案要不要现在开始按这个思路改代码？涉及 `FramelessWindowHelper.h/.cpp`、`FramelessDialogBase.h`、`FramelessDialog.cpp`（还有 `MainWindow.cpp`/`TagSelectorOverlay.cpp` 里的调用点跟着改签名），文件比较多，改之前跟你确认一下范围没问题。