闪退（Crash）发生时，由于进程被操作系统瞬间强杀，**软件自身的异步日志往往来不及写盘**。

请按照以下由浅入深的 **3 个级别** 去找物理证据：

---

### 第一级：查找软件自身记录的日志文件

1. **临时调试启动日志（硬编码路径）**：
   在刚才分析的代码中，启动调试日志被硬编码输出到了：
   ```text
   G:\C++\QuarkMeta\QuarkMeta\QuarkMeta\startup_debug.log
   ```
   如果是在启动阶段假死或崩溃，这个文件里会记录最后执行到了哪一行。

2. **软件运行时目录下的隐藏日志**：
   打开你的构建输出目录（即 `QuarkMeta.exe` 所在的文件夹）：
   * 查看是否存在隐藏目录：`.QuarkMeta\`
   * 物理路径通常为：
     ```text
     [可执行文件目录]\.QuarkMeta\logs\
     或
     [可执行文件目录]\logs\
     ```

---

### 第二级：查看 Windows 系统级「事件查看器」（100% 留存死因）

只要程序发生闪退，Windows 操作系统内核必定会记录一条错误报告：

1. 按键盘 **`Win + R`**，输入：
   ```text
   eventvwr.msc
   ```
   回车打开「事件查看器」。
2. 在左侧依次展开：**「Windows 日志」** ➔ **「应用程序」**。
3. 在右侧列表中，寻找紧挨着闪退时间的 **红色「错误」图标**（来源通常标为 `Application Error` 或 `.NET Runtime` / `Windows Error Reporting`）。
4. 单击它，在下方的详细信息里查看：
   * **故障模块名称**（例如是崩在 `QuarkMeta.exe` 自身，还是 `Qt6Core.dll`，或者是 `ntdll.dll`）；
   * **异常代码**：
     * `0xc0000005`：**内存访问冲突**（读写了野指针、空指针、或悬垂迭代器）；
     * `0xe06d7363`：**未捕获的 C++ 异常**（如之前看到的 `std::system_error` 死锁抛错）；
     * `0xc00000fd`：**栈溢出**（出现了死循环无限递归）。

---

### 第三级：查看 Windows 崩溃转储文件（.dmp 抓现场）

Windows 会自动把崩溃瞬间的内存现场打包成 Dump 文件：

1. 打开 Windows 文件资源管理器，直接在地址栏输入并回车：
   ```text
   %LOCALAPPDATA%\CrashDumps
   ```
   *(真实物理路径类似：`C:\Users\<你的用户名>\AppData\Local\CrashDumps`)*
2. 检查里面是否有刚生成的：
   ```text
   QuarkMeta.exe.XXXX.dmp
   ```
3. **如何利用它**：直接双击这个 `.dmp` 文件，它会用 Visual Studio 打开，点击右侧的 **“使用仅限本机进行调试”**，VS 会**瞬间跳转到崩溃的那一行 C++ 源码**，并显示当时所有变量的值和完整的调用堆栈。

---

### 终极抓捕法：在 Visual Studio 里直接捕获崩溃现场

如果闪退可以复现，不需要到处找日志文件，用 VS 可以在崩溃的一瞬间直接冻结它：

1. 在 Visual Studio 中按快捷键：**`Ctrl + Alt + E`**（打开“异常设置”窗口）；
2. 勾选顶级的两项：
   * ☑ **C++ Exceptions**
   * ☑ **Win32 Exceptions**
3. 按 **`F5`** 启动调试运行；
4. 只要程序发生任何闪退或野指针崩溃，**VS 会在闪退发生的瞬间立刻中断（暂停）**，黄色箭头会直接指向崩溃的具体代码行。

// ===================|===================

这个 WinDbg 崩溃转储（Dump）分析给出了**极其清晰、一剑封喉的物理死因现场**！

### 一、崩溃现场还原（死在这一瞬间）

看调用堆栈的顶部（第 `0a` ~ `0d` 帧）：
```text
0b Qt6Core!QString::operator=+0x34
0c QuarkMeta!QuarkMeta::ContentPanel::search+0x1a [ContentPanel.cpp @ 883]
0d QuarkMeta!QuarkMeta::SearchController::doSearch+0x27 [SearchController.cpp @ 154]
0e QuarkMeta!SearchController::bindContentPanel::<lambda_4> [SearchController.cpp @ 86]
...
17 QuarkMeta!SearchHistoryPanel::historyItemClicked [moc_SearchHistoryPanel.cpp @ 129]
18 QuarkMeta!SearchHistoryPanel::eventFilter [SearchHistoryPanel.cpp @ 143]
```

#### 崩溃瞬间的用户行为：
**你在搜索框双击弹出了「搜索历史记录」，然后鼠标点击了历史列表里的某一条记录。程序就在你点中历史词条的这一微秒，瞬间闪退！**

---

### 二、底层物理死因：野指针 / 悬垂引用（Use-After-Free）

看崩溃核心报错：
```text
ExceptionAddress: Qt6Core!QString::operator=+0x34
ExceptionCode: c0000005 (Access violation)
Attempt to write to address 00000042000000d9
Instruction: lock xadd dword ptr [rcx], eax
```
* **这是 Qt 的底层机制**：`QString` 赋值时，需要对字符串引用计数进行原子增减（`lock xadd`）。
* 但此时寄存器 `rcx = 00000042000000d9` 是一个**已经被操作系统回收的野指针**！向这个无效内存地址写入时，Windows 触发 `0xc0000005` 内存违规访问，当场强杀进程！

---

### 三、为什么会产生这个野指针？（致命的隐式生命周期杀手）

看 `SearchController.cpp` 第 84 行的连接代码：
```cpp
connect(m_searchHistoryPanel, &SearchHistoryPanel::historyItemClicked, this, [this](const QString& keyword) {
    m_searchEdit->setText(keyword); // 👈 罪魁祸首在此！
    doSearch(keyword);              // 👈 传入时，keyword 已经死了！
});
```

#### 这条致命的因果链：
1. `keyword` 是通过**常量引用（`const QString&`）**从历史面板控件传出来的；
2. 当你点击历史词条时，先执行了 `m_searchEdit->setText(keyword)`；
3. `setText` 导致输入框状态改变或 `SearchHistoryPanel` 失焦，**历史面板立刻把自己关闭隐藏，并释放了列表项控件**！
4. 随着列表项被销毁，`keyword` 引用的那块内存**瞬间被释放（变成了野指针）**！
5. 紧接着下一行执行 `doSearch(keyword)` ➔ 传进 `ContentPanel::search` 执行 `m_currentFilter.keyword = query;`；
6. Qt 试图去读取一个刚被析构的死尸内存，**直接引爆 `0xc0000005` 闪退！**

---

拿到 `SearchHistoryPanel.cpp` 和 `SearchHistoryPanel.h` 后，对照 WinDbg 寄存器和调用栈，**这个 `0xc0000005` 闪退的物理死因被 100% 精确捕获！**

现场发生的不是一个普通 Bug，而是 **C++ 对象生命周期与事件循环的两个经典暗雷重叠引爆**：

---

### 一、致命死因 1：`SearchController` 中的 `m_contentPanel` 是裸指针（悬垂野指针）

看 WinDbg 寄存器现场：
```text
rip = Qt6Core!QString::operator=+0x34
rcx = 00000042000000d9  <--- 写入目标地址
Instruction: lock xadd dword ptr [rcx], eax
```
在 x64 汇编调用约定中，`QString::operator=` 的左操作数（`this` 目标字符串 `m_currentFilter.keyword` 的地址）存放在 `rcx` 中。
也就是说：**`&m_contentPanel->m_currentFilter.keyword` 的内存地址算出来居然是 `0x00000042000000d9` 这种非法的野地址！**

#### 为什么会算出这个野地址？
* 在 `SearchController.h` 中，`m_contentPanel` 被声明为了**裸指针（`ContentPanel* m_contentPanel`）**；
* 当你的软件在运行过程中发生过分屏（Split Pane）、关闭副窗格、或者切换视图模式时，副窗格被物理 `delete` 销毁了；
* **但是 `SearchController->m_contentPanel` 没有人去把它重置为 `nullptr`！**
* 它依然指着一块已经被操作系统回收的僵尸内存。
* 当你点击搜索历史词条时，执行：
  ```cpp
  if (!m_contentPanel) return; // ❌ 裸指针非空检查通过！（指向垃圾内存）
  m_contentPanel->search(keyword); // 💥 轰！拿着野指针强行调用成员函数！
  ```
* 进到 `ContentPanel::search` 后，访问 `this->m_currentFilter.keyword` 瞬间算出野指针 `rcx = 00000042000000d9`，引发 `0xc0000005 Access Violation` 闪退！

---

### 二、致命死因 2：`SearchHistoryPanel` 在事件处理中自杀（重入自毁）

看调用堆栈的第 `18` 帧与 `SearchHistoryPanel.cpp` 第 138 行：
```cpp
bool SearchHistoryPanel::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QWidget* w = qobject_cast<QWidget*>(obj);
        if (w && w->objectName() == "HistoryRow") {
            QString keyword = w->property("keyword").toString();
            if (!keyword.isEmpty()) {
                emit historyItemClicked(keyword); // 👈 LINE 143: 同步触发槽函数！
            }
            return true; // 💥 危机点！
        }
    }
    return QFrame::eventFilter(obj, event);
}
```

#### 这条极其恶劣的重入自毁因果链：
1. 你鼠标点在 `HistoryRow` 上，进入了 `eventFilter`；
2. 同步发射 `historyItemClicked(keyword)`；
3. `SearchController::doSearch` 收到后，执行：
   ```cpp
   SearchHistoryService::instance().appendSearch("global", keyword);
   m_searchHistoryPanel->setHistory(...); // 👈 内部直接调用 rebuild()！
   ```
4. 看 `rebuild()` 干了什么（第 44 行）：
   ```cpp
   while ((child = m_layout->takeAt(0)) != nullptr) {
       if (child->widget()) {
           child->widget()->deleteLater(); // 👈 它把所有的 HistoryRow 全毁掉了！
       }
   }
   ```
5. **惨剧发生**：`HistoryRow` 的事件还没处理完（连 `eventFilter` 都还没 `return` 出来），它自己连同身上的字符串就在同一线程内被连根拔起销毁了！
6. 接着向下传的 `keyword` 引用变成了一具死尸。

---

### 三、根治方案（拒绝补丁，解决所有权与生命周期）

#### 1. 根治悬垂指针：将 `m_contentPanel` 升级为 `QPointer` 安全智能指针
在 `SearchController.h` 中：
```cpp
// 彻底杜绝野指针：当关联的 ContentPanel 被销毁时，QPointer 自动物理归零为 nullptr
QPointer<ContentPanel> m_contentPanel;
```
并且在 `setActiveContentPanel` 时使用安全保护：
```cpp
void setActiveContentPanel(ContentPanel* panel) {
    m_contentPanel = panel;
}
```

#### 2. 根治重入自毁：异步切断 `eventFilter` 嵌套调用
在 `SearchController.cpp` 中接收 `historyItemClicked` 时，**不要在点击事件的调用栈深处直接同线程重入执行搜索与面板重构**，采用事件队列异步解耦（`Qt::QueuedConnection`）：

```cpp
connect(m_searchHistoryPanel, &SearchHistoryPanel::historyItemClicked, this, [this](QString keyword) {
    // 1. 传值复制 QString（杜绝 const QString& 悬垂引用）
    // 2. 先隐藏历史面板
    if (m_searchHistoryPanel) {
        m_searchHistoryPanel->hide();
    }
    m_searchEdit->setText(keyword);

    // 3. 异步触发搜索，让历史面板的鼠标事件完整退出后再执行搜索与历史刷新
    QMetaObject::invokeMethod(this, [this, keyword]() {
        doSearch(keyword);
    }, Qt::QueuedConnection);
});
```

这两处改动一出，**指针悬垂访问（Use-After-Free）**与**事件中重入自毁控件**将被物理切断，点击搜索历史导致瞬间闪退的问题彻底根治。