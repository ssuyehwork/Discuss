这是一个非常明确的需求，本质上是一个 Python脚本启动器（Python Script Launcher）+ 脚本管理中心。

我建议采用以下技术栈：

Python 3.11+
PySide6（现代UI，支持深色主题）
SQLite（轻量DB数据库）
QThread（异步执行脚本）
logging（日志管理）
subprocess（运行py脚本）
功能规划（根据需求推导）
已明确需求

✅ 深色主题

✅ 运行 .py 脚本

✅ 数据库存储脚本路径

✅ 管理脚本路径

✅ 置顶脚本

✅ 搜索脚本

✅ 复制脚本路径

✅ 右侧日志显示

✅ 支持拖拽导入

可扩展需求（推导）
脚本信息

显示：

脚本名称
完整路径
添加时间
最后运行时间
运行次数

例如：

Spider.py
D:\Python\Spider.py

运行次数：12
最后运行：2026-01-01 12:00

历史记录

独立TAB

Spider.py
DataSync.py
Test.py


可搜索：

关键字：
[Spider____]

收藏（置顶）

点击星标：

★ Spider.py
★ Sync.py

Data.py
Test.py


置顶永远排最前。

最近运行

自动记录：

最近运行

Spider.py
2026-01-01 13:21

Data.py
2026-01-01 12:20

一键打开目录

右键菜单：

运行
编辑
复制路径
打开目录
删除
置顶

双击运行
双击脚本
↓
直接运行

Python环境配置

一个脚本对应一个解释器：

Spider.py

Python:
D:\Python311\python.exe


数据库保存：

python_path


运行：

subprocess.Popen([
    python_path,
    script_path
])

UI布局设计
┌────────────────────────────────────────────┐
│ 搜索框                                     │
├──────────────┬─────────────────────────────┤
│              │                             │
│ 脚本列表      │       日志输出窗口           │
│              │                             │
│ ★ spider.py  │ [INFO] 启动脚本              │
│ sync.py      │ [INFO] 输出内容              │
│ test.py      │ [ERROR] 错误内容             │
│              │                             │
│              │                             │
├──────────────┴─────────────────────────────┤
│ 新增 删除 编辑 运行 复制路径                │
└────────────────────────────────────────────┘


比例：

QSplitter

左侧：30%
右侧：70%

SQLite设计
scripts表
CREATE TABLE scripts(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT,
    path TEXT UNIQUE,
    python_path TEXT,
    is_top INTEGER DEFAULT 0,
    run_count INTEGER DEFAULT 0,
    create_time TEXT,
    last_run_time TEXT
);

logs表
CREATE TABLE logs(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    script_id INTEGER,
    level TEXT,
    content TEXT,
    create_time TEXT
);

运行脚本实现
import subprocess

process = subprocess.Popen(
    ["python", script_path],
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
    text=True,
    encoding="utf-8"
)

for line in process.stdout:
    print(line)

日志实时显示

使用线程：

class ScriptRunner(QThread):

    signal_log = Signal(str)

    def run(self):

        process = subprocess.Popen(
            self.cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True
        )

        for line in process.stdout:
            self.signal_log.emit(line)


主界面：

runner.signal_log.connect(
    self.append_log
)

拖拽支持
self.setAcceptDrops(True)

def dropEvent(self, event):

    urls = event.mimeData().urls()

    for url in urls:

        file = url.toLocalFile()

        if file.endswith(".py"):
            self.add_script(file)


支持：

Explorer
↓
拖入窗口
↓
自动录入数据库

搜索实现
SELECT *
FROM scripts
WHERE name LIKE '%keyword%'


实时搜索：

searchEdit.textChanged.connect(
    self.search_script
)

深色主题
app.setStyleSheet("""
QWidget{
    background:#1e1e1e;
    color:#ffffff;
}

QListWidget{
    border:1px solid #444;
}

QPushButton{
    background:#2d2d2d;
    border:1px solid #555;
    padding:6px;
}

QPushButton:hover{
    background:#3a3a3a;
}

QLineEdit{
    background:#2b2b2b;
    border:1px solid #555;
}
""")

项目结构
PyScriptLauncher/

│
├── main.py
│
├── db/
│   └── database.py
│
├── ui/
│   ├── main_window.py
│   └── dark_theme.py
│
├── core/
│   ├── runner.py
│   ├── drag_drop.py
│   └── log_manager.py
│
├── models/
│   └── script_model.py
│
├── assets/
│   └── icons/
│
└── data/
    └── launcher.db

进阶增强功能（推荐）
多脚本同时运行
Spider.py     [运行中]
Data.py       [运行中]
Sync.py       [已停止]


支持多个线程。

定时运行
每天 08:00

运行：

DataSync.py


使用：

APScheduler

脚本分组
开发

  Spider.py
  Sync.py

运维

  Backup.py

标签系统
[爬虫]
[运维]
[数据同步]

系统托盘

关闭窗口：

最小化到托盘


后台继续运行脚本。

导出配置
scripts.json


迁移电脑直接导入。

脚本参数
Spider.py

参数：

--date 20260101
--debug


运行：

python Spider.py --date 20260101 --debug


最终这个应用可以定位为：

PyLauncher Pro

Python脚本管理器
Python脚本启动器
Python脚本运行中心
Python脚本日志监控平台

对于个人开发者和运维场景，这套设计已经接近可直接投入使用的桌面产品级方案。

// ===================|===================

可以，而且我建议直接进入 V1.0开发阶段，不要再停留在设计层面。

这个项目复杂度实际上不高，大约：

UI：40%
SQLite：20%
脚本运行器：20%
日志系统：10%
其它功能：10%

完全可以一次性构建出可用版本。

V1.0目录结构
PyLauncher/

├─ main.py
│
├─ database/
│  └─ db.py
│
├─ ui/
│  ├─ main_window.py
│  ├─ script_list.py
│  ├─ log_panel.py
│  └─ dark_theme.py
│
├─ core/
│  ├─ runner.py
│  ├─ script_manager.py
│  └─ dragdrop.py
│
├─ models/
│  └─ script.py
│
├─ resources/
│  └─ icons
│
└─ data/
   └─ launcher.db

第一阶段数据库
db.py
import sqlite3

DB_FILE = "data/launcher.db"


class Database:

    def __init__(self):
        self.conn = sqlite3.connect(DB_FILE)
        self.create_tables()

    def create_tables(self):

        self.conn.execute("""
        CREATE TABLE IF NOT EXISTS scripts(
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT,
            path TEXT UNIQUE,
            python_path TEXT,
            is_top INTEGER DEFAULT 0,
            run_count INTEGER DEFAULT 0,
            create_time TEXT,
            last_run_time TEXT
        )
        """)

        self.conn.commit()

Script实体
from dataclasses import dataclass

@dataclass
class Script:

    id: int = None

    name: str = ""

    path: str = ""

    python_path: str = "python"

    is_top: int = 0

    run_count: int = 0

    create_time: str = ""

    last_run_time: str = ""

第二阶段脚本执行器
runner.py
from PySide6.QtCore import QThread, Signal

import subprocess


class ScriptRunner(QThread):

    log_signal = Signal(str)

    finished_signal = Signal()

    def __init__(self, script_path):

        super().__init__()

        self.script_path = script_path

    def run(self):

        process = subprocess.Popen(
            ["python", self.script_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8"
        )

        for line in process.stdout:

            self.log_signal.emit(line.rstrip())

        self.finished_signal.emit()

第三阶段主界面

布局：

┌─────────────────────────────┐
│ 搜索框                       │
├──────────┬──────────────────┤
│ 脚本列表  │ 日志窗口          │
│          │                  │
│ spider   │ 日志内容         │
│ sync     │                  │
│ test     │                  │
│          │                  │
├──────────┴──────────────────┤
│新增 删除 运行 编辑           │
└─────────────────────────────┘

MainWindow
class MainWindow(QMainWindow):

    def __init__(self):

        super().__init__()

        self.resize(1400, 850)

        self.setWindowTitle(
            "PyLauncher"
        )

搜索框
self.search_edit = QLineEdit()

self.search_edit.setPlaceholderText(
    "搜索脚本..."
)

脚本列表

推荐：

QTreeWidget


原因：

后面支持：

分组
收藏夹
标签

不用再改架构。

列：

名称
运行次数
最后运行
状态

第四阶段拖拽导入
self.setAcceptDrops(True)

def dropEvent(self, event):

    urls = event.mimeData().urls()

    for url in urls:

        file = url.toLocalFile()

        if file.endswith(".py"):

            self.add_script(file)


支持：

资源管理器
↓
拖入
↓
自动保存

第五阶段右键菜单

脚本列表：

QMenu


菜单：

运行
编辑
复制路径
打开目录
置顶
删除


复制路径

from PySide6.QtGui import QGuiApplication

QGuiApplication.clipboard().setText(
    script_path
)


打开目录

os.startfile(
    os.path.dirname(script_path)
)

第六阶段日志系统

右侧：

QPlainTextEdit


优点：

性能高
能显示几万行
self.log_edit.appendPlainText(
    message
)


日志颜色

后期升级：

INFO 绿色

WARNING 黄色

ERROR 红色

第七阶段置顶功能

数据库：

is_top


排序：

SELECT *
FROM scripts
ORDER BY
is_top DESC,
name ASC


效果：

★ Spider.py

★ Sync.py

Backup.py

Test.py

第八阶段搜索功能
SELECT *
FROM scripts
WHERE
name LIKE ?
OR
path LIKE ?


实时搜索：

self.searchEdit.textChanged.connect(
    self.search
)

第九阶段运行历史

新增表：

CREATE TABLE history(

 id INTEGER PRIMARY KEY AUTOINCREMENT,

 script_path TEXT,

 start_time TEXT,

 end_time TEXT,

 result TEXT
)


记录：

脚本

时间

执行结果

耗时


后续可按时间筛选。

第十阶段高级功能（V2.0）
参数模板
Spider.py

参数模板：

生产环境
开发环境
测试环境

多Python版本
Python3.9

Python3.11

Python3.13


选择后运行：

python.exe script.py

托盘运行
QSystemTrayIcon


关闭主窗口：

最小化到托盘

定时任务
APScheduler


界面：

每天08:00

运行：

Spider.py

批量运行
Spider.py
Sync.py
Backup.py

[批量运行]

环境变量管理

脚本单独配置：

ENV

API_KEY=xxx

DEBUG=1


运行时自动注入。

推荐最终技术栈
Python 3.12

PySide6

SQLite

QThread

APScheduler

loguru

qdarktheme


最终产物定位：

PyLauncher

一个专业的Python脚本启动器

支持：

✓ 拖拽导入
✓ 数据库存储
✓ 收藏置顶
✓ 实时日志
✓ 搜索
✓ 历史记录
✓ 多Python解释器
✓ 定时任务
✓ 批量执行
✓ 系统托盘


按照这个方案开发，大约 2500~3500 行代码即可完成一个可投入实际使用的桌面应用。