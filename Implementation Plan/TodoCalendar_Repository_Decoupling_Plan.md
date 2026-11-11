# TodoCalendarWindow 直连剥离与 TodoRepository 专职领域仓储重构实施方案

## Chapter 1: Problem Statement & Root Cause Analysis（问题与根因分析）

### 1. 现象与代码坏味道
在 `RapidNotes_Source/RapidNotes/src/ui/TodoCalendarWindow.cpp` 中，UI 控件直接穿透调用底层上帝类 `DatabaseManager::instance()`，共计调用 22 处：
1. **日历绘制直连**：`paintCell` 中每次单元格重绘均调用 `DatabaseManager::instance().getTodosByDate(date)`，高频阻塞 UI 渲染；
2. **信号订阅直连**：直接监听 `&DatabaseManager::instance(), &DatabaseManager::todoChanged`；
3. **右键动作与状态变更直连**：在上下文菜单中直接就地调用 `addTodo`、`updateTodo`、`deleteTodo`；
4. **伪批量循环写盘 (违背 SSOT 批量契约)**：在右键批量删除动作中，手写 `for (auto* item : items) { DatabaseManager::instance().deleteTodo(id); }`，在排程视图中手写 `for (int id : taskIds) { DatabaseManager::instance().deleteTodo(id); }`，单次删除都会触发独立加锁、独立执行、写盘标记以及发射 `emit todoChanged()`，引发严重的磁盘 I/O 浪费与 UI 连环刷新风暴；
5. **违背单向依赖契约**：UI 视图层直接耦合数据库单例，违背了 `Shell -> View -> Controller -> Domain -> Data` 单向架构流动。

### 2. 根因剖析
历史遗留代码中未设立专职的待办仓储层（`TodoRepository`），且 `DatabaseManager` 中缺少原生原子批量删除接口，导致 UI 视图层不得不手写循环串行写盘。

### 3. 治理目标
1. **在底层 `DatabaseManager` 原生提供原子批量删除接口**：`bool deleteTodosBatch(const QList<int>& ids)`，在单次互斥锁与单次 SQL 事务中执行 `DELETE FROM todos WHERE id IN (...)`，执行完毕后仅标记一次写盘并只发射一次 `emit todoChanged()`；
2. **建立专职领域仓储 `RapidNotes::TodoRepository`**：确立 `TodoItem` 实体契约，精准对接底层接口；`addTodo` 严格返回 `int`（新增待办记录 ID），`deleteTodosBatch` 直接委托底层单事务批量接口；
3. **彻底纯化 UI 容器**：将 `TodoCalendarWindow.cpp` 中的全部 22 处 `DatabaseManager::instance()` 替换为 `TodoRepository::instance()`，消除所有伪批量 for 循环，彻底纯化为 Dumb View。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `RapidNotes_Source/RapidNotes/src/core/DatabaseManager.h` — 声明原生原子批量删除接口 `bool deleteTodosBatch(const QList<int>& ids)`。
2. `RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp` — 实现基于单事务与集合式 SQL 的 `deleteTodosBatch`。
3. `RapidNotes_Source/RapidNotes/src/core/TodoRepository.h` [新建] — 待办专职仓储模块头文件与 TodoItem 契约。
4. `RapidNotes_Source/RapidNotes/src/core/TodoRepository.cpp` [新建] — 待办专职仓储模块实现。
5. `RapidNotes_Source/RapidNotes/src/ui/TodoCalendarWindow.h` — 替换头文件包含与类型声明。
6. `RapidNotes_Source/RapidNotes/src/ui/TodoCalendarWindow.cpp` — 彻底替换 22 处直连调用，收敛至 `TodoRepository` 原生批量接口。
7. `RapidNotes_Source/RapidNotes/CMakeLists.txt` — 注册 `TodoRepository.h` 与 `TodoRepository.cpp` 构建单元。

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `RapidNotes_Source/RapidNotes/src/core/DatabaseManager.h` — 声明原生原子批量删除接口

```diff
<<<<<<< SEARCH
    bool updateTodo(const Todo& todo);
    bool deleteTodo(int id);
    QList<Todo> getTodosByDate(const QDate& date);
=======
    bool updateTodo(const Todo& todo);
    bool deleteTodo(int id);
    bool deleteTodosBatch(const QList<int>& ids);
    QList<Todo> getTodosByDate(const QDate& date);
>>>>>>> REPLACE
```

### 3.2 `RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp` — 实现原子批量删除接口

```diff
<<<<<<< SEARCH
bool DatabaseManager::deleteTodo(int id) {
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return false;
    
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM todos WHERE id = ?");
    query.addBindValue(id);
    
    bool ok = query.exec();
    if (ok) {
        markDirty();
        emit todoChanged();
    }
    return ok;
}
=======
bool DatabaseManager::deleteTodo(int id) {
    return deleteTodosBatch(QList<int>() << id);
}

bool DatabaseManager::deleteTodosBatch(const QList<int>& ids) {
    if (ids.isEmpty()) return true;

    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return false;

    QStringList placeholders;
    placeholders.reserve(ids.size());
    for (int i = 0; i < ids.size(); ++i) {
        placeholders.append("?");
    }

    QSqlQuery query(m_db);
    query.prepare(QString("DELETE FROM todos WHERE id IN (%1)").arg(placeholders.join(",")));
    for (int id : ids) {
        query.addBindValue(id);
    }

    bool ok = query.exec();
    if (ok) {
        markDirty();
        emit todoChanged();
    }
    return ok;
}
>>>>>>> REPLACE
```

### 3.3 `RapidNotes_Source/RapidNotes/src/ui/TodoCalendarWindow.h` — 替换数据库依赖为专职仓储头文件

```diff
<<<<<<< SEARCH
#include "FramelessDialog.h"
#include "../core/DatabaseManager.h"
#include <QCalendarWidget>
=======
#include "FramelessDialog.h"
#include "../core/TodoRepository.h"
#include <QCalendarWidget>
>>>>>>> REPLACE
```

### 3.4 `RapidNotes_Source/RapidNotes/src/ui/TodoCalendarWindow.cpp` — 替换信号连接与初始化

```diff
<<<<<<< SEARCH
    connect(m_btnAlarm, &QPushButton::clicked, this, &TodoCalendarWindow::onAddAlarm);
    connect(m_btnAdd, &QPushButton::clicked, this, &TodoCalendarWindow::onAddTodo);
    connect(m_todoList, &QListWidget::itemDoubleClicked, this, &TodoCalendarWindow::onEditTodo);
    connect(m_detailed24hList, &QListWidget::itemDoubleClicked, this, &TodoCalendarWindow::onDetailedItemDoubleClicked);
    connect(&DatabaseManager::instance(), &DatabaseManager::todoChanged, this, &TodoCalendarWindow::refreshTodos);
=======
    connect(m_btnAlarm, &QPushButton::clicked, this, &TodoCalendarWindow::onAddAlarm);
    connect(m_btnAdd, &QPushButton::clicked, this, &TodoCalendarWindow::onAddTodo);
    connect(m_todoList, &QListWidget::itemDoubleClicked, this, &TodoCalendarWindow::onEditTodo);
    connect(m_detailed24hList, &QListWidget::itemDoubleClicked, this, &TodoCalendarWindow::onDetailedItemDoubleClicked);
    connect(&RapidNotes::TodoRepository::instance(), &RapidNotes::TodoRepository::todoChanged, this, &TodoCalendarWindow::refreshTodos);
>>>>>>> REPLACE
```

### 3.5 `RapidNotes_Source/RapidNotes/src/ui/TodoCalendarWindow.cpp` — 替换日历单元格绘制中的直连读取

```diff
<<<<<<< SEARCH
void CustomCalendar::paintCell(QPainter* painter, const QRect& rect, QDate date) const {
    QList<DatabaseManager::Todo> todos = DatabaseManager::instance().getTodosByDate(date);
    bool isSelected = (date == selectedDate());
=======
void CustomCalendar::paintCell(QPainter* painter, const QRect& rect, QDate date) const {
    QList<DatabaseManager::Todo> todos = RapidNotes::TodoRepository::instance().getTodosByDate(date);
    bool isSelected = (date == selectedDate());
>>>>>>> REPLACE
```

### 3.6 `RapidNotes_Source/RapidNotes/src/ui/TodoCalendarWindow.cpp` — 替换左侧列表右键动作直连与单条写盘

```diff
<<<<<<< SEARCH
        auto* doneAction = menu->addAction(IconHelper::getIcon("select", "#2ecc71"), items.size() > 1 ? QString("批量标记完成 (%1)").arg(items.size()) : "标记完成");
        connect(doneAction, &QAction::triggered, [this, items](){
            QList<DatabaseManager::Todo> todos = DatabaseManager::instance().getTodosByDate(m_calendar->selectedDate());
            for (auto* item : items) {
                int id = item->data(Qt::UserRole).toInt();
                for (auto& t : todos) {
                    if (t.id == id) {
                        t.status = 1;
                        t.progress = 100;
                        DatabaseManager::instance().updateTodo(t);
                        break;
                    }
                }
            }
        });

        auto* deleteAction = menu->addAction(IconHelper::getIcon("delete", "#e74c3c"), items.size() > 1 ? QString("批量删除 (%1)").arg(items.size()) : "删除此任务");
        connect(deleteAction, &QAction::triggered, [this, items](){
            for (auto* item : items) {
                int id = item->data(Qt::UserRole).toInt();
                DatabaseManager::instance().deleteTodo(id);
            }
        });
=======
        auto* doneAction = menu->addAction(IconHelper::getIcon("select", "#2ecc71"), items.size() > 1 ? QString("批量标记完成 (%1)").arg(items.size()) : "标记完成");
        connect(doneAction, &QAction::triggered, [this, items](){
            QList<DatabaseManager::Todo> todos = RapidNotes::TodoRepository::instance().getTodosByDate(m_calendar->selectedDate());
            for (auto* item : items) {
                int id = item->data(Qt::UserRole).toInt();
                for (auto& t : todos) {
                    if (t.id == id) {
                        t.status = 1;
                        t.progress = 100;
                        RapidNotes::TodoRepository::instance().updateTodo(t);
                        break;
                    }
                }
            }
        });

        auto* deleteAction = menu->addAction(IconHelper::getIcon("delete", "#e74c3c"), items.size() > 1 ? QString("批量删除 (%1)").arg(items.size()) : "删除此任务");
        connect(deleteAction, &QAction::triggered, [this, items](){
            QList<int> idsToDelete;
            for (auto* item : items) {
                idsToDelete << item->data(Qt::UserRole).toInt();
            }
            RapidNotes::TodoRepository::instance().deleteTodosBatch(idsToDelete);
        });
>>>>>>> REPLACE
```

### 3.7 `RapidNotes_Source/RapidNotes/src/ui/TodoCalendarWindow.cpp` — 替换排程视图右键菜单中的直连与伪批量

```diff
<<<<<<< SEARCH
        // 收集所有选中行中的任务ID
        QList<int> taskIds;
        QList<DatabaseManager::Todo> todos = DatabaseManager::instance().getTodosByDate(m_calendar->selectedDate());
        
        for (auto* item : items) {
            int hour = m_detailed24hList->row(item);
            for (const auto& t : todos) {
                if (t.startTime.isValid() && t.startTime.time().hour() == hour) {
                    taskIds << t.id;
                    break;
                }
            }
        }

        if (items.size() == 1) {
            int hour = m_detailed24hList->row(items.first());
            bool hasTask = !taskIds.isEmpty();
            if (hasTask) {
                int taskId = taskIds.first();
                auto* editAction = menu->addAction(IconHelper::getIcon("edit", "#4facfe"), "编辑任务");
                auto* deleteAction = menu->addAction(IconHelper::getIcon("delete", "#e74c3c"), "删除任务");
                connect(editAction, &QAction::triggered, [this, taskId](){
                    QList<DatabaseManager::Todo> todos = DatabaseManager::instance().getTodosByDate(m_calendar->selectedDate());
                    for(const auto& t : todos) if(t.id == taskId) { 
                        openEditDialog(t);
                        break; 
                    }
                });
                connect(deleteAction, &QAction::triggered, [this, taskId](){ DatabaseManager::instance().deleteTodo(taskId); });
            } else {
                auto* addAction = menu->addAction(IconHelper::getIcon("add", "#4facfe"), QString("在 %1:00 新增任务").arg(hour, 2, 10, QChar('0')));
                connect(addAction, &QAction::triggered, [this, hour](){
                    DatabaseManager::Todo t;
                    t.startTime = QDateTime(m_calendar->selectedDate(), QTime(hour, 0));
                    t.endTime = t.startTime.addSecs(3600);
                    auto* dlg = new TodoEditDialog(t, this);
                    dlg->setAttribute(Qt::WA_DeleteOnClose);
                    connect(dlg, &QDialog::accepted, [this, dlg](){ 
                        DatabaseManager::instance().addTodo(dlg->getTodo());
                        this->refreshTodos();
                    });
                    dlg->show();
                });
            }
        } else {
            // 多选情况
            if (!taskIds.isEmpty()) {
                auto* doneAction = menu->addAction(IconHelper::getIcon("select", "#2ecc71"), QString("批量标记完成 (%1)").arg(taskIds.size()));
                connect(doneAction, &QAction::triggered, [this, taskIds](){
                    QList<DatabaseManager::Todo> todos = DatabaseManager::instance().getTodosByDate(m_calendar->selectedDate());
                    for (int id : taskIds) {
                        for (auto& t : todos) {
                            if (t.id == id) {
                                t.status = 1;
                                t.progress = 100;
                                DatabaseManager::instance().updateTodo(t);
                                break;
                            }
                        }
                    }
                });

                auto* deleteAction = menu->addAction(IconHelper::getIcon("delete", "#e74c3c"), QString("批量删除任务 (%1)").arg(taskIds.size()));
                connect(deleteAction, &QAction::triggered, [this, taskIds](){
                    for (int id : taskIds) {
                        DatabaseManager::instance().deleteTodo(id);
                    }
                });
            } else {
                return; // 选中的全是空行且是多选，不显示菜单
            }
        }

        menu->exec(QCursor::pos());
    });
=======
        // 收集所有选中行中的任务ID
        QList<int> taskIds;
        QList<DatabaseManager::Todo> todos = RapidNotes::TodoRepository::instance().getTodosByDate(m_calendar->selectedDate());
        
        for (auto* item : items) {
            int hour = m_detailed24hList->row(item);
            for (const auto& t : todos) {
                if (t.startTime.isValid() && t.startTime.time().hour() == hour) {
                    taskIds << t.id;
                    break;
                }
            }
        }

        if (items.size() == 1) {
            int hour = m_detailed24hList->row(items.first());
            bool hasTask = !taskIds.isEmpty();
            if (hasTask) {
                int taskId = taskIds.first();
                auto* editAction = menu->addAction(IconHelper::getIcon("edit", "#4facfe"), "编辑任务");
                auto* deleteAction = menu->addAction(IconHelper::getIcon("delete", "#e74c3c"), "删除任务");
                connect(editAction, &QAction::triggered, [this, taskId](){
                    QList<DatabaseManager::Todo> todos = RapidNotes::TodoRepository::instance().getTodosByDate(m_calendar->selectedDate());
                    for(const auto& t : todos) if(t.id == taskId) { 
                        openEditDialog(t);
                        break; 
                    }
                });
                connect(deleteAction, &QAction::triggered, [this, taskId](){ RapidNotes::TodoRepository::instance().deleteTodo(taskId); });
            } else {
                auto* addAction = menu->addAction(IconHelper::getIcon("add", "#4facfe"), QString("在 %1:00 新增任务").arg(hour, 2, 10, QChar('0')));
                connect(addAction, &QAction::triggered, [this, hour](){
                    DatabaseManager::Todo t;
                    t.startTime = QDateTime(m_calendar->selectedDate(), QTime(hour, 0));
                    t.endTime = t.startTime.addSecs(3600);
                    auto* dlg = new TodoEditDialog(t, this);
                    dlg->setAttribute(Qt::WA_DeleteOnClose);
                    connect(dlg, &QDialog::accepted, [this, dlg](){ 
                        RapidNotes::TodoRepository::instance().addTodo(dlg->getTodo());
                        this->refreshTodos();
                    });
                    dlg->show();
                });
            }
        } else {
            // 多选情况
            if (!taskIds.isEmpty()) {
                auto* doneAction = menu->addAction(IconHelper::getIcon("select", "#2ecc71"), QString("批量标记完成 (%1)").arg(taskIds.size()));
                connect(doneAction, &QAction::triggered, [this, taskIds](){
                    QList<DatabaseManager::Todo> todos = RapidNotes::TodoRepository::instance().getTodosByDate(m_calendar->selectedDate());
                    for (int id : taskIds) {
                        for (auto& t : todos) {
                            if (t.id == id) {
                                t.status = 1;
                                t.progress = 100;
                                RapidNotes::TodoRepository::instance().updateTodo(t);
                                break;
                            }
                        }
                    }
                });

                auto* deleteAction = menu->addAction(IconHelper::getIcon("delete", "#e74c3c"), QString("批量删除任务 (%1)").arg(taskIds.size()));
                connect(deleteAction, &QAction::triggered, [this, taskIds](){
                    RapidNotes::TodoRepository::instance().deleteTodosBatch(taskIds);
                });
            } else {
                return; // 选中的全是空行且是多选，不显示菜单
            }
        }

        menu->exec(QCursor::pos());
    });
>>>>>>> REPLACE
```

### 3.8 `RapidNotes_Source/RapidNotes/src/ui/TodoCalendarWindow.cpp` — 替换日历视图右键弹窗中的直连

```diff
<<<<<<< SEARCH
            QDate selectedDate = m_calendar->selectedDate();
            QList<DatabaseManager::Todo> todos = DatabaseManager::instance().getTodosByDate(selectedDate);
=======
            QDate selectedDate = m_calendar->selectedDate();
            QList<DatabaseManager::Todo> todos = RapidNotes::TodoRepository::instance().getTodosByDate(selectedDate);
>>>>>>> REPLACE
```

### 3.9 `RapidNotes_Source/RapidNotes/src/ui/TodoCalendarWindow.cpp` — 替换悬停 ToolTip 与 24h 列表渲染直连

```diff
<<<<<<< SEARCH
            if (date.isValid()) {
                QList<DatabaseManager::Todo> todos = DatabaseManager::instance().getTodosByDate(date);
                if (!todos.isEmpty()) {
=======
            if (date.isValid()) {
                QList<DatabaseManager::Todo> todos = RapidNotes::TodoRepository::instance().getTodosByDate(date);
                if (!todos.isEmpty()) {
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void TodoCalendarWindow::update24hList(const QDate& date) {
    m_detailed24hList->clear();
    QList<DatabaseManager::Todo> todos = DatabaseManager::instance().getTodosByDate(date);
=======
void TodoCalendarWindow::update24hList(const QDate& date) {
    m_detailed24hList->clear();
    QList<DatabaseManager::Todo> todos = RapidNotes::TodoRepository::instance().getTodosByDate(date);
>>>>>>> REPLACE
```

### 3.10 `RapidNotes_Source/RapidNotes/src/ui/TodoCalendarWindow.cpp` — 替换任务全量加载与新增/编辑槽函数直连

```diff
<<<<<<< SEARCH
void TodoCalendarWindow::refreshTodos() {
    m_todoList->clear();
    
    // [USER_REQUEST] 2026-03-xx 获取全局任务数据，并根据页签及搜索词过滤
    QList<DatabaseManager::Todo> allTodos = DatabaseManager::instance().getAllTodos();
=======
void TodoCalendarWindow::refreshTodos() {
    m_todoList->clear();
    
    // [USER_REQUEST] 2026-03-xx 获取全局任务数据，并根据页签及搜索词过滤
    QList<DatabaseManager::Todo> allTodos = RapidNotes::TodoRepository::instance().getAllTodos();
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    connect(dlg, &QDialog::accepted, [this, dlg](){
        DatabaseManager::instance().addTodo(dlg->getTodo());
        this->refreshTodos();
    });
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
}

void TodoCalendarWindow::onAddTodo() {
    DatabaseManager::Todo t;
    t.startTime = QDateTime(m_calendar->selectedDate(), QTime::currentTime());
    t.endTime = t.startTime.addSecs(3600);
    
    auto* dlg = new TodoEditDialog(t, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    connect(dlg, &QDialog::accepted, [this, dlg](){
        DatabaseManager::instance().addTodo(dlg->getTodo());
        this->refreshTodos();
    });
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
}

void TodoCalendarWindow::onEditTodo(QListWidgetItem* item) {
    int id = item->data(Qt::UserRole).toInt();
    QList<DatabaseManager::Todo> todos = DatabaseManager::instance().getTodosByDate(m_calendar->selectedDate());
    for (const auto& t : todos) {
        if (t.id == id) {
            openEditDialog(t);
            break;
        }
    }
}

void TodoCalendarWindow::onDetailedItemDoubleClicked(QListWidgetItem* item) {
    // [USER_REQUEST] 2026-03-xx 按照用户要求，实现排程视图双击逻辑：有任务则编辑，无任务则按该小时新增
    int todoId = item->data(Qt::UserRole).toInt();
    if (todoId > 0) {
        QList<DatabaseManager::Todo> todos = DatabaseManager::instance().getTodosByDate(m_calendar->selectedDate());
        for (const auto& t : todos) {
            if (t.id == todoId) {
                openEditDialog(t);
                return;
            }
        }
    } else {
        // 无任务，在对应小时新增
        int hour = m_detailed24hList->row(item);
        DatabaseManager::Todo t;
        t.startTime = QDateTime(m_calendar->selectedDate(), QTime(hour, 0));
        t.endTime = t.startTime.addSecs(3600);
        
        auto* dlg = new TodoEditDialog(t, this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        connect(dlg, &QDialog::accepted, [this, dlg](){
            DatabaseManager::instance().addTodo(dlg->getTodo());
            this->refreshTodos();
        });
        dlg->show();
        dlg->raise();
        dlg->activateWindow();
    }
}

void TodoCalendarWindow::openEditDialog(const DatabaseManager::Todo& t) {
    QDialog* dlg = nullptr;
    if (t.priority == 2) {
        dlg = new AlarmEditDialog(t, this);
    } else {
        dlg = new TodoEditDialog(t, this);
    }
    
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    connect(dlg, &QDialog::accepted, [this, dlg, t](){
        DatabaseManager::Todo updatedTodo;
        if (t.priority == 2) updatedTodo = qobject_cast<AlarmEditDialog*>(dlg)->getTodo();
        else updatedTodo = qobject_cast<TodoEditDialog*>(dlg)->getTodo();
        
        DatabaseManager::instance().updateTodo(updatedTodo);
        this->refreshTodos();
    });
=======
    connect(dlg, &QDialog::accepted, [this, dlg](){
        RapidNotes::TodoRepository::instance().addTodo(dlg->getTodo());
        this->refreshTodos();
    });
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
}

void TodoCalendarWindow::onAddTodo() {
    DatabaseManager::Todo t;
    t.startTime = QDateTime(m_calendar->selectedDate(), QTime::currentTime());
    t.endTime = t.startTime.addSecs(3600);
    
    auto* dlg = new TodoEditDialog(t, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    connect(dlg, &QDialog::accepted, [this, dlg](){
        RapidNotes::TodoRepository::instance().addTodo(dlg->getTodo());
        this->refreshTodos();
    });
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
}

void TodoCalendarWindow::onEditTodo(QListWidgetItem* item) {
    int id = item->data(Qt::UserRole).toInt();
    QList<DatabaseManager::Todo> todos = RapidNotes::TodoRepository::instance().getTodosByDate(m_calendar->selectedDate());
    for (const auto& t : todos) {
        if (t.id == id) {
            openEditDialog(t);
            break;
        }
    }
}

void TodoCalendarWindow::onDetailedItemDoubleClicked(QListWidgetItem* item) {
    // [USER_REQUEST] 2026-03-xx 按照用户要求，实现排程视图双击逻辑：有任务则编辑，无任务则按该小时新增
    int todoId = item->data(Qt::UserRole).toInt();
    if (todoId > 0) {
        QList<DatabaseManager::Todo> todos = RapidNotes::TodoRepository::instance().getTodosByDate(m_calendar->selectedDate());
        for (const auto& t : todos) {
            if (t.id == todoId) {
                openEditDialog(t);
                return;
            }
        }
    } else {
        // 无任务，在对应小时新增
        int hour = m_detailed24hList->row(item);
        DatabaseManager::Todo t;
        t.startTime = QDateTime(m_calendar->selectedDate(), QTime(hour, 0));
        t.endTime = t.startTime.addSecs(3600);
        
        auto* dlg = new TodoEditDialog(t, this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        connect(dlg, &QDialog::accepted, [this, dlg](){
            RapidNotes::TodoRepository::instance().addTodo(dlg->getTodo());
            this->refreshTodos();
        });
        dlg->show();
        dlg->raise();
        dlg->activateWindow();
    }
}

void TodoCalendarWindow::openEditDialog(const DatabaseManager::Todo& t) {
    QDialog* dlg = nullptr;
    if (t.priority == 2) {
        dlg = new AlarmEditDialog(t, this);
    } else {
        dlg = new TodoEditDialog(t, this);
    }
    
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    connect(dlg, &QDialog::accepted, [this, dlg, t](){
        DatabaseManager::Todo updatedTodo;
        if (t.priority == 2) updatedTodo = qobject_cast<AlarmEditDialog*>(dlg)->getTodo();
        else updatedTodo = qobject_cast<TodoEditDialog*>(dlg)->getTodo();
        
        RapidNotes::TodoRepository::instance().updateTodo(updatedTodo);
        this->refreshTodos();
    });
>>>>>>> REPLACE
```

### 3.11 `RapidNotes_Source/RapidNotes/CMakeLists.txt` — 注册 TodoRepository 构建单元

```diff
<<<<<<< SEARCH
    src/core/TagLexiconService.cpp
    src/core/TagLexiconService.h
    src/core/TransferToMarkedCommand.cpp
=======
    src/core/TagLexiconService.cpp
    src/core/TagLexiconService.h
    src/core/TodoRepository.cpp
    src/core/TodoRepository.h
    src/core/TransferToMarkedCommand.cpp
>>>>>>> REPLACE
```

---

## Chapter 4: TodoRepository Header & Implementation Design（专职仓储模块设计）

### 4.1 头文件 `src/core/TodoRepository.h`
```cpp
#ifndef RAPIDNOTES_TODOREPOSITORY_H
#define RAPIDNOTES_TODOREPOSITORY_H

#include <QObject>
#include <QList>
#include <QDate>
#include <QDateTime>
#include "DatabaseManager.h"

namespace RapidNotes {

// 遵循 SSOT 实体契约：过度期兼容引用并确立独立 TodoItem 契约
using TodoItem = DatabaseManager::Todo;

class TodoRepository : public QObject {
    Q_OBJECT
public:
    static TodoRepository& instance();

    QList<TodoItem> getTodosByDate(const QDate& date);
    QList<TodoItem> getAllTodos();
    int addTodo(const TodoItem& todo);
    bool updateTodo(const TodoItem& todo);
    bool deleteTodo(int id);
    bool deleteTodosBatch(const QList<int>& ids);

signals:
    void todoChanged();

private:
    explicit TodoRepository(QObject* parent = nullptr);
    ~TodoRepository() override = default;
};

} // namespace RapidNotes

#endif // RAPIDNOTES_TODOREPOSITORY_H
```

### 4.2 实现文件 `src/core/TodoRepository.cpp`
```cpp
#include "TodoRepository.h"
#include "DatabaseManager.h"

namespace RapidNotes {

TodoRepository& TodoRepository::instance() {
    static TodoRepository s_instance;
    return s_instance;
}

TodoRepository::TodoRepository(QObject* parent) : QObject(parent) {
    connect(&DatabaseManager::instance(), &DatabaseManager::todoChanged, this, &TodoRepository::todoChanged);
}

QList<TodoItem> TodoRepository::getTodosByDate(const QDate& date) {
    return DatabaseManager::instance().getTodosByDate(date);
}

QList<TodoItem> TodoRepository::getAllTodos() {
    return DatabaseManager::instance().getAllTodos();
}

int TodoRepository::addTodo(const TodoItem& todo) {
    // 严格对齐实际返回值契约：返回底层真实生成的新待办 ID
    return DatabaseManager::instance().addTodo(todo);
}

bool TodoRepository::updateTodo(const TodoItem& todo) {
    return DatabaseManager::instance().updateTodo(todo);
}

bool TodoRepository::deleteTodo(int id) {
    return DatabaseManager::instance().deleteTodo(id);
}

bool TodoRepository::deleteTodosBatch(const QList<int>& ids) {
    // 严格调用底层单事务原子批量删除接口，杜绝任何 for 循环串行调用
    return DatabaseManager::instance().deleteTodosBatch(ids);
}

} // namespace RapidNotes
```

---

## Chapter 5: Build & Compilation Verification（构建与编译验收标准）

1. **构建语法通过**：CMake 成功生成 VS 解决方案，`DatabaseManager` 与 `TodoRepository` 正常编译；
2. **零符号冲突与零签名报错**：MSVC 成功解析 `DatabaseManager::deleteTodosBatch` 与 `TodoRepository::addTodo`，返回值类型匹配一致；
3. **消除直连证据**：在 `TodoCalendarWindow.cpp` 中检索 `DatabaseManager::instance()`，匹配次数严格由 22 处归零至 0 处。

---

## Chapter 6: Runtime Behavior Verification（运行时行为验收）

1. **日历网格热力渲染**：点击不同日期，单元格热力高亮与待办事件正确加载展示；
2. **待办增删改正常**：双击排程列表新增待办、双击编辑、右键单项删除，数据库能正确写入并实时刷新；
3. **原子批量删除性能验收**：在多选待办时点击“批量删除”，由 `deleteTodosBatch` 一次性完成 SQL 事务执行，并且在日志与事件总线中**确认仅发射了一次 `todoChanged` 信号**，界面无卡顿与闪烁。

---

## Chapter 7: SSOT Statement Table（唯一真理源对照表）

| 业务领域与实体行为 | 既有违背 SSOT 方式 | 重构后唯一合法真理源通道 | 批量化支持契约 |
| :--- | :--- | :--- | :--- |
| **待办事项数据读取** | `TodoCalendarWindow` 直连 `DatabaseManager::getTodosByDate` / `getAllTodos` | `RapidNotes::TodoRepository::instance().getTodosByDate` / `getAllTodos` | 集合式读取 |
| **待办事项新增** | 直连 `DatabaseManager::addTodo` | `RapidNotes::TodoRepository::instance().addTodo` | 严格返回新记录主键 ID |
| **待办事项修改** | 直连 `DatabaseManager::updateTodo` | `RapidNotes::TodoRepository::instance().updateTodo` | 原子化属性同步 |
| **待办事项原子批量删除** | 界面或仓储层使用 `for` 循环串行调用 `deleteTodo(id)` | `DatabaseManager::deleteTodosBatch` + `TodoRepository::deleteTodosBatch` | 底层原生单事务 `WHERE id IN (...)`，单次加锁单次广播 |
| **变更通知广播** | 监听底层 `DatabaseManager::todoChanged` | 订阅领域仓储 `TodoRepository::todoChanged` | 单一领域事件总线 |

---

## Chapter 8: Self-Audit & Three-Gate Checklist（三道关合规自检）

- [x] **第一关：代码真实性核查**
  - 所有 SEARCH 块均与实际文件精确匹配（100% 存在且唯一）；
  - `addTodo` 签名严格返回 `int`，与底层头文件 100% 吻合；
  - `deleteTodosBatch` 在底层实现单事务批量删除，彻底消灭伪批量。
- [x] **第二关：架构硬锁审查**
  - 【契约锁】：既有接口签名冻结，对外暴露符合规范；
  - 【依赖锁】：`TodoCalendarWindow (View) -> TodoRepository (Domain) -> Database (Data)` 严格单向依赖；
  - 【门禁锁】：彻底消灭 UI 直连；
  - 【黑盒隔离锁】：内部实现私有，通过 API/DTO 通信；
  - 【零值篡改锁】：样式、颜色 Hex、时间计算完全照抄既有逻辑，零擅自修改。
- [x] **第三关：物理隔离合规核查**
  - 方案仅存放于 `Implementation Plan/` 目录，源代码文件保持零篡改。
