# RapidNotes SSOT 接口索引文档

本文件为 RapidNotes 架构重构对外公开接口的唯一真理源 (SSOT) 索引表，严格遵循《RapidNotes_Prompts-2.md》通用规则第 6 条与《Memories_RapidNotes.md》第 10 章规范。
全工程公开接口必须在此登记，且代码头文件中的注释必须与本索引严格保持 100% 一致。

---

## 第 1 章：条目类型词表 (ItemType)

### 模块基本信息
- **模块与头文件**：`RapidNotes::ItemType` (`src/core/ItemType.h`)
- **它是哪一件事实的唯一来源**：笔记与剪贴板条目类型的字符串词表与强类型枚举映射。
- **用途与适用场景**：全工程唯一允许出现 `item_type` 字符串字面量（如 `"image"`、`"text"`、`"code"`、`"link"`、`"file"` 等）的地方，提供类型互转与类别判定。
- **禁止在别处重复实现的内容**：严禁在 `DatabaseManager`、`NoteModel`、`StringUtils` 或任何 UI/业务代码中手写类型字符串字面量或写私有判断函数。
- **主要调用方**：`DatabaseManager`、`ItemClassifier`、`NoteModel`、`StringUtils`、`NoteEditWindow`、`QuickWindow`。

### 公开接口列表
1. `QString toString(Type type)`
   - **用途**：将强类型枚举转换为数据库和网络通信所用的标准 `item_type` 字符串。
2. `Type fromString(const QString& str)`
   - **用途**：将字符串转换为强类型枚举，未知或非法类型回退为 `Type::Unknown`。
3. `bool isFileOrFolder(Type type)`
   - **用途**：判定指定类型是否属于文件或文件夹类（包括 `File`, `Files`, `Folder`, `Folders`, `LocalFile`, `LocalFolder`, `LocalBatch`）。
4. `bool isPureText(Type type)`
   - **用途**：判定指定类型是否属于纯文本类（包括 `Text`, `Code`, `OcrText`, `CapturedMessage`）。

---

## 第 2 章：文本特征与类型判定 (ItemClassifier)

### 模块基本信息
- **模块与头文件**：`RapidNotes::ItemClassifier` (`src/core/ItemClassifier.h`)
- **它是哪一件事实的唯一来源**：文本内容特征嗅探、入库类型判定以及视图渲染显示类别。
- **用途与适用场景**：剪贴板内容采集入库时的类型判定，以及列表项图标渲染的纯内存类别判定。
- **禁止在别处重复实现的内容**：严禁在视图层调用磁盘 I/O（`QFileInfo` 等）进行图标或类型探测；严禁重复编写 URL、RGB、十六进制颜色、代码特征的正则表达式。
- **主要调用方**：`main.cpp`、`NoteModel`、`FileStorageHelper`、`NoteIngestService`。

### 公开接口列表
1. `bool isUrl(const QString& text)`
   - **用途**：基于前缀特征嗅探文本是否为网址链接（`http://`, `https://`, `www.`）。
2. `bool isHexColor(const QString& text)`
   - **用途**：嗅探文本是否为十六进制色码（`#RGB` 或 `#RRGGBB`）。
3. `bool isRgbTriplet(const QString& text, int* r = nullptr, int* g = nullptr, int* b = nullptr)`
   - **用途**：嗅探文本是否为三元组 RGB 颜色，并解析输出分量。
4. `bool isCodeLike(const QString& text)`
   - **用途**：嗅探纯文本是否具有代码特征（关键字、HTML 标签、JSON 等）。
5. `bool isAbsolutePathText(const QString& text)`
   - **用途**：嗅探文本是否为磁盘绝对路径。
6. `ItemType::Type detectIngestType(const QString& text)`
   - **用途**：数据采集入库时的类型决策纯函数。
7. `DisplayCategory detectDisplayCategory(const QString& itemTypeStr, const QString& content)`
   - **用途**：视图模型列表渲染专用的显示分类计算（返回图标名与图标颜色，纯内存计算，严禁磁盘 I/O）。

---

## 第 3 章：路径种类与图标归并 (PathSummary)

### 模块基本信息
- **模块与头文件**：`RapidNotes::PathSummary` (`src/core/PathSummary.h`)
- **它是哪一件事实的唯一来源**：多路径集合或分号分隔路径的种类判定与图标/颜色归并。
- **用途与适用场景**：多文件/文件夹条目在模型层（`NoteModel`）及后续路径条目（`note_paths`）的图标与视觉归并。
- **禁止在别处重复实现的内容**：严禁在外部写 `if (isDir) ... else ...` 散落拼接图标名和硬编码颜色值。
- **主要调用方**：`NoteModel`、`StringUtils`。

### 公开接口列表
1. `Kind summarize(const QStringList& paths)`
   - **用途**：对路径列表进行统计归并，返回 `SingleFile`, `SingleFolder`, `MultipleFilesOrMixed`, `MultipleFolders`。
2. `Kind summarizeFromSemicolonString(const QString& content)`
   - **用途**：对分号连接的多路径内容进行归并分析。
3. `QString getIconName(Kind kind)`
   - **用途**：返回该种类对应的 SVG 图标名（`file`, `folder`, `files_multiple`, `folders_multiple`）。
4. `QString getIconColor(Kind kind)`
   - **用途**：返回该种类对应的标准主题颜色 Hex（`#FFFF00`, `#FF8C00`, `#FF0000`, `#483D8B`）。

---

## 第 4 章：字数阶梯计算算法 (WordBucket)

### 模块基本信息
- **模块与头文件**：`RapidNotes::WordBucket` (`src/core/WordBucket.h`)
- **它是哪一件事实的唯一来源**：笔记内容字数清洗与阶梯桶位（Word Bucket）映射算法。
- **用途与适用场景**：入库和更新笔记时计算 `word_count` 字段，用于侧边栏字数阶梯筛选与统计。
- **禁止在别处重复实现的内容**：严禁在 `DatabaseManager` 或其他模块私自编写清洗 HTML/空白的字数计算或分档阶梯。
- **主要调用方**：`DatabaseManager`、后续 `NoteIngestService`。

### 公开接口列表
1. `int calculate(const QString& content, const QString& itemType)`
   - **用途**：根据内容与条目类型，清洗 HTML 标签、实体与空白字符后，返回所属的字数阶梯档位（0, 10~100 每10字一档, 200~1000 每100字一档, 1001 代表 1000 字以上；非正文类型返回 0）。

---

## 第 5 章：标签编解码与格式唯一真理源 (TagCodec)

### 模块基本信息
- **模块与头文件**：`RapidNotes::TagCodec` (`src/core/TagCodec.h`)
- **它是哪一件事实的唯一来源**：全工程标签的拆分、拼接、规范化与去重。
- **用途与适用场景**：标签字符串与列表的双向流转，统一处理中英文逗号、空白清洗与保序去重。
- **禁止在别处重复实现的内容**：严禁在任何地方直接调用 `.split(",")`、`.split(QRegularExpression(...))` 或 `.join(",")`；严禁私自编写标签 trim + 去重循环。
- **主要调用方**：`DatabaseManager`、`QuickWindow`、`NoteEditWindow`、`AdvancedTagSelector`、`TagEditDialog`、`FramelessDialog`、`FileStorageHelper`。

### 公开接口列表
1. `QStringList split(const QString& tagsStr)`
   - **用途**：拆分标签字符串（支持半角逗号 `,` 与全角逗号 `，`），自动进行 trim、去空与保序去重。
2. `QStringList normalize(const QStringList& tags)`
   - **用途**：对已有标签列表进行逐项 trim、剔除空项并保序去重。
3. `QString join(const QStringList& tags)`
   - **用途**：持久化与数据存储用拼接，严格使用纯半角逗号 `,` 无空格连接。
4. `QString joinDisplay(const QStringList& tags)`
   - **用途**：界面展示与文本框回填专用，使用半角逗号加空格 `, ` 连接。

---

## 第 6 章：图片缩略图生成算法 (ThumbnailMaker)

### 模块基本信息
- **模块与头文件**：`RapidNotes::ThumbnailMaker` (`src/core/ThumbnailMaker.h`)
- **它是哪一件事实的唯一来源**：图片 64×64 平滑缩略图与图标生成。
- **用途与适用场景**：入库预处理与视图缓存渲染时的高性能平滑缩略图生成。
- **禁止在别处重复实现的内容**：严禁在 UI 模型中内嵌手写缩放参数或解码整图生成图标。
- **主要调用方**：`NoteModel`、后台入库计算流水线。

### 公开接口列表
1. `QImage makeThumbnail(const QImage& sourceImage, const QSize& targetSize = QSize(64, 64))`
   - **用途**：按保持纵横比与平滑缩放算法（`Qt::SmoothTransformation`）生成缩略图 QImage。
2. `QIcon makeThumbnailIcon(const QImage& sourceImage, const QSize& targetSize = QSize(64, 64))`
   - **用途**：将缩略图转换为可直接用于 Model DecorationRole 的 QIcon。
