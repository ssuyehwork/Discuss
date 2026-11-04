# RapidNotes 数据库与底层逻辑重构提示词

使用方式：每次交给 Jules 的内容 = 「第一部分 通用规则」+「第二部分 目标终态」+ 当前要执行的一个步骤（第三部分）。一次只执行一个步骤，完成并汇报后再进行下一步。

---

## 项目意图（先读这一段，再读后面的规则）

RapidNotes 现在操作时卡顿，根因有三类：
1. 写盘过多：一个操作多次提交，逐条逐文件写盘，主线程做数据库写入和重计算。
2. 同一事实存了多份：标签存在三处、类型词表不统一、同一条规则在多个文件各写一份。
3. 职责过载：一个类同时管建表、增删改查、备份、授权、界面状态。

本次重构要一次性从根上解决这三类问题，而不是逐个症状修补。因此：
- 规则里没有写到的情况，一律按这三条原则判断：**每件事实只有一个来源；一个模块只做一件事；不增加写盘次数和计算量**。
- 遇到会让你想加特判、兜底、兼容壳的情况，说明方案方向有问题，停下来在汇报里列出，不要自行绕开。
- 文件和文件夹只记录路径，不复制本体（对齐 Ditto 的做法），以避免占用磁盘空间。
- 你只看到一个步骤，但步骤之间互相依赖。不要只看局部：每个步骤必须先做全局普查，见通用规则第 7 条。

---

## 第一部分：通用规则（每个步骤都必须遵守）

### 1. 任务目标
重构 RapidNotes 的数据库层与底层逻辑，使所有功能归一化、模块化。
范围：`src/core` 下全部代码、`src/models` 下的数据访问逻辑、`src/main.cpp` 中的剪贴板入库逻辑。

### 2. 不通过标准
出现下列任意一条，本步骤即判定失败，必须重做：
- **补丁式修改**：为绕开问题而加的特判、兜底分支、延迟重置、双重写入、"再迁移一次"式修正。必须从根上解决。
- **职责过载**：一个类或一个函数承担两种以上职责（例如既管存储又管业务规则，既管数据库又管界面状态）。
- **违背 SSOT**：同一事实存储在两处，或同一规则定义在两处。
- **爆刷盘**：一个业务操作产生多于一次事务提交；逐条、逐文件写盘。
- **浪费算力**：循环内重复 prepare、N+1 查询、全表扫描后在 C++ 里过滤、主线程里做磁盘访问或重计算。
- **索引缺失**：存在未登记在 `docs/SSOT接口索引.md` 的公开接口，或索引与代码不一致。

### 3. 界面保护
- 不得修改任何界面参数值和样式参数值：尺寸、间距、字号、颜色值、QSS、图标名、图标颜色、文案、布局、动画时长、快捷键、控件层级。
- 不得修改任何交互方式（例如：行内编辑不得改成弹窗输入）。
- 修改界面文件只允许一种情况：把对旧数据库接口的调用换成新接口。
- 规则里没有写明的行为，保持现有代码的行为，不得凭推测改动，不得新增功能，不得改变功能的设计理念。
- 如果某个步骤迫使你必须改动界面文件中的字符串或数值，不要改，在汇报里列出文件名和位置。

### 4. 颜色与授权保持现状
- 颜色：`notes.color` 的取值（`#2ECC71`、`#0A362F`、`#2d2d2d`、`#2c3e50` 等）、触发时机、分类颜色联动，全部原样保留，不得统一、修正、新增。随所在操作原样迁入新模块。
- 授权：`getTrialStatus`、`verifyActivationCode`、`resetActivation`、`saveTrialToFile`、`loadTrialFromFile`、硬件指纹校验相关代码，只原样搬迁到独立文件，不改任何逻辑和取值，也不要在汇报或注释中复述其中的硬编码内容。

### 5. 工程规则
- 每个步骤结束时整个项目必须能编译。新增或删除源文件必须同步更新 `CMakeLists.txt` 的 `SOURCES`，不得改动 CMake 的其他设置。
- 删除旧接口时，必须在同一步骤内改完所有调用处。不得留转发壳、兼容层、`// OLD` 注释、被注释掉的旧代码。
- 线程约束：每个 `QSqlDatabase` 连接只在创建它的线程使用。后台线程只用于：备份、缩略图回填、入库前的重计算（分类判定、缩略图生成），且各自使用独立连接或完全不碰数据库。
- 核心层（`src/core`）不得包含任何 `src/ui` 下的头文件，不得弹窗，不得读写界面状态。
- 不得删除用户磁盘上的任何文件（包括 `attachments` 目录、旧数据库文件、各类备份）。你只能新增和修改代码。

### 6. SSOT 接口索引文档
- 在仓库中维护唯一一份 `docs/SSOT接口索引.md`，登记全部对外公开的接口（类、公开函数、信号）。
- 每条记录统一包含：接口名与所属模块；它是哪一件事实的唯一来源；用途与适用场景（一句话）；禁止在别处重复实现的内容；主要调用方。
- 按"事实"分类（以第二部分 2.1 的 SSOT 表为目录），不按文件分类。
- 每个接口在头文件中写一段与索引一致的注释，索引由这些注释汇总而来，两处内容不得出现差异。
- 新增、改名、删除接口时，必须在同一步骤内同步更新索引和头文件注释。
- 不通过标准追加：存在未登记的公开接口，或索引内容与代码不一致，本步骤判定失败。

### 7. 全局普查与反向核对
- **动手之前**先对本步骤涉及的内容做全工程普查，并把结果写在汇报开头：涉及的所有读写点、所有调用方（文件数与处数，附搜索命令）、所有重复实现。普查结果出来之前不得修改代码。
- 普查必须覆盖 `src/core`、`src/models`、`src/ui`、`src/main.cpp` 全部目录，不得只看被点名的文件。
- 普查中发现的、本步骤规则没有覆盖的相关位置，列在汇报里，不得自行决定改或不改。
- **完成之后**做反向核对：用同一批搜索命令确认旧用法已归零；说明本步骤改动是否触及了其他模块的职责。

### 8. 每步汇报格式（只写结果）
1. 新增、删除、修改的文件清单。
2. 被删除的旧接口清单（旧名称 → 新归属）。
3. 对照本步骤"验收"逐条给出通过或不通过，并给出检查用的搜索命令及结果。
4. 全局普查结果（动手之前的搜索命令、文件数、处数）与反向核对结果。
5. 本步骤对 `docs/SSOT接口索引.md` 的新增、修改、删除条目清单。
6. 无法确认的事项：只列出，不得猜测，不得自行决定。

---

## 第二部分：目标终态（所有步骤共同遵守）

### 2.1 事实的唯一来源（SSOT）

| 事实 | 唯一来源 | 禁止 |
|---|---|---|
| 条目类型词表 | `ItemType`（唯一允许出现 `item_type` 字符串字面量的地方） | 其他文件写类型字符串 |
| 文本的类型判定规则（网址、色码、代码特征、路径） | `ItemClassifier` | 在入库、模型、工具类里各写一份 |
| 条目的标签 | `tags` + `note_tags` 两张表 | `notes.tags` 字符串列 |
| 文件/文件夹条目的路径 | `note_paths` 表 | 用 `;` 拼接后存进 `content` |
| 文件后缀、是否文件夹 | `note_paths.ext`、`note_paths.is_dir` | `notes.file_extensions` 列 |
| 标签的拆分与拼接 | `TagCodec`（拆分接受半角和全角逗号；拼接只用半角逗号 `,`，无空格） | 各处各写 `split`/`join` |
| 字数阶梯 | `WordBucket`（现有 `calculateWordBucket` 逻辑原样搬入） | 其他位置计算 |
| 查询过滤条件 | `NoteQueryService` 内唯一的过滤构造器 | 搜索、计数、统计各写一份 |
| 排序规则 | `SortPolicy`（现有排序规则原样集中），查询与手动排序共用 | 两处各拼 `ORDER BY` |
| 被锁定的分类集合 | 分类密码表，由 `CategoryLockService` 缓存并在变更时失效 | 每次查询重新读表 |
| 笔记内容 → 剪贴板内容 | `NoteClipboardPayload`（一个函数） | 在多个窗口里各写一份 |
| 窗口切换 + 模拟 Ctrl+V | `PasteInjector` | 在多个窗口里各写一份 |

### 2.2 模块与职责（一个模块只做一件事）

| 模块 | 唯一职责 |
|---|---|
| `ConnectionManager` | 打开连接、设置 PRAGMA、提供事务作用域 `TransactionScope`（支持嵌套，只在最外层提交） |
| `SchemaMigrator` | 建表、建索引、按 `PRAGMA user_version` 做版本迁移 |
| `LegacyDatabaseRescue` | 旧版加密外壳、AppData 残留、旧 notes.db 的抢救逻辑（原样搬迁，不改逻辑） |
| `NoteRepository` | 笔记及其路径记录的增删改、状态变更、排序写入、懒加载大字段 |
| `TagRepository` | 标签实体与关联、全局重命名与删除、标签统计 |
| `CategoryRepository` | 分类树增删改、排序、置顶、预设标签 |
| `CategoryLockService` | 分类密码、解锁状态、锁定集合缓存 |
| `TodoRepository` | 待办 |
| `NoteQueryService` | 列表、计数、统计、系统分类计数、回收站视图、流式导出读取 |
| `NoteIngestService` | 一切"创建笔记"的业务规则：类型判定、标题生成、标签与分类自动归档、按内容哈希去重合并 |
| `AppSettings` | 自动归档开关、自动归档目标分类、隐藏加锁分类（沿用现有 QSettings 的组织名、应用名和键名） |
| `TagClipboard` | 标签剪贴板 |
| `DataEvents` | 数据变更信号（沿用现有信号名和参数）。事务提交后发射，一个业务操作最多发射一次 |
| `BackupService` | 备份、恢复入口 |
| `LicenseService` | 授权（原样搬迁） |
| `ClipboardMonitor` | 只负责监听系统剪贴板并发出原始数据 |
| `PasteInjector` | 激活目标窗口并模拟 Ctrl+V（延时等时序参数保持现有数值） |

### 2.3 数据库终态表结构

- `notes`：`id`、`title`、`content`、`item_type`、`content_hash`、`category_id`、`color`、`rating`、`is_pinned`、`is_favorite`、`is_deleted`、`source_app`、`source_title`、`remark`、`sort_order`、`word_count`、`created_at`、`updated_at`、`last_accessed_at`、`data_blob`（图片原始数据）、`thumb_blob`（图片缩略图）。字段类型和默认值沿用现有定义。
  - 删除列：`tags`、`file_extensions`；`is_locked` 仅当全工程搜索确认没有任何读取时才删除。
  - 文件/文件夹条目的 `content` 存空；读取时由路径记录拼成以 `;` 连接的字符串放进返回结果的 `content` 键，保证界面代码读取到的内容与现在一致。
- `note_paths`：`note_id`、`ord`、`path`、`is_dir`、`ext`；主键 `(note_id, ord)`。`ext` 为小写后缀，无后缀或文件夹时为空。
- `tags`：`id`、`name`（唯一）。
- `note_tags`：`note_id`、`tag_id`、`ord`；主键 `(note_id, tag_id)`。
- `categories`、`todos`、`system_config`：结构不变。
- 索引：保留现有的 `idx_notes_main_filter`、`idx_notes_rating`、`idx_notes_content_hash`；新增 `notes(date(created_at))`、`notes(date(updated_at))`、`notes(last_accessed_at)`、`note_tags(tag_id, note_id)`、`note_paths(ext)`。
- 返回给界面的笔记记录，键名与现有一致（`tags` 由 `note_tags` 按 `ord` 拼成，`content` 见上）。
- 列表类查询不返回 `data_blob`；图片行返回 `thumb_blob`。需要原图时按 id 懒加载。
- PRAGMA：保持 `journal_mode=WAL`、`synchronous=FULL` 不变。通过"每个业务操作恰好一个事务"来消除多余刷盘。

---

## 第三部分：步骤

### 步骤 1：词表与纯函数（不改存储）

**目标**：把散落的规则收成唯一实现。本步骤不改表结构，不改 SQL。

**做什么**
1. 新建 `ItemType`：枚举加字符串转换。先全工程搜索所有写入和比较 `item_type` 的位置，保留所有实际有生产者的类型，同时定义回收站使用的伪类型 `deleted_category`。文件类旧类型（`file`、`files`、`folder`、`folders`、`local_file`、`local_folder`、`local_batch`）本步骤暂时保留，在步骤 3 中合并删除，这是计划内的中间状态。
2. 新建 `ItemClassifier`：把以下位置的判定规则合并为一组谓词（`isUrl`、`isHexColor`、`isRgbTriplet`、`isCodeLike`、`isAbsolutePathText` 等），并提供两个入口：入库用的类型判定、界面图标用的显示类别（显示类别判定不得访问磁盘）。
   - 来源：`main.cpp` 的剪贴板处理 lambda、`NoteModel::data` 的图标判定、`StringUtils::detectItemType`、`DatabaseManager` 中的类型相关判断。
   - 合并的是实现，不是改变各调用点原有的判定范围。各调用点原来如何判定，现在结果必须相同。
3. 新建 `PathSummary`：由路径列表得出种类（单文件、单文件夹、多文件或混合、多文件夹），`NoteModel` 现有的四种图标（图标名和颜色不变）改为由它决定。
4. 新建 `WordBucket`、`TagCodec`、`ThumbnailMaker`（缩略图缩放参数使用 `NoteModel` 中现有的 64×64、保持纵横比、平滑缩放，原样搬来）。
5. 删除所有重复实现，所有调用点改用新模块。

**不做**：不改表结构；不新增类型；不修改图标名和颜色。

**验收**
- 全工程搜索 `item_type` 的字符串字面量，只出现在 `ItemType` 内。
- 全工程只有 `TagCodec` 里出现对标签的 `split`/`join`。
- `StringUtils::detectItemType` 已删除。
- 各原调用点的判定结果与改动前一致（用代码对照说明）。
- `docs/SSOT接口索引.md` 已同步本步骤新增、修改、删除的接口，头文件注释与索引一致。

---

### 步骤 2：模块拆分（不改表结构，不改 SQL 语义）

**目标**：把 `DatabaseManager` 按 2.2 的职责拆开并删除，行为完全不变。

**做什么**
1. 按 2.2 新建模块，把 `DatabaseManager` 里的全部方法归入唯一归属：
   - 笔记增删改、状态、排序写入、`recordAccess`、`getNoteById`、`getLastCreatedNoteId`、回收站相关写入 → `NoteRepository`。
   - `addNote` 里的业务规则（按哈希去重合并、已有分类优先保留、标签并集、仅当原标题是通用标题时才替换、`link` 类型自动归入 "Link" 分类、分类预设标签合并）→ 移入 `NoteIngestService`，由它调用 `NoteRepository` 的原子操作。`NoteRepository` 不再包含任何业务规则。规则内容、判定集合、写入的字段（含"重复命中时不更新 `updated_at`"）保持不变。
   - 标签相关 → `TagRepository`；分类相关 → `CategoryRepository`；密码与解锁 → `CategoryLockService`；待办 → `TodoRepository`；授权 → `LicenseService`（原样搬迁）；旧外壳抢救 → `LegacyDatabaseRescue`（原样搬迁）；标签剪贴板 → `TagClipboard`；自动归档开关等 QSettings 状态 → `AppSettings`；信号 → `DataEvents`。
   - 查询相关（`searchNotes`、`getNotesCount`、`getFilterStats`、`getCounts`、`applyCommonFilters`、`applySecurityFilter`、`DEFAULT_PAGE_SIZE`）→ 先整体移入 `NoteQueryService`，SQL 暂不改。
2. `beginBatch`/`endBatch`/`rollbackBatch` 删除，改为 `TransactionScope`：嵌套时内层加入外层事务，只在最外层提交或回滚。每个对外的写接口恰好一个事务（含现在没有事务的 `addNote`、`updateNote`、`syncNoteTags`、`recordAccess` 等）。
   - 现有的 `beginBatch`/`endBatch` 里与授权相关的两处调用（缓存授权状态、写授权文件）不保留。授权文件只在激活和重置时写入（这两处原有行为不变）。在汇报中单独说明这一处行为变化。
3. 信号：事务提交后由 `DataEvents` 发射，沿用现有信号名和参数。同一个业务操作内部不得多次发射。`addNote` 新建后单独的 `recordAccess`（多一次 UPDATE 和一次 `noteUpdated`）合并进插入语句本身（插入时直接写 `last_accessed_at`）。
4. 依赖方向：核心层不得包含 `src/ui` 下的头文件（删除 `StringUtils.h`、`FramelessDialog.h` 等包含）；`DatabaseManager` 删除记录时不得再调用 `ClipboardMonitor`。改为：`NoteRepository` 删除后发出 `DataEvents` 信号，`ClipboardMonitor` 订阅该信号清除自己的最后哈希（连接在应用装配处完成）。
5. `ClipboardMonitor::m_lastHash` 的所有读写（含清除）都必须在 `m_hashMutex` 保护下进行。
6. 更新所有调用处（`src/ui`、`src/models`、`src/core` 中的其他文件、`src/main.cpp`），所有对 `DatabaseManager::instance()` 的引用消失。本步骤结束时不得保留任何转发壳。
   - 先全工程搜索 `DatabaseManager` 的全部引用，在汇报中给出所用搜索命令、涉及文件数与引用处数。
   - 在 `docs/SSOT接口索引.md` 中建立"旧方法 → 新模块与方法"的完整映射表，覆盖 `DatabaseManager` 的每一个公开方法，每个旧方法只有一个新归属。
   - 调用处按映射表机械替换，可以分批进行，但交付时必须是一次完整、可编译、且 `DatabaseManager` 已删除的结果。
   - 完成后用同一条搜索命令验证结果为零。

**不做**：不改表结构、不改 SQL 的查询结果、不改排序、不改颜色取值、不改授权逻辑。

**验收**
- 工程中不再存在 `DatabaseManager` 类。
- `QSqlQuery` 只出现在：`NoteRepository`、`TagRepository`、`CategoryRepository`、`CategoryLockService`、`TodoRepository`、`NoteQueryService`、`SchemaMigrator`、`ConnectionManager`、`LicenseService`、`BackupService` 内。
- `src/core` 下没有任何文件包含 `src/ui` 的头文件。
- 每个对外写方法恰好一个事务（逐个方法列表说明）。
- `docs/SSOT接口索引.md` 已同步本步骤新增、修改、删除的接口，头文件注释与索引一致。

---

### 步骤 3：表结构与迁移（标签唯一来源、路径条目、缩略图、索引）

**目标**：落地 2.3 的终态表结构，并把现有真实数据迁移过去。

**做什么**
1. `SchemaMigrator` 用 `PRAGMA user_version` 管理版本，替代现在散落的 `system_config` 迁移标记。启动时检测到旧结构（`user_version` 为 0 且存在 `notes.tags` 列）才执行迁移。
2. 迁移流程：
   1. 先用 `VACUUM INTO` 在 `backups` 目录生成 `pre_migration_<yyyyMMdd_HHmmss>.db`（只生成一次）。
   2. 在一个事务内完成全部迁移，包括：
      - 重建 `notes` 表（去掉 `tags`、`file_extensions` 以及按 2.3 判定可删的 `is_locked`；新增 `thumb_blob`），复制原有行。
      - 标签：以旧 `notes.tags` 字符串为准（它是界面显示和搜索使用的那份），用 `TagCodec` 拆分，补全 `tags` 表，重建 `note_tags`（`ord` 为拆分顺序）。
      - 路径条目：旧类型 `file`、`files`、`folder`、`folders` 的 `content` 按 `;` 拆成路径；旧类型 `local_file`、`local_folder`、`local_batch`，若 `source_title` 是绝对路径则用它作路径，否则把 `content` 原样作为路径（这类记录将成为失效路径）。全部合并为新类型 `path`，写入 `note_paths`（`is_dir` 和 `ext` 通过读取文件信息得到；路径不存在时 `is_dir` 为 0，`ext` 取后缀）。这些记录的 `data_blob` 不迁移，置空。不迁移文件本体。
      - 原本为 `text` 类型、但整段内容（去引号后）是一个存在的绝对路径的记录，按 `ItemClassifier` 的入库类型判定转为路径条目（与旧版界面显示和发送时的行为保持一致）。汇报中给出转换数量。
      - `content_hash`：路径条目重新计算为"路径按 `;` 连接后的 UTF-8"的 SHA-256；其他条目不变。
      - `word_count`、颜色、评分、置顶、收藏、分类、时间字段全部原样保留。`source_title` 不改写。
      - 建立新索引，设置 `user_version`。
   3. 校验：笔记行数一致；每条旧标签名都能在新 `note_tags` 中找到；失败则回滚、保持旧库不变，并通过现有的初始化失败路径返回错误（不要新增任何界面）。
3. 全部仓库与查询代码按新结构改写：
   - 路径条目的创建、读取、删除都在同一事务内同时处理 `notes` 与 `note_paths`；删除笔记时同一事务内清理 `note_tags`、`note_paths`。
   - 合并类型：`ItemType` 删除文件类旧类型，统一为 `path`；全工程所有比较旧类型字符串的位置（界面、模型、工具类）改为 `ItemType` 判定。
   - `getAllTags`、`getRecentTagsWithCounts` 直接由 `tags`、`note_tags`、`notes` 联表聚合得出（不再扫描并解析字符串），保持"只统计未删除的笔记"以及现有的排序结果。
   - 批量接口改为集合式：批量状态更新用单条 `UPDATE ... WHERE id IN (...)`（超出变量上限时分块，仍在同一事务内）；批量移动分类与预设标签合并用 `INSERT OR IGNORE ... SELECT`，不得逐条查询；手动排序（`moveNote`、`moveNotesToRow`、`reorderNotes`、`reorderCategories`、`updateCategoryOrder`）一次 prepare、循环绑定、只更新 `sort_order` 实际变化的行、整个操作一个事务。
4. 缩略图：新增图片时（入库前的后台计算阶段）生成 `thumb_blob`；已有图片在迁移完成后由后台线程（独立连接）回填，每批一个事务，全部完成后只发射一次变更信号。回填完成前，缺少缩略图的图片行使用 `NoteModel` 中现有的图片占位图标。

**不做**：不改 `ThumbnailMaker` 的缩放参数；不迁移文件本体；不删除 `attachments` 目录和任何旧文件；不新增失效路径的界面提示。

**验收**
- 全工程不存在对 `notes.tags` 列和 `file_extensions` 列的任何读写。
- 迁移前后笔记行数一致，标签集合一致（给出核对方法）。
- 不再存在 `local_*`、`file`、`files`、`folder`、`folders` 类型字符串。
- 批量接口内不存在循环内 `prepare`、不存在按条目重复 SELECT。
- `docs/SSOT接口索引.md` 已同步本步骤新增、修改、删除的接口，头文件注释与索引一致。

---

### 步骤 4：读取路径（查询服务重写、模型适配）

**目标**：搜索、计数、统计共用同一个过滤构造器；列表不再读取大字段；界面模型不再做磁盘访问。

**做什么**
1. `NoteQueryService` 对外接口：`fetchPage(请求)`、`count(请求)`、`stats(请求)`、`systemCounts()`、`streamNotes(请求, 回调)`（带大字段，逐行回调，用于导出）。`请求` 包含关键词、过滤类型、过滤值、`criteria`（其键和值格式与界面筛选面板现在产生的完全一致，不得要求界面改动）、排序覆盖、页码、每页大小。
2. 唯一的过滤构造器同时服务 `fetchPage`、`count`、`stats` 和手动排序时取视图顺序。关键词按空白拆分，每个词必须命中标题、备注、标签、后缀（后缀来自路径记录）之一，计数与统计使用同一套。排序规则集中到 `SortPolicy`，保持现有各视图的排序结果。保持 `LIKE` 方式，不引入 FTS；删除所有声称"已使用 FTS5 触发器"的过期注释。
   - 现状里 `getNotesCount` 把整个关键词当作一个词，`searchNotes` 却按空白拆成多个词，两者口径不一致。统一后计数与列表都按"拆词、每词必须命中"处理，以列表的行为为准。在汇报中单独说明这一处。
   - 现状里 `criteria` 的 `stars` 是把字符串直接拼进 SQL，统一后全部使用绑定参数，取值语义不变。
   - 旧 `file_extensions` 列参与搜索与"类型"筛选，其内容由旧 `extractFileExtensions` 产生（文件为小写后缀，文件夹为"文件夹"，`link` 类型为"Link"）。删除该列后，这个"后缀标签"由唯一一处表达式从 `note_paths`（以及 `link` 类型）推导，搜索、类型筛选、类型统计三处共用，推导结果必须与旧列一致。
   - 排序规则集中到 `SortPolicy` 时，必须逐条保留现有行为：自定义排序（`sort_by` + `sort_order`）优先；`recent_notes_window` 标记时完全不加置顶；`all` 及非 `recently_visited`、非 `category` 的视图加 `is_pinned DESC`；`recently_visited` 无自定义排序时固定为 `last_accessed_at DESC`；分类视图按该分类的排序模式（`created_at_desc`、`updated_at_desc`、`title_asc`、`title_desc`，其余为 `is_pinned DESC, sort_order ASC, updated_at DESC`）；其余视图为 `is_pinned DESC, sort_order ASC, updated_at DESC`。`sort_by` 只允许白名单列名。
3. `stats` 对过滤后的结果集只用一个公共表表达式，总共不超过 4 条语句：一次紧凑列扫描（收藏、评分、字数阶梯、颜色、类型、创建日期、更新日期），在 C++ 里单次循环聚合；标签统计与后缀统计各一条联表聚合。输出的键与结构保持现状（包括 `__untagged__`）。不再对 SQL 文本做字符串替换。
   - 类型统计：图片、脚本代码、纯文本、Link、文件夹、各后缀、其他，沿用现有标签文案。路径条目中既无后缀又不是文件夹者，计入"其他"，筛选"其他"时同样命中它们。已无数据来源的"数据库附件""本地文件""批量托管"三个统计项不再产生；如果在界面文件里发现对它们的硬编码引用，不要修改，在汇报里列出。
4. `systemCounts()`：用一条条件聚合语句加一条分类分组语句得出现有的全部计数键（`all`、`today`、`yesterday`、`recently_visited`、`uncategorized`、`untagged`、`bookmark`、`trash`、`cat_<id>`），锁定分类集合只取一次（来自 `CategoryLockService` 缓存）。
5. 回收站视图保持现有的"笔记 + 已删除分类"并集输出，字段不变。
6. `NoteModel`：
   - `data()` 内不再访问磁盘、不再解码整张图：文件夹或文件的图标由 `PathSummary` 与存储的 `is_dir` 决定；图片缩略图来自 `thumb_blob`。
   - `BlobRole` 与工具提示里的图片预览改为通过 `NoteRepository` 按 id 懒加载。
   - 每个角色返回的值在相同数据下与现在完全相同（图标名、颜色、工具提示的结构与文案、缓存上限 500 都不变）。
   - 删除只写不读、从未生效的 `m_tooltipCache`。
7. `CategoryModel::refresh`：`systemCounts()` 与分类列表各查询一次；加锁状态取自 `CategoryLockService` 缓存，不再逐个分类查询；显示结果不变。
8. 导出相关调用改用 `streamNotes`。
9. `QuickWindow::updateContextSnapshotById` 现在每次都用 `searchNotes` 取整个视图再取前后各 5 条。改为由 `NoteQueryService` 提供"按 id 取视图中前后各 N 条"的接口（使用同一个过滤构造器与 `SortPolicy`，结果与现在相同），不再加载整个视图。

**不做**：不修改 `FilterPanel` 与任何界面文件的参数、样式；不改变每页大小的取值（500）。

**验收**
- 全工程只有 `NoteQueryService` 拼装 WHERE 和 ORDER BY。
- 同一组条件下，`count` 与 `fetchPage` 的总数一致，`stats` 各项合计与之相符（给出核对方法）。
- `NoteModel::data` 内没有 `QFileInfo`、没有 `loadFromData`。
- `systemCounts()` 与 `stats()` 的 SQL 语句条数符合上述上限。
- `docs/SSOT接口索引.md` 已同步本步骤新增、修改、删除的接口，头文件注释与索引一致。

---

### 步骤 5：采集与入库链路

**目标**：从系统剪贴板到入库只有一条清晰的链路，重计算不在主线程，写库一次一个事务。

**做什么**
1. `ClipboardMonitor` 只做三件事：监听变化；在主线程立即取出原始数据（本地文件路径、图片、文本）与前台窗口的句柄和进程号；发出一个不可变的原始数据结构。删除其中的来源应用解析之外的所有业务（类型判定、哈希之外的规则）。
   - 来源应用名和窗口标题，用主线程抓到的句柄和进程号在后台线程解析；"本进程内部回环拦截"使用主线程抓到的进程号判定。
   - 现有的"相同内容哈希不重复处理"保留在监听器内，作为事件级抑制；数据级去重只由数据库的 `content_hash` 负责（`NoteIngestService`）。
   - `clipboardChanged` 信号必须在主线程发射（现在是在后台线程里发射，烟花特效由此在非界面线程执行）。
2. 把 `main.cpp` 中 `newContentDetected` 处理 lambda 的全部业务规则移入 `NoteIngestService`：标题生成（`[截图] MMdd_HHmm`、`Copied File - …`、`Copied Folder - …`、`Copied Files - … 等 N 个文件` 等）、文件与文件夹判定、带引号路径、色码标签与自动归入 "Color" 分类、网址标签与域名标题、自动归档目标分类。文案与规则原样保留。`main.cpp` 只保留信号连接。
3. 所有"创建笔记"的调用方（界面里的新建、按行创建、合并、拖入、采集、CSV 导入、导入包）统一改为调用 `NoteIngestService` 的单一入口，由它负责分类判定、必要时生成缩略图、一次事务写入。图片缩略图和重计算在后台线程完成后，再交给连接所属线程做一次事务写入。
4. 删除 `addNoteAsync` 这类名不副实的接口。

**不做**：不改烟花特效、提示框的任何参数和样式；不改 `skipNext` 的机制。

**验收**
- `main.cpp` 中不再有标题生成、正则、分类逻辑。
- 一次复制产生的数据库写入恰好一个事务、至多一次变更信号。
- `ClipboardMonitor` 中不包含数据库相关代码。
- `docs/SSOT接口索引.md` 已同步本步骤新增、修改、删除的接口，头文件注释与索引一致。

---

### 步骤 6：发送、导入、导出（只存路径，不复制文件）

**目标**：文件和文件夹只记录路径，不复制本体，对齐 Ditto 的做法；发送只有一个入口。

**做什么**
1. 发送：
   - `NoteClipboardPayload` 一个函数：由笔记得出剪贴板内容（图片；存在的路径列表；文本；HTML）。路径条目只取磁盘上仍然存在的路径；全部不存在时，改为写入路径文本，并显示现有文案"[!] 原文件已丢失，已复制路径文本"。
   - 拖拽导出（`NoteModel::mimeData`）、双击发送（`QuickWindow::activateNote`、`QuickWindow::sendNote`、`RecentNotesWindow::sendNote`）、`StringUtils::copyNotesToClipboard`、`doExtractContent` 全部改用这个函数，各自按现有格式取需要的部分（拖拽包含文本与路径；剪贴板的路径条目只写路径列表）。
   - `PasteInjector`：把三处重复的"激活目标窗口 + 释放修饰键 + 模拟 Ctrl+V"收为一个实现，延时等时序参数保持现有数值。
   - 所有写剪贴板之前必须调用 `ClipboardMonitor::skipNext()`（现在多文件路径分支漏了）。
   - 双击发送时更新访问时间的操作只能是一次事务，且不得触发列表整页重新查询以外的额外刷新。如果现有界面刷新逻辑因此需要修改，不要改，在汇报里列出。
   - 删除"数据库附件"分支（把 `data_blob` 写到临时目录再发送）。该分支调用的 `verifyExportPermission()` 可能因此不再有调用方：不要删除它，全工程搜索其调用处，若已无调用，只在汇报里列出，由我决定。
2. 导入（`FileStorageHelper::processImport` 及其调用方）：
   - 不再调用任何文件复制。删除 `storeFile` 的复制逻辑、`getStorageRoot`、`attachments` 目录的创建与使用。
   - 导入文件：每个文件一条路径条目（沿用现有的标签 `导入文件`、颜色 `#2c3e50`、来源应用 `FileStorage`），经 `NoteIngestService` 写入。
   - 导入文件夹：保留"文件夹变分类、子文件夹变子分类"的结构，文件夹内每个文件各一条路径条目；`notes.csv` 的还原行为保持；独立的 `.csv` 文件仍解析为文本笔记。
   - 整个导入过程在同一个事务中，取消则回滚（不再有物理文件需要清理）。
   - 进度对话框保持现有样式和文案；因为不再处理字节，弹出进度框的条件只保留"项目数达到 50"，进度按已处理项目数与总项目数的比例换算；停止在导入前递归统计文件总大小。在汇报里单独说明这一处。
3. 导出：
   - 路径条目只导出路径文本（并入 `notes.csv`，`Content` 列为以 `;` 连接的路径），不复制文件。图片条目仍按现有方式导出为图片文件。
   - 导出包 `.rnp`：路径条目写成 `paths` 数组（不写 `content`）；`.rnp` 的加密方式不变。导入时：读到 `paths` 则还原为路径条目；读到旧包里的文件类条目，`content` 按 `;` 拆成路径还原；`content` 以 `attachments/` 开头的旧条目无法还原，跳过并用日志记录数量。
   - 导出与导入包使用 `streamNotes` 和 `NoteIngestService`，不再自行拼接 SQL 或写入逻辑。

**不做**：不新增失效路径的界面标记；不新增菜单项；不修改任何对话框的文案和样式。

**验收**
- 全工程不存在对 `QFile::copy`、`copyRecursively` 用于入库或导入的调用（导出到用户选择的目录时也不复制文件）。
- 全工程不存在 `attachments` 字符串。
- 写剪贴板的位置都紧跟在 `skipNext()` 之后（逐处列出）。
- 只存在一份 `PasteInjector` 的发送实现。
- `docs/SSOT接口索引.md` 已同步本步骤新增、修改、删除的接口，头文件注释与索引一致。

---

### 步骤 7：备份、启动自检与收尾

**目标**：备份不再拖慢主线程；做一次全局复查。

**做什么**
1. `BackupService`：
   - 最新备份：在后台线程使用独立连接执行 `VACUUM INTO` 到临时文件，再原子重命名为 `backups/inspiration_latest.db`（文件名保持不变）。保留现有的"数据库异常缩小则熔断保护"和"保留损坏文件最多 5 个"的规则。
   - 触发策略保持现有间隔和空闲判定。把现在与注释不一致的那一条改为注释所说的行为：从数据首次变脏起累计超过 120 秒必须强制同步（现在代码用的是"最后一次活动"，永远不会触发）。
   - 带时间戳的归档备份保留数量（3 个）不变。
   - 退出时的最后一次备份保持现有时机。
2. 启动自检：用 `PRAGMA quick_check` 代替启动时的完整 `integrity_check`。
3. 全局复查并汇报以下检查项的搜索结果：
   - `item_type` 字符串字面量只在 `ItemType` 中。
   - `QSqlQuery` 只在 2.2 列出的模块内出现。
   - `src/core` 不包含 `src/ui` 头文件。
   - 不存在循环内 `prepare`、不存在 N+1 查询。
   - 不存在对 `attachments` 的引用。
   - 不存在被注释掉的旧代码和 `// OLD`、`// 兼容` 之类的标记。
   - `docs/SSOT接口索引.md` 登记完整：全工程公开接口与索引逐项对得上，头文件注释与索引一致。
   - 未对任何界面参数值、样式参数值作出改动（对照 git diff 逐个界面文件说明）。

**验收**：以上各项全部给出搜索命令与结果。