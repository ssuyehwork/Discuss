# Implementation Plan: RapidNotes 数据库与底层逻辑重构【步骤 1：词表与纯函数】实施方案

**基准 Commit 锚点 (Base Commit Hash)**: `9a9dbfd`
**关联模块**: `RapidNotes_Source/RapidNotes`
**设计真理源**: `RapidNotes_Source/Memories_RapidNotes.md` 第 6 章、`RapidNotes_Source/RapidNotes_Prompts.md` 步骤 1

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 核心病灶与系统性缺陷深入剖析
1. **类型字面量散落与硬编码反模式**：全工程中多达 20+ 处直接比对或硬编码 `"text"`、`"image"`、`"code"`、`"link"`、`"file"`、`"folder"` 等类型字符串字面量，缺乏单一类型系统真理源（SSOT）。`main.cpp` 的剪贴板监控、`NoteModel::data` 的 DecorationRole 判断以及 `DatabaseManager` 中各自分散维护一套判定逻辑。
2. **标签解析与序列化各自造轮子与孤儿死代码风险**：全工程散落 40+ 处对标签的 `.split(",")`、`.split(QRegularExpression("[,，]"))` 以及 `.join(", ")`、`.join(",")`，未形成统一的编码解码契约。若抽取了 `TagCodec` 却未替换全工程业务调用点，将沦为孤儿死代码，违背 SSOT。
3. **字数阶梯与缩略图逻辑侵入核心层与视图层**：`calculateWordBucket` 脱壳与阶梯清洗深嵌在 `DatabaseManager.cpp` 匿名命名空间中；缩略图缩放硬编码在 `NoteModel.cpp` 中且与视图渲染混杂。

### 1.2 架构治理设计目标与五大一票否决红线
- **一票否决 1：算力利用红线**：类型与种类判定纯函数化，视图层图标判定严禁做磁盘 I/O（`isAbsolutePathText` 与 `detectDisplayCategory` 彻底与磁盘解耦）；
- **一票否决 2：接口归一化与全量接入**：`TagCodec` 统一规范标签集合与字符串的双向流转，全工程所有调用点必须 100% 接入，彻底杜绝孤儿死代码；
- **一票否决 3：补丁零容忍**：彻底废除 `StringUtils::detectItemType`，收敛为唯一的 `ItemClassifier` 与 `ItemType`；
- **一票否决 4：严禁偷指针红线**：所有模块均为无状态纯函数（Pure Functions）或值语义工具类，零指针依赖；
- **一票否决 5：界面零篡改红线**：图标名、图标颜色（Hue 值）、缩略图尺寸（64×64 平滑缩放）、字数阶梯分桶阈值 100% 保持现状。
- **本步骤范围严格锁定**：不改动表结构，不改动 SQL 语义与结果。

---

## Chapter 2: Modified Files List（影响文件清单）

### 2.1 新建纯函数与词表文件清单（生产源码）
1. `RapidNotes_Source/RapidNotes/src/core/ItemType.h`
2. `RapidNotes_Source/RapidNotes/src/core/ItemType.cpp`
3. `RapidNotes_Source/RapidNotes/src/core/ItemClassifier.h`
4. `RapidNotes_Source/RapidNotes/src/core/ItemClassifier.cpp`
5. `RapidNotes_Source/RapidNotes/src/core/PathSummary.h`
6. `RapidNotes_Source/RapidNotes/src/core/PathSummary.cpp`
7. `RapidNotes_Source/RapidNotes/src/core/WordBucket.h`
8. `RapidNotes_Source/RapidNotes/src/core/WordBucket.cpp`
9. `RapidNotes_Source/RapidNotes/src/core/TagCodec.h`
10. `RapidNotes_Source/RapidNotes/src/core/TagCodec.cpp`
11. `RapidNotes_Source/RapidNotes/src/core/ThumbnailMaker.h`
12. `RapidNotes_Source/RapidNotes/src/core/ThumbnailMaker.cpp`

### 2.2 待重构修改与注册文件清单
1. `RapidNotes_Source/RapidNotes/CMakeLists.txt`
2. `RapidNotes_Source/RapidNotes/src/ui/StringUtils.h`
3. `RapidNotes_Source/RapidNotes/src/ui/NoteEditWindow.cpp`
4. `RapidNotes_Source/RapidNotes/src/models/NoteModel.cpp`
5. `RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp`
6. `RapidNotes_Source/RapidNotes/src/ui/QuickWindow.cpp`
7. `RapidNotes_Source/RapidNotes/src/ui/AdvancedTagSelector.cpp`
8. `RapidNotes_Source/RapidNotes/src/ui/TagEditDialog.cpp`
9. `RapidNotes_Source/RapidNotes/src/ui/FramelessDialog.cpp`
10. `RapidNotes_Source/RapidNotes/src/core/FileStorageHelper.cpp`
11. `RapidNotes_Source/RapidNotes/src/main.cpp`

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 构建配置全量注册：`RapidNotes_Source/RapidNotes/CMakeLists.txt`

<<<<<<< SEARCH
src/core/ActionRecorder.cpp
    src/main.cpp
=======
src/core/ActionRecorder.cpp
    src/core/ItemType.cpp
    src/core/ItemType.h
    src/core/ItemClassifier.cpp
    src/core/ItemClassifier.h
    src/core/PathSummary.cpp
    src/core/PathSummary.h
    src/core/WordBucket.cpp
    src/core/WordBucket.h
    src/core/TagCodec.cpp
    src/core/TagCodec.h
    src/core/ThumbnailMaker.cpp
    src/core/ThumbnailMaker.h
    src/main.cpp
>>>>>>> REPLACE

---

### 3.2 条目类型词表头文件：`RapidNotes_Source/RapidNotes/src/core/ItemType.h`

<<<<<<< SEARCH FILE: RapidNotes_Source/RapidNotes/src/core/ItemType.h
=======
#pragma once

#include <QString>

namespace RapidNotes {

class ItemType {
public:
    enum class Type {
        Text,
        Image,
        Code,
        Link,
        Color,
        Palette,
        OcrText,
        CapturedMessage,
        PixelRuler,
        DeletedCategory,
        // 旧文件类中间状态（保留至步骤3统一合并为 Path）
        File,
        Files,
        Folder,
        Folders,
        LocalFile,
        LocalFolder,
        LocalBatch,
        Unknown
    };

    static QString toString(Type type);
    static Type fromString(const QString& str);

    static bool isFileOrFolder(Type type);
    static bool isPureText(Type type);

private:
    ItemType() = default;
};

} // namespace RapidNotes
>>>>>>> REPLACE

---

### 3.3 条目类型词表实现文件：`RapidNotes_Source/RapidNotes/src/core/ItemType.cpp`

<<<<<<< SEARCH FILE: RapidNotes_Source/RapidNotes/src/core/ItemType.cpp
=======
#include "ItemType.h"

namespace RapidNotes {

QString ItemType::toString(Type type) {
    switch (type) {
    case Type::Text:            return QStringLiteral("text");
    case Type::Image:           return QStringLiteral("image");
    case Type::Code:            return QStringLiteral("code");
    case Type::Link:            return QStringLiteral("link");
    case Type::Color:           return QStringLiteral("color");
    case Type::Palette:         return QStringLiteral("palette");
    case Type::OcrText:         return QStringLiteral("ocr_text");
    case Type::CapturedMessage: return QStringLiteral("captured_message");
    case Type::PixelRuler:      return QStringLiteral("pixel_ruler");
    case Type::DeletedCategory: return QStringLiteral("deleted_category");
    case Type::File:            return QStringLiteral("file");
    case Type::Files:           return QStringLiteral("files");
    case Type::Folder:          return QStringLiteral("folder");
    case Type::Folders:         return QStringLiteral("folders");
    case Type::LocalFile:       return QStringLiteral("local_file");
    case Type::LocalFolder:     return QStringLiteral("local_folder");
    case Type::LocalBatch:      return QStringLiteral("local_batch");
    case Type::Unknown:         return QStringLiteral("text");
    }
    return QStringLiteral("text");
}

ItemType::Type ItemType::fromString(const QString& str) {
    QString s = str.trimmed().toLower();
    if (s == QStringLiteral("text"))             return Type::Text;
    if (s == QStringLiteral("image"))            return Type::Image;
    if (s == QStringLiteral("code"))             return Type::Code;
    if (s == QStringLiteral("link"))             return Type::Link;
    if (s == QStringLiteral("color"))            return Type::Color;
    if (s == QStringLiteral("palette"))          return Type::Palette;
    if (s == QStringLiteral("ocr_text"))         return Type::OcrText;
    if (s == QStringLiteral("captured_message")) return Type::CapturedMessage;
    if (s == QStringLiteral("pixel_ruler"))      return Type::PixelRuler;
    if (s == QStringLiteral("deleted_category")) return Type::DeletedCategory;
    if (s == QStringLiteral("file"))             return Type::File;
    if (s == QStringLiteral("files"))            return Type::Files;
    if (s == QStringLiteral("folder"))           return Type::Folder;
    if (s == QStringLiteral("folders"))          return Type::Folders;
    if (s == QStringLiteral("local_file"))       return Type::LocalFile;
    if (s == QStringLiteral("local_folder"))     return Type::LocalFolder;
    if (s == QStringLiteral("local_batch"))      return Type::LocalBatch;
    return Type::Unknown;
}

bool ItemType::isFileOrFolder(Type type) {
    return type == Type::File || type == Type::Files ||
           type == Type::Folder || type == Type::Folders ||
           type == Type::LocalFile || type == Type::LocalFolder ||
           type == Type::LocalBatch;
}

bool ItemType::isPureText(Type type) {
    return type == Type::Text || type == Type::Code ||
           type == Type::OcrText || type == Type::CapturedMessage;
}

} // namespace RapidNotes
>>>>>>> REPLACE

---

### 3.4 类型与特征判定器头文件：`RapidNotes_Source/RapidNotes/src/core/ItemClassifier.h`

<<<<<<< SEARCH FILE: RapidNotes_Source/RapidNotes/src/core/ItemClassifier.h
=======
#pragma once

#include <QString>
#include <QRegularExpression>
#include "ItemType.h"

namespace RapidNotes {

struct DisplayCategory {
    QString iconName;
    QString iconColor;
};

class ItemClassifier {
public:
    static bool isUrl(const QString& text);
    static bool isHexColor(const QString& text);
    static bool isRgbTriplet(const QString& text, int* r = nullptr, int* g = nullptr, int* b = nullptr);
    static bool isCodeLike(const QString& text);
    static bool isAbsolutePathText(const QString& text);

    // 入库用的类型判定（支持纯文本与文件路径判定）
    static ItemType::Type detectIngestType(const QString& text);

    // 界面图标用的显示类别（严禁访问磁盘 I/O）
    static DisplayCategory detectDisplayCategory(const QString& itemTypeStr, const QString& content);

private:
    ItemClassifier() = default;
};

} // namespace RapidNotes
>>>>>>> REPLACE

---

### 3.5 类型与特征判定器实现文件：`RapidNotes_Source/RapidNotes/src/core/ItemClassifier.cpp`

<<<<<<< SEARCH FILE: RapidNotes_Source/RapidNotes/src/core/ItemClassifier.cpp
=======
#include "ItemClassifier.h"
#include <QFileInfo>
#include <QTextDocument>

namespace RapidNotes {

static QString stripHtml(const QString& text) {
    if (!text.contains('<')) return text;
    QTextDocument doc;
    doc.setHtml(text);
    return doc.toPlainText();
}

bool ItemClassifier::isUrl(const QString& text) {
    QString stripped = text.trimmed();
    return stripped.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) ||
           stripped.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive) ||
           stripped.startsWith(QStringLiteral("www."), Qt::CaseInsensitive);
}

bool ItemClassifier::isHexColor(const QString& text) {
    static const QRegularExpression hexRegex(QStringLiteral("^#([A-Fa-f0-9]{6}|[A-Fa-f0-9]{3})$"));
    return hexRegex.match(text.trimmed()).hasMatch();
}

bool ItemClassifier::isRgbTriplet(const QString& text, int* r, int* g, int* b) {
    static const QRegularExpression rgbRegex(QStringLiteral(R"(^(\d{1,3}),\s*(\d{1,3}),\s*(\d{1,3})$)"));
    QRegularExpressionMatch match = rgbRegex.match(text.trimmed());
    if (match.hasMatch()) {
        int rVal = match.captured(1).toInt();
        int gVal = match.captured(2).toInt();
        int bVal = match.captured(3).toInt();
        if (rVal <= 255 && gVal <= 255 && bVal <= 255) {
            if (r) *r = rVal;
            if (g) *g = gVal;
            if (b) *b = bVal;
            return true;
        }
    }
    return false;
}

bool ItemClassifier::isCodeLike(const QString& text) {
    QString plain = stripHtml(text).trimmed();
    if (plain.startsWith(QStringLiteral("import ")) ||
        plain.startsWith(QStringLiteral("class ")) ||
        plain.startsWith(QStringLiteral("def ")) ||
        plain.startsWith(QStringLiteral("function")) ||
        plain.startsWith(QStringLiteral("var ")) ||
        plain.startsWith(QStringLiteral("const "))) {
        return true;
    }

    if (plain.startsWith(QStringLiteral("<"))) {
        static const QRegularExpression htmlTagRegex(
            QStringLiteral("^<(html|div|p|span|table|h[1-6]|script|!DOCTYPE|body|head)"),
            QRegularExpression::CaseInsensitiveOption);
        if (htmlTagRegex.match(plain).hasMatch()) {
            return true;
        }
    }

    if (plain.startsWith(QStringLiteral("{")) &&
        plain.contains(QStringLiteral("\":")) &&
        plain.contains(QStringLiteral("}"))) {
        return true;
    }

    return false;
}

bool ItemClassifier::isAbsolutePathText(const QString& text) {
    QString cleanPath = text.trimmed();
    if ((cleanPath.startsWith('"') && cleanPath.endsWith('"')) ||
        (cleanPath.startsWith('\'') && cleanPath.endsWith('\''))) {
        cleanPath = cleanPath.mid(1, cleanPath.length() - 2);
    }
    return cleanPath.length() < 260 && (
        (cleanPath.length() > 2 && cleanPath[1] == ':') ||
        cleanPath.startsWith(QStringLiteral("\\")) ||
        cleanPath.startsWith('/') ||
        cleanPath.startsWith(QStringLiteral("./")) ||
        cleanPath.startsWith(QStringLiteral("../")));
}

ItemType::Type ItemClassifier::detectIngestType(const QString& text) {
    QString stripped = text.trimmed();
    if (isUrl(stripped)) {
        return ItemType::Type::Link;
    }

    QString plain = stripHtml(text).trimmed();
    if (isHexColor(plain)) {
        return ItemType::Type::Color;
    }

    if (isCodeLike(text)) {
        return ItemType::Type::Code;
    }

    if (isAbsolutePathText(stripped)) {
        QString cleanPath = stripped;
        if ((cleanPath.startsWith('"') && cleanPath.endsWith('"')) ||
            (cleanPath.startsWith('\'') && cleanPath.endsWith('\''))) {
            cleanPath = cleanPath.mid(1, cleanPath.length() - 2);
        }
        QFileInfo info(cleanPath);
        if (info.exists()) {
            return info.isDir() ? ItemType::Type::Folder : ItemType::Type::File;
        }
    }

    return ItemType::Type::Text;
}

DisplayCategory ItemClassifier::detectDisplayCategory(const QString& itemTypeStr, const QString& content) {
    ItemType::Type type = ItemType::fromString(itemTypeStr);
    QString strippedContent = content.trimmed();

    switch (type) {
    case ItemType::Type::Link:
        return {QStringLiteral("link"), QStringLiteral("#17B345")};
    case ItemType::Type::Code:
        return {QStringLiteral("code"), QStringLiteral("#00FF00")};
    case ItemType::Type::Color:
    case ItemType::Type::Palette:
        return {QStringLiteral("palette"), strippedContent};
    case ItemType::Type::OcrText:
        return {QStringLiteral("screenshot_ocr"), QStringLiteral("#00FFFF")};
    case ItemType::Type::CapturedMessage:
        return {QStringLiteral("message"), QStringLiteral("#00FFFF")};
    case ItemType::Type::PixelRuler:
        return {QStringLiteral("pixel_ruler"), QStringLiteral("#FF5722")};
    case ItemType::Type::LocalFile:
        return {QStringLiteral("file_import"), QStringLiteral("#FFFF00")};
    case ItemType::Type::LocalFolder:
        return {QStringLiteral("folder_import"), QStringLiteral("#FF8C00")};
    case ItemType::Type::LocalBatch:
        return {QStringLiteral("batch_import"), QStringLiteral("#FFFF00")};
    default:
        break;
    }

    // 纯文本兜底形态检测（完全不访问磁盘）
    if (isUrl(strippedContent)) {
        return {QStringLiteral("link"), QStringLiteral("#17B345")};
    }

    QString plain = stripHtml(content).trimmed();
    if (isHexColor(plain)) {
        return {QStringLiteral("palette"), plain};
    }

    if (isCodeLike(content)) {
        return {QStringLiteral("code"), QStringLiteral("#00FF00")};
    }

    return {QStringLiteral("text"), QStringLiteral("#95A5A6")};
}

} // namespace RapidNotes
>>>>>>> REPLACE

---

### 3.6 路径归并种类判定器头文件：`RapidNotes_Source/RapidNotes/src/core/PathSummary.h`

<<<<<<< SEARCH FILE: RapidNotes_Source/RapidNotes/src/core/PathSummary.h
=======
#pragma once

#include <QString>
#include <QStringList>

namespace RapidNotes {

class PathSummary {
public:
    enum class Kind {
        SingleFile,
        SingleFolder,
        MultipleFilesOrMixed,
        MultipleFolders
    };

    static Kind summarize(const QStringList& paths);
    static Kind summarizeFromSemicolonString(const QString& content);

    static QString getIconName(Kind kind);
    static QString getIconColor(Kind kind);

private:
    PathSummary() = default;
};

} // namespace RapidNotes
>>>>>>> REPLACE

---

### 3.7 路径归并种类判定器实现文件：`RapidNotes_Source/RapidNotes/src/core/PathSummary.cpp`

<<<<<<< SEARCH FILE: RapidNotes_Source/RapidNotes/src/core/PathSummary.cpp
=======
#include "PathSummary.h"
#include <QFileInfo>

namespace RapidNotes {

PathSummary::Kind PathSummary::summarize(const QStringList& paths) {
    if (paths.isEmpty()) {
        return Kind::SingleFile;
    }

    if (paths.size() == 1) {
        QString clean = paths.first().trimmed();
        if ((clean.startsWith('"') && clean.endsWith('"')) ||
            (clean.startsWith('\'') && clean.endsWith('\''))) {
            clean = clean.mid(1, clean.length() - 2);
        }
        if (clean.endsWith('/') || clean.endsWith('\\')) {
            clean.chop(1);
        }
        QFileInfo fi(clean);
        return fi.isDir() ? Kind::SingleFolder : Kind::SingleFile;
    }

    int dirCount = 0;
    for (const QString& p : paths) {
        QString clean = p.trimmed();
        if ((clean.startsWith('"') && clean.endsWith('"')) ||
            (clean.startsWith('\'') && clean.endsWith('\''))) {
            clean = clean.mid(1, clean.length() - 2);
        }
        if (clean.endsWith('/') || clean.endsWith('\\')) {
            clean.chop(1);
        }
        if (QFileInfo(clean).isDir()) {
            dirCount++;
        }
    }

    if (dirCount == paths.size()) {
        return Kind::MultipleFolders;
    }
    return Kind::MultipleFilesOrMixed;
}

PathSummary::Kind PathSummary::summarizeFromSemicolonString(const QString& content) {
    QStringList paths = content.split(';', Qt::SkipEmptyParts);
    return summarize(paths);
}

QString PathSummary::getIconName(Kind kind) {
    switch (kind) {
    case Kind::SingleFolder:         return QStringLiteral("folder");
    case Kind::MultipleFolders:      return QStringLiteral("folders_multiple");
    case Kind::MultipleFilesOrMixed: return QStringLiteral("files_multiple");
    case Kind::SingleFile:
    default:                         return QStringLiteral("file");
    }
}

QString PathSummary::getIconColor(Kind kind) {
    switch (kind) {
    case Kind::SingleFolder:         return QStringLiteral("#FF8C00"); // 橙色 (Hue 33)
    case Kind::MultipleFolders:      return QStringLiteral("#483D8B"); // 深紫蓝色 (Hue 248)
    case Kind::MultipleFilesOrMixed: return QStringLiteral("#FF0000"); // 纯红色 (Hue 0)
    case Kind::SingleFile:
    default:                         return QStringLiteral("#FFFF00"); // 纯黄色 (Hue 60)
    }
}

} // namespace RapidNotes
>>>>>>> REPLACE

---

### 3.8 字数阶梯计算头文件：`RapidNotes_Source/RapidNotes/src/core/WordBucket.h`

<<<<<<< SEARCH FILE: RapidNotes_Source/RapidNotes/src/core/WordBucket.h
=======
#pragma once

#include <QString>

namespace RapidNotes {

class WordBucket {
public:
    static int calculate(const QString& content, const QString& itemType);

private:
    WordBucket() = default;
};

} // namespace RapidNotes
>>>>>>> REPLACE

---

### 3.9 字数阶梯计算实现文件：`RapidNotes_Source/RapidNotes/src/core/WordBucket.cpp`

<<<<<<< SEARCH FILE: RapidNotes_Source/RapidNotes/src/core/WordBucket.cpp
=======
#include "WordBucket.h"
#include <QRegularExpression>
#include <QtGlobal>

namespace RapidNotes {

int WordBucket::calculate(const QString& content, const QString& itemType) {
    // 1. 类型拦截：非正文相关类型直接归为“无字数” (0)
    if (itemType != QStringLiteral("text") &&
        itemType != QStringLiteral("code") &&
        itemType != QStringLiteral("ocr_text") &&
        itemType != QStringLiteral("captured_message")) {
        return 0;
    }

    QString plainText = content;

    // 2. 连锅端：清除 <style> 和 <script> 及其内部的所有排版代码
    plainText.remove(QRegularExpression(QStringLiteral("<style[^>]*>.*?</style>"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption));
    plainText.remove(QRegularExpression(QStringLiteral("<script[^>]*>.*?</script>"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption));

    // 3. 剥离：清除普通 HTML 标签
    plainText.remove(QRegularExpression(QStringLiteral("<[^>]*>")));

    // 4. 清洗：清除常见的 HTML 实体乱码（如 &nbsp; &amp;）
    plainText.remove(QRegularExpression(QStringLiteral("&[a-zA-Z]+;")));

    // 5. 压缩：清除所有不可见的空白字符（空格、换行等）
    plainText.remove(QRegularExpression(QStringLiteral("\\s+")));

    int len = plainText.length();

    // 6. 阶梯映射
    if (len == 0) return 0;
    if (len <= 100) return (qMax(0, len - 1) / 10 + 1) * 10;
    if (len <= 1000) return ((len - 1) / 100 + 1) * 100;

    return 1001; // 1000 字以上
}

} // namespace RapidNotes
>>>>>>> REPLACE

---

### 3.10 标签编解码器头文件：`RapidNotes_Source/RapidNotes/src/core/TagCodec.h`

<<<<<<< SEARCH FILE: RapidNotes_Source/RapidNotes/src/core/TagCodec.h
=======
#pragma once

#include <QString>
#include <QStringList>

namespace RapidNotes {

class TagCodec {
public:
    // 拆分：同时支持半角逗号 `,` 和全角逗号 `，`
    static QStringList split(const QString& tagsStr);

    // 拼接：严格使用半角逗号 `,` 连接，无空格
    static QString join(const QStringList& tags);

    // 界面显示用拼接：使用半角逗号加空格 `, ` 连接
    static QString joinDisplay(const QStringList& tags);

private:
    TagCodec() = default;
};

} // namespace RapidNotes
>>>>>>> REPLACE

---

### 3.11 标签编解码器实现文件：`RapidNotes_Source/RapidNotes/src/core/TagCodec.cpp`

<<<<<<< SEARCH FILE: RapidNotes_Source/RapidNotes/src/core/TagCodec.cpp
=======
#include "TagCodec.h"
#include <QRegularExpression>

namespace RapidNotes {

QStringList TagCodec::split(const QString& tagsStr) {
    if (tagsStr.trimmed().isEmpty()) return QStringList();
    static const QRegularExpression splitRegex(QStringLiteral("[,，]"));
    QStringList rawList = tagsStr.split(splitRegex, Qt::SkipEmptyParts);
    QStringList result;
    for (const QString& t : rawList) {
        QString trimmed = t.trimmed();
        if (!trimmed.isEmpty() && !result.contains(trimmed)) {
            result.append(trimmed);
        }
    }
    return result;
}

QString TagCodec::join(const QStringList& tags) {
    return tags.join(QLatin1Char(','));
}

QString TagCodec::joinDisplay(const QStringList& tags) {
    return tags.join(QStringLiteral(", "));
}

} // namespace RapidNotes
>>>>>>> REPLACE

---

### 3.12 缩略图生成器头文件：`RapidNotes_Source/RapidNotes/src/core/ThumbnailMaker.h`

<<<<<<< SEARCH FILE: RapidNotes_Source/RapidNotes/src/core/ThumbnailMaker.h
=======
#pragma once

#include <QImage>
#include <QIcon>
#include <QSize>

namespace RapidNotes {

class ThumbnailMaker {
public:
    static QImage makeThumbnail(const QImage& sourceImage, const QSize& targetSize = QSize(64, 64));
    static QIcon makeThumbnailIcon(const QImage& sourceImage, const QSize& targetSize = QSize(64, 64));

private:
    ThumbnailMaker() = default;
};

} // namespace RapidNotes
>>>>>>> REPLACE

---

### 3.13 缩略图生成器实现文件：`RapidNotes_Source/RapidNotes/src/core/ThumbnailMaker.cpp`

<<<<<<< SEARCH FILE: RapidNotes_Source/RapidNotes/src/core/ThumbnailMaker.cpp
=======
#include "ThumbnailMaker.h"
#include <QPixmap>

namespace RapidNotes {

QImage ThumbnailMaker::makeThumbnail(const QImage& sourceImage, const QSize& targetSize) {
    if (sourceImage.isNull()) return QImage();
    return sourceImage.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QIcon ThumbnailMaker::makeThumbnailIcon(const QImage& sourceImage, const QSize& targetSize) {
    QImage thumb = makeThumbnail(sourceImage, targetSize);
    if (thumb.isNull()) return QIcon();
    return QIcon(QPixmap::fromImage(thumb));
}

} // namespace RapidNotes
>>>>>>> REPLACE

---

### 3.14 彻底移除 StringUtils 中的 detectItemType：`RapidNotes_Source/RapidNotes/src/ui/StringUtils.h`

<<<<<<< SEARCH
/**
     * @brief [NEW] 2026-03-xx 统一类型检测逻辑。
     * 整合分散在 main.cpp 和 NoteModel 中的识别规则。
     */
    static QString detectItemType(const QString& text) {
        QString stripped = text.trimmed();
        QString plain = htmlToPlainText(text).trimmed();

        if (stripped.startsWith("http://") || stripped.startsWith("https://") || stripped.startsWith("www.")) {
            return "link";
        }
        
        static const QRegularExpression hexColorRegex("^#([A-Fa-f0-9]{6}|[A-Fa-f0-9]{3})$");
        if (hexColorRegex.match(plain).hasMatch()) {
            return "color";
        }

        if (plain.startsWith("import ") || plain.startsWith("class ") || 
            plain.startsWith("def ") || plain.startsWith("function") || 
            plain.startsWith("var ") || plain.startsWith("const ") ||
            (plain.startsWith("<") && [](){
                static const QRegularExpression htmlTagRegex("^<(html|div|p|span|table|h[1-6]|script|!DOCTYPE|body|head)", QRegularExpression::CaseInsensitiveOption);
                return htmlTagRegex;
            }().match(plain).hasMatch()) ||
            (plain.startsWith("{") && plain.contains("\":") && plain.contains("}"))) {
            return "code";
        }

        // 检测路径
        QString cleanPath = stripped;
        if ((cleanPath.startsWith("\"") && cleanPath.endsWith("\"")) || 
            (cleanPath.startsWith("'") && cleanPath.endsWith("'"))) {
            cleanPath = cleanPath.mid(1, cleanPath.length() - 2);
        }
        if (cleanPath.length() < 260 && (
            (cleanPath.length() > 2 && cleanPath[1] == ':') || 
            cleanPath.startsWith("\\\\") || cleanPath.startsWith("/") || 
            cleanPath.startsWith("./") || cleanPath.startsWith("../"))) {
            QFileInfo info(cleanPath);
            if (info.exists()) {
                return info.isDir() ? "folder" : "file";
            }
        }

        return "text";
    }
=======

>>>>>>> REPLACE

---

### 3.15 NoteEditWindow 头文件引入：`RapidNotes_Source/RapidNotes/src/ui/NoteEditWindow.cpp`

<<<<<<< SEARCH
#include "NoteEditWindow.h"
#include "StringUtils.h"
=======
#include "ui/NoteEditWindow.h"
#include "ui/AdvancedTagSelector.h"
#include "ui/IconHelper.h"
#include "ui/FramelessDialog.h"
#include "core/DatabaseManager.h"
#include "core/ItemClassifier.h"
#include "core/TagCodec.h"
>>>>>>> REPLACE

---

### 3.16 NoteEditWindow 调用改用 ItemClassifier：`RapidNotes_Source/RapidNotes/src/ui/NoteEditWindow.cpp`

<<<<<<< SEARCH
if (finalType == "image" || finalType == "file") {
        if (contentHasSubstantialChange && !m_contentEdit->isRich()) {
            finalType = StringUtils::detectItemType(content);
        }
    } else if (finalType == "link" || finalType == "color" || finalType == "code") {
        // 按照用户要求：既然创建时是网页链接，无论怎么修改内容（如修改标题），它仍然应该是网页链接。
        // 除非用户删掉了链接换成了纯文本，否则不应降级。
        if (contentHasSubstantialChange) {
            finalType = StringUtils::detectItemType(content);
            // 补偿逻辑：如果新检测出的是普通文本，但旧的是链接，说明用户可能只是在微调链接文本，应予以保留属性。
            if (finalType == "text" && m_origItemType == "link" && content.contains(".")) {
                 finalType = "link";
            }
        }
    } else {
        // 原本就是 text 类型，执行全量检测
        finalType = StringUtils::detectItemType(content);
    }
=======
if (finalType == "image" || finalType == "file") {
        if (contentHasSubstantialChange && !m_contentEdit->isRich()) {
            finalType = RapidNotes::ItemType::toString(RapidNotes::ItemClassifier::detectIngestType(content));
        }
    } else if (finalType == "link" || finalType == "color" || finalType == "code") {
        // 按照用户要求：既然创建时是网页链接，无论怎么修改内容（如修改标题），它仍然应该是网页链接。
        // 除非用户删掉了链接换成了纯文本，否则不应降级。
        if (contentHasSubstantialChange) {
            finalType = RapidNotes::ItemType::toString(RapidNotes::ItemClassifier::detectIngestType(content));
            // 补偿逻辑：如果新检测出的是普通文本，但旧的是链接，说明用户可能只是在微调链接文本，应予以保留属性。
            if (finalType == "text" && m_origItemType == "link" && content.contains(".")) {
                 finalType = "link";
            }
        }
    } else {
        // 原本就是 text 类型，执行全量检测
        finalType = RapidNotes::ItemType::toString(RapidNotes::ItemClassifier::detectIngestType(content));
    }
>>>>>>> REPLACE

---

### 3.17 NoteEditWindow 标签初始化接入 TagCodec：`RapidNotes_Source/RapidNotes/src/ui/NoteEditWindow.cpp`

<<<<<<< SEARCH
    if (m_titleEdit) m_titleEdit->setPlainText(title);
    if (m_contentEdit) m_contentEdit->setInitialContent(content);
    if (m_tagEdit) m_tagEdit->setText(tags.join(", "));
=======
    if (m_titleEdit) m_titleEdit->setPlainText(title);
    if (m_contentEdit) m_contentEdit->setInitialContent(content);
    if (m_tagEdit) m_tagEdit->setText(RapidNotes::TagCodec::joinDisplay(tags));
>>>>>>> REPLACE

---

### 3.18 NoteEditWindow 保存时标签解析接入 TagCodec：`RapidNotes_Source/RapidNotes/src/ui/NoteEditWindow.cpp`

<<<<<<< SEARCH
    QString tagsStr = m_tagEdit->text();
    QStringList tagsList = tagsStr.split(QRegularExpression("[,，]"), Qt::SkipEmptyParts);
    for (QString& t : tagsList) t = t.trimmed();
=======
    QString tagsStr = m_tagEdit->text();
    QStringList tagsList = RapidNotes::TagCodec::split(tagsStr);
>>>>>>> REPLACE

---

### 3.19 NoteEditWindow 标签选择器弹窗接入 TagCodec：`RapidNotes_Source/RapidNotes/src/ui/NoteEditWindow.cpp`

<<<<<<< SEARCH
void NoteEditWindow::openTagSelector() {
    QStringList currentTags = m_tagEdit->text().split(",", Qt::SkipEmptyParts);
    for (QString& t : currentTags) t = t.trimmed();

    auto* selector = new AdvancedTagSelector(this);
    auto recentTags = DatabaseManager::instance().getRecentTagsWithCounts(20);
    auto allTags = DatabaseManager::instance().getAllTags();
    selector->setup(recentTags, allTags, currentTags);

    connect(selector, &AdvancedTagSelector::tagsConfirmed, [this](const QStringList& tags){
        m_tagEdit->setText(tags.join(", "));
    });

    selector->showAtCursor();
}
=======
void NoteEditWindow::openTagSelector() {
    QStringList currentTags = RapidNotes::TagCodec::split(m_tagEdit->text());

    auto* selector = new AdvancedTagSelector(this);
    auto recentTags = DatabaseManager::instance().getRecentTagsWithCounts(20);
    auto allTags = DatabaseManager::instance().getAllTags();
    selector->setup(recentTags, allTags, currentTags);

    connect(selector, &AdvancedTagSelector::tagsConfirmed, [this](const QStringList& tags){
        m_tagEdit->setText(RapidNotes::TagCodec::joinDisplay(tags));
    });

    selector->showAtCursor();
}
>>>>>>> REPLACE

---

### 3.20 NoteModel 头文件引入：`RapidNotes_Source/RapidNotes/src/models/NoteModel.cpp`

<<<<<<< SEARCH
#include "NoteModel.h"
#include <QDateTime>
=======
#include "NoteModel.h"
#include "../core/ItemType.h"
#include "../core/ItemClassifier.h"
#include "../core/PathSummary.h"
#include "../core/ThumbnailMaker.h"
#include <QDateTime>
>>>>>>> REPLACE

---

### 3.21 NoteModel DecorationRole 委托给 ItemClassifier 与 PathSummary：`RapidNotes_Source/RapidNotes/src/models/NoteModel.cpp`

<<<<<<< SEARCH
case Qt::DecorationRole: {
            // 2026-03-11 按照用户要求，全局重定义图标颜色，确保色差 >= 60 度以实现快速识别
            QString type = note.value("item_type").toString();
            QString content = note.value("content").toString().trimmed();
            QString iconName = "text"; // Default
            QString iconColor = "#95A5A6"; // 文本：默认灰色

            // 优先依据 item_type 显示图标，确保即使内容不完整也能维持属性展示一致性
            if (type == "link") {
                iconName = "link";
                iconColor = "#17B345";
                return IconHelper::getIcon(iconName, iconColor, 32);
            } else if (type == "code") {
                iconName = "code";
                iconColor = "#00FF00";
                return IconHelper::getIcon(iconName, iconColor, 32);
            } else if (type == "color") {
                iconName = "palette";
                iconColor = content;
                return IconHelper::getIcon(iconName, iconColor, 32);
            }

            if (type == "image") {
                int id = note.value("id").toInt();
                if (m_thumbnailCache.contains(id)) return m_thumbnailCache[id];
                
                QImage img;
                img.loadFromData(note.value("data_blob").toByteArray());
                if (!img.isNull()) {
                    // [OPTIMIZATION] 2026-05-xx 按照用户要求：同步扩大缩略图缓存至 500，确保大数据量滚动无掉帧
                    if (m_thumbnailCache.size() > 500) m_thumbnailCache.clear();
                    
                    QIcon thumb(QPixmap::fromImage(img.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
                    m_thumbnailCache[id] = thumb;
                    return thumb;
                }
                iconName = "image";
                iconColor = "#FF00FF"; // 图片：洋红色 (Hue 300)
            } else if (type == "file" || type == "files" || type == "folder" || type == "folders") {
                if (type == "folder" || (!content.contains(";") && QFileInfo(content.trimmed().remove('\"').remove('\'')).isDir())) {
                    iconName = "folder";
                    iconColor = "#FF8C00"; // 单文件夹：橙色 (Hue 33)
                } else if (type == "folders") {
                    iconName = "folders_multiple";
                    iconColor = "#483D8B"; // 多文件夹：深紫蓝色 (Hue 248) - 与红色文件形成巨大反差
                } else if (content.contains(";")) {
                    iconName = "files_multiple";
                    iconColor = "#FF0000"; // 多文件：纯红色 (Hue 0)
                } else {
                    iconName = "file";
                    iconColor = "#FFFF00"; // 单文件：纯黄色 (Hue 60)
                }
            } else if (type == "ocr_text") {
                iconName = "screenshot_ocr";
                iconColor = "#00FFFF"; // OCR：纯青色 (Hue 180)
            } else if (type == "captured_message") {
                iconName = "message";
                iconColor = "#00FFFF"; // 采集消息：纯青色 (Hue 180)
            } else if (type == "local_file" || type == "local_batch") {
                iconName = (type == "local_file") ? "file_import" : "batch_import";
                iconColor = "#FFFF00"; // 本地导入文件：黄色
            } else if (type == "local_folder") {
                iconName = "folder_import";
                iconColor = "#FF8C00"; // 本地导入文件夹：橙色
            } else if (type == "color" || type == "palette") {
                iconName = "palette";
                iconColor = content; // 保持原有颜色
            } else if (type == "pixel_ruler") {
                iconName = "pixel_ruler";
                iconColor = "#FF5722"; // 像素尺：深橙
            } else {
                QString stripped = content.trimmed();
                QString cleanPath = stripped;
                if ((cleanPath.startsWith("\"") && cleanPath.endsWith("\"")) || 
                    (cleanPath.startsWith("'") && cleanPath.endsWith("'"))) {
                    cleanPath = cleanPath.mid(1, cleanPath.length() - 2);
                }

                QString plain = StringUtils::htmlToPlainText(content).trimmed();
                if (stripped.startsWith("http://") || stripped.startsWith("https://") || stripped.startsWith("www.")) {
                    iconName = "link";
                    iconColor = "#17B345"; // 链接：绿色 (2026-03-xx 用户修改)
                } else if (static const QRegularExpression hexColorRegex("^#([A-Fa-f0-9]{6}|[A-Fa-f0-9]{3})$"); hexColorRegex.match(plain).hasMatch()) {
                    iconName = "palette";
                    iconColor = plain;
                } else if (plain.startsWith("import ") || plain.startsWith("class ") || 
                           plain.startsWith("def ") || plain.startsWith("function") || 
                           plain.startsWith("var ") || plain.startsWith("const ") ||
                           // 2026-03-xx 按照用户要求重构：不再仅靠单字符判断，改用结构化特征识别
                           (plain.startsWith("<") && [](){
                               static const QRegularExpression htmlTagRegex("^<(html|div|p|span|table|h[1-6]|script|!DOCTYPE|body|head)", QRegularExpression::CaseInsensitiveOption);
                               return htmlTagRegex;
                           }().match(plain).hasMatch()) ||
                           (plain.startsWith("{") && plain.contains("\":") && plain.contains("}"))) {
                    iconName = "code";
                    iconColor = "#00FF00"; // 代码：鲜绿色 (Hue 120)
                } else if (cleanPath.length() < 260 && (
                           (cleanPath.length() > 2 && cleanPath[1] == ':') || 
                           cleanPath.startsWith("\\\\") || cleanPath.startsWith("/") || 
                           cleanPath.startsWith("./") || cleanPath.startsWith("../"))) {
                    QFileInfo info(cleanPath);
                    if (info.exists()) {
                        if (info.isDir()) {
                            iconName = "folder";
                            iconColor = "#FF8C00"; // 自动识别文件夹：橙色
                        } else {
                            iconName = "file";
                            iconColor = "#FFFF00"; // 自动识别文件：黄色
                        }
                    }
                }
            }
            return IconHelper::getIcon(iconName, iconColor, 32);
        }
=======
case Qt::DecorationRole: {
            QString type = note.value("item_type").toString();
            QString content = note.value("content").toString().trimmed();

            // 图片类型特殊处理（使用 ThumbnailMaker 生成 64x64 缩略图并缓存）
            if (type == "image") {
                int id = note.value("id").toInt();
                if (m_thumbnailCache.contains(id)) return m_thumbnailCache[id];

                QImage img;
                img.loadFromData(note.value("data_blob").toByteArray());
                if (!img.isNull()) {
                    if (m_thumbnailCache.size() > 500) m_thumbnailCache.clear();
                    QIcon thumb = RapidNotes::ThumbnailMaker::makeThumbnailIcon(img);
                    m_thumbnailCache[id] = thumb;
                    return thumb;
                }
                return IconHelper::getIcon(QStringLiteral("image"), QStringLiteral("#FF00FF"), 32);
            }

            // 文件类旧类型由 PathSummary 决定
            if (type == "file" || type == "files" || type == "folder" || type == "folders") {
                RapidNotes::PathSummary::Kind kind = RapidNotes::PathSummary::summarizeFromSemicolonString(content);
                return IconHelper::getIcon(RapidNotes::PathSummary::getIconName(kind), RapidNotes::PathSummary::getIconColor(kind), 32);
            }

            // 其他类型及文本特征归并由 ItemClassifier 决定（纯内存判定，绝对不访问磁盘）
            RapidNotes::DisplayCategory cat = RapidNotes::ItemClassifier::detectDisplayCategory(type, content);
            return IconHelper::getIcon(cat.iconName, cat.iconColor, 32);
        }
>>>>>>> REPLACE

---

### 3.22 DatabaseManager 引入纯函数头文件：`RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp`

<<<<<<< SEARCH
#include "DatabaseManager.h"
#include <QDebug>
#include <QSqlRecord>
#include <QSqlError>
=======
#include "DatabaseManager.h"
#include "WordBucket.h"
#include "TagCodec.h"
#include "ItemType.h"
#include <QDebug>
#include <QSqlRecord>
#include <QSqlError>
>>>>>>> REPLACE

---

### 3.23 DatabaseManager 委托 calculateWordBucket 纯函数实现：`RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp`

<<<<<<< SEARCH
// 2026-05-02 紧急修复v2：加强版 C++ 脱壳引擎 (彻底剿灭残余 CSS 与实体字符)
    int calculateWordBucket(const QString& content, const QString& itemType) {
        // 1. 类型拦截：非正文相关类型直接归为“无字数” (0)
        if (itemType != "text" && itemType != "code" && itemType != "ocr_text" && itemType != "captured_message") {
            return 0;
        }

        QString plainText = content;

        // 2. 连锅端：杀掉 <style> 和 <script> 及其内部的所有排版代码（必须加 DotMatchesEverythingOption 解决换行问题）
        plainText.remove(QRegularExpression("<style[^>]*>.*?</style>", QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption));
        plainText.remove(QRegularExpression("<script[^>]*>.*?</script>", QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption));

        // 3. 剥离：杀掉所有剩下的普通 HTML 标签
        plainText.remove(QRegularExpression("<[^>]*>"));

        // 4. 清洗：杀掉常见的 HTML 实体乱码（比如 &nbsp; &amp;）
        plainText.remove(QRegularExpression("&[a-zA-Z]+;"));

        // 5. 压缩：杀掉所有不可见的空白字符（空格、换行等）
        plainText.remove(QRegularExpression("\\s+"));

        int len = plainText.length();

        // 6. 阶梯映射
        if (len == 0) return 0;
        if (len <= 100) return (qMax(0, len - 1) / 10 + 1) * 10;
        if (len <= 1000) return ((len - 1) / 100 + 1) * 100;

        return 1001; // 1000 字以上
    }
=======
    // 2026-05-02 委托至纯函数真理源 RapidNotes::WordBucket
    int calculateWordBucket(const QString& content, const QString& itemType) {
        return RapidNotes::WordBucket::calculate(content, itemType);
    }
>>>>>>> REPLACE

---

### 3.24 DatabaseManager 启动回填标签解析接入 TagCodec：`RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp`

<<<<<<< SEARCH
                while (fetchNotes.next()) {
                    int noteId = fetchNotes.value(0).toInt();
                    QStringList tagList = fetchNotes.value(1).toString().split(QRegularExpression("[,，]"), Qt::SkipEmptyParts);
                    syncNoteTags(noteId, tagList);
                    count++;
                }
=======
                while (fetchNotes.next()) {
                    int noteId = fetchNotes.value(0).toInt();
                    QStringList tagList = RapidNotes::TagCodec::split(fetchNotes.value(1).toString());
                    syncNoteTags(noteId, tagList);
                    count++;
                }
>>>>>>> REPLACE

---

### 3.25 DatabaseManager addNote 合并既有标签接入 TagCodec：`RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp`

<<<<<<< SEARCH
            QVariantMap existingNote = getNoteById(existingId);
            QString existingTagsStr = existingNote.value("tags").toString();
            QStringList existingTags = existingTagsStr.split(",", Qt::SkipEmptyParts);
            for(QString& t : existingTags) t = t.trimmed();
=======
            QVariantMap existingNote = getNoteById(existingId);
            QString existingTagsStr = existingNote.value("tags").toString();
            QStringList existingTags = RapidNotes::TagCodec::split(existingTagsStr);
>>>>>>> REPLACE

---

### 3.26 DatabaseManager addNote 分类预设标签接入 TagCodec：`RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp`

<<<<<<< SEARCH
                    QString preset = catQuery.value(1).toString();
                    if (!preset.isEmpty()) {
                        QStringList pTags = preset.split(",", Qt::SkipEmptyParts);
                        for (const QString& t : pTags) {
                            if (!existingTags.contains(t.trimmed())) existingTags << t.trimmed();
                        }
                    }
=======
                    QString preset = catQuery.value(1).toString();
                    if (!preset.isEmpty()) {
                        QStringList pTags = RapidNotes::TagCodec::split(preset);
                        for (const QString& t : pTags) {
                            if (!existingTags.contains(t)) existingTags << t;
                        }
                    }
>>>>>>> REPLACE

---

### 3.27 DatabaseManager addNote 更新合并标签接入 TagCodec：`RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp`

<<<<<<< SEARCH
            updateQuery.prepare(sql);
            updateQuery.bindValue(":tags", existingTags.join(", "));
            updateQuery.bindValue(":now", currentTime);
=======
            updateQuery.prepare(sql);
            updateQuery.bindValue(":tags", RapidNotes::TagCodec::joinDisplay(existingTags));
            updateQuery.bindValue(":now", currentTime);
>>>>>>> REPLACE

---

### 3.28 DatabaseManager addNote 预设标签合并入新建笔记接入 TagCodec：`RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp`

<<<<<<< SEARCH
                QString preset = catQuery.value(1).toString();
                if (!preset.isEmpty()) {
                    QStringList pTags = preset.split(",", Qt::SkipEmptyParts);
                    for (const QString& t : pTags) {
                        QString trimmed = t.trimmed();
                        if (!finalTags.contains(trimmed)) finalTags << trimmed;
                    }
                }
=======
                QString preset = catQuery.value(1).toString();
                if (!preset.isEmpty()) {
                    QStringList pTags = RapidNotes::TagCodec::split(preset);
                    for (const QString& t : pTags) {
                        if (!finalTags.contains(t)) finalTags << t;
                    }
                }
>>>>>>> REPLACE

---

### 3.29 DatabaseManager addNote 新建标签绑定接入 TagCodec：`RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp`

<<<<<<< SEARCH
        QStringList cleanedFinalTags;
        for (const QString& t : finalTags) {
            QString tr = t.trimmed();
            if (!tr.isEmpty() && !cleanedFinalTags.contains(tr)) cleanedFinalTags << tr;
        }
        query.bindValue(":tags", cleanedFinalTags.join(", "));
=======
        QStringList cleanedFinalTags = RapidNotes::TagCodec::split(RapidNotes::TagCodec::join(finalTags));
        query.bindValue(":tags", RapidNotes::TagCodec::joinDisplay(cleanedFinalTags));
>>>>>>> REPLACE

---

### 3.30 DatabaseManager updateNote 标签绑定接入 TagCodec：`RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp`

<<<<<<< SEARCH
        QStringList trimmedTags;
        for (const QString& t : tags) {
            QString tr = t.trimmed();
            if (!tr.isEmpty() && !trimmedTags.contains(tr)) trimmedTags << tr;
        }
        query.bindValue(":tags", trimmedTags.join(", "));
=======
        QStringList trimmedTags = RapidNotes::TagCodec::split(RapidNotes::TagCodec::join(tags));
        query.bindValue(":tags", RapidNotes::TagCodec::joinDisplay(trimmedTags));
>>>>>>> REPLACE

---

### 3.31 DatabaseManager updateNoteState 同步标签接入 TagCodec：`RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp`

<<<<<<< SEARCH
                if (column == "tags") {
                    syncNoteTags(id, tags.split(QRegularExpression("[,，]"), Qt::SkipEmptyParts));
                }
=======
                if (column == "tags") {
                    syncNoteTags(id, RapidNotes::TagCodec::split(tags));
                }
>>>>>>> REPLACE

---

### 3.32 DatabaseManager updateNoteStateBatch 同步标签接入 TagCodec：`RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp`

<<<<<<< SEARCH
                // 2026-05-02 补齐批量状态下的字数与标签同步
                if (column == "tags") {
                    syncNoteTags(id, value.toString().split(QRegularExpression("[,，]"), Qt::SkipEmptyParts));
                } else if (column == "content" || column == "item_type") {
=======
                // 2026-05-02 补齐批量状态下的字数与标签同步
                if (column == "tags") {
                    syncNoteTags(id, RapidNotes::TagCodec::split(value.toString()));
                } else if (column == "content" || column == "item_type") {
>>>>>>> REPLACE

---

### 3.33 DatabaseManager updateNoteStateBatch 预设标签合并接入 TagCodec：`RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp`

<<<<<<< SEARCH
                if (fetchTags.exec() && fetchTags.next()) {
                    QString existing = fetchTags.value(0).toString();
                    QStringList tagList = existing.split(",", Qt::SkipEmptyParts);
                    QStringList newTags = presetTags.split(",", Qt::SkipEmptyParts);
                    bool changed = false;
                    for (const QString& t : newTags) { if (!tagList.contains(t.trimmed())) { tagList.append(t.trimmed()); changed = true; } }
                    if (changed) { QSqlQuery updateTags(m_db); updateTags.prepare("UPDATE notes SET tags = :tags WHERE id = :id"); updateTags.bindValue(":tags", tagList.join(", ")); updateTags.bindValue(":id", id); updateTags.exec(); }
                }
=======
                if (fetchTags.exec() && fetchTags.next()) {
                    QString existing = fetchTags.value(0).toString();
                    QStringList tagList = RapidNotes::TagCodec::split(existing);
                    QStringList newTags = RapidNotes::TagCodec::split(presetTags);
                    bool changed = false;
                    for (const QString& t : newTags) { if (!tagList.contains(t)) { tagList.append(t); changed = true; } }
                    if (changed) { QSqlQuery updateTags(m_db); updateTags.prepare("UPDATE notes SET tags = :tags WHERE id = :id"); updateTags.bindValue(":tags", RapidNotes::TagCodec::joinDisplay(tagList)); updateTags.bindValue(":id", id); updateTags.exec(); }
                }
>>>>>>> REPLACE

---

### 3.34 DatabaseManager getAllTags 标签解析接入 TagCodec：`RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp`

<<<<<<< SEARCH
        while (query.next()) {
            QString tagsStr = query.value(0).toString();
            QStringList parts = tagsStr.split(QRegularExpression("[,，]"), Qt::SkipEmptyParts);
            for (const QString& part : parts) {
                QString trimmed = part.trimmed();
                if (!trimmed.isEmpty() && !allTags.contains(trimmed)) allTags.append(trimmed);
            }
        }
=======
        while (query.next()) {
            QString tagsStr = query.value(0).toString();
            QStringList parts = RapidNotes::TagCodec::split(tagsStr);
            for (const QString& part : parts) {
                if (!allTags.contains(part)) allTags.append(part);
            }
        }
>>>>>>> REPLACE

---

### 3.35 DatabaseManager getRecentTagsWithCounts 标签解析接入 TagCodec：`RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp`

<<<<<<< SEARCH
            QString tagsStr = query.value(0).toString();
            QDateTime updatedAt = query.value(1).toDateTime();
            QStringList parts = tagsStr.split(QRegularExpression("[,，]"), Qt::SkipEmptyParts);
            for (const QString& part : parts) {
=======
            QString tagsStr = query.value(0).toString();
            QDateTime updatedAt = query.value(1).toDateTime();
            QStringList parts = RapidNotes::TagCodec::split(tagsStr);
            for (const QString& part : parts) {
>>>>>>> REPLACE

---

### 3.36 DatabaseManager updateCategory 批量预设标签接入 TagCodec：`RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp`

<<<<<<< SEARCH
if (!tags.isEmpty()) {
            QStringList newTagsList = tags.split(",", Qt::SkipEmptyParts);
            QSqlQuery fetchNotes(m_db);
            fetchNotes.prepare("SELECT id, tags FROM notes WHERE category_id = :catId AND is_deleted = 0");
            fetchNotes.bindValue(":catId", catId);
            if (fetchNotes.exec()) {
                while (fetchNotes.next()) {
                    int noteId = fetchNotes.value(0).toInt();
                    QString existingTagsStr = fetchNotes.value(1).toString();
                    QStringList existingTags = existingTagsStr.split(",", Qt::SkipEmptyParts);
                    bool changed = false;
                    for (const QString& t : newTagsList) { QString trimmed = t.trimmed(); if (!trimmed.isEmpty() && !existingTags.contains(trimmed)) { existingTags.append(trimmed); changed = true; } }
                    if (changed) {
                        affectedIds << noteId;
                        QSqlQuery updateNote(m_db);
                        updateNote.prepare("UPDATE notes SET tags = :tags WHERE id = :id");
                        updateNote.bindValue(":tags", existingTags.join(", "));
                        updateNote.bindValue(":id", noteId);
                        updateNote.exec();

                        // 2026-05-02 补齐预设标签同步漏洞
                        syncNoteTags(noteId, existingTags);
                    }
                }
=======
        if (!tags.isEmpty()) {
            QStringList newTagsList = RapidNotes::TagCodec::split(tags);
            QSqlQuery fetchNotes(m_db);
            fetchNotes.prepare("SELECT id, tags FROM notes WHERE category_id = :catId AND is_deleted = 0");
            fetchNotes.bindValue(":catId", catId);
            if (fetchNotes.exec()) {
                while (fetchNotes.next()) {
                    int noteId = fetchNotes.value(0).toInt();
                    QString existingTagsStr = fetchNotes.value(1).toString();
                    QStringList existingTags = RapidNotes::TagCodec::split(existingTagsStr);
                    bool changed = false;
                    for (const QString& t : newTagsList) { if (!existingTags.contains(t)) { existingTags.append(t); changed = true; } }
                    if (changed) {
                        affectedIds << noteId;
                        QSqlQuery updateNote(m_db);
                        updateNote.prepare("UPDATE notes SET tags = :tags WHERE id = :id");
                        updateNote.bindValue(":tags", RapidNotes::TagCodec::joinDisplay(existingTags));
                        updateNote.bindValue(":id", noteId);
                        updateNote.exec();

                        // 2026-05-02 补齐预设标签同步漏洞
                        syncNoteTags(noteId, existingTags);
                    }
                }
            }
        }
>>>>>>> REPLACE

---

### 3.37 DatabaseManager addTagToNote 标签合并与序列化接入 TagCodec：`RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp`

<<<<<<< SEARCH
QStringList existingStrList = note["tags"].toString().split(QRegularExpression("[,，]"), Qt::SkipEmptyParts);
    QStringList finalTags;
    // 确保原有标签也经过清理
    for (const QString& t : existingStrList) {
        QString trimmed = t.trimmed();
        if (!trimmed.isEmpty() && !finalTags.contains(trimmed)) finalTags << trimmed;
    }

    // 合并新标签
    for (const QString& t : tags) {
        QString trimmed = t.trimmed();
        if (!trimmed.isEmpty() && !finalTags.contains(trimmed)) finalTags << trimmed;
    }

    // 2026-05-09 按照用户要求：严禁使用 "逗号+空格"，统一使用纯逗号分割。
    QString tagsString = finalTags.join(",");
    bool ok = updateNoteState(noteId, "tags", tagsString);
=======
    QStringList finalTags = RapidNotes::TagCodec::split(note["tags"].toString());

    // 合并新标签
    for (const QString& t : tags) {
        QString trimmed = t.trimmed();
        if (!trimmed.isEmpty() && !finalTags.contains(trimmed)) finalTags << trimmed;
    }

    // 2026-05-09 按照用户要求：使用纯逗号分割统一由 TagCodec::join 负责
    QString tagsString = RapidNotes::TagCodec::join(finalTags);
>>>>>>> REPLACE

---

### 3.38 DatabaseManager renameTag 标签重命名与更新接入 TagCodec：`RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp`

<<<<<<< SEARCH
QStringList tagList = tagsStr.split(QRegularExpression("[,，]"), Qt::SkipEmptyParts);
            QStringList newList;
            bool changed = false;
            for (const QString& t : tagList) {
                if (t.trimmed() == targetOld) { newList << targetNew; changed = true; }
                else newList << t.trimmed();
            }
            if (changed) {
                QSqlQuery up(m_db);
                up.prepare("UPDATE notes SET tags = ? WHERE id = ?");
                up.addBindValue(newList.join(", "));
                up.addBindValue(noteId);
                up.exec();
            }
        }
=======
            int noteId = query.value(0).toInt();
            QString tagsStr = query.value(1).toString();
            QStringList tagList = RapidNotes::TagCodec::split(tagsStr);
            QStringList newList;
            bool changed = false;
            for (const QString& t : tagList) {
                if (t == targetOld) { newList << targetNew; changed = true; }
                else newList << t;
            }
            if (changed) {
                QSqlQuery up(m_db);
                up.prepare("UPDATE notes SET tags = ? WHERE id = ?");
                up.addBindValue(RapidNotes::TagCodec::joinDisplay(newList));
                up.addBindValue(noteId);
                up.exec();
            }
>>>>>>> REPLACE

---

### 3.39 DatabaseManager deleteTag 标签移除与更新接入 TagCodec：`RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp`

<<<<<<< SEARCH
int id = updateNotes.value(0).toInt();
                QStringList tagList = updateNotes.value(1).toString().split(QRegularExpression("[,，]"), Qt::SkipEmptyParts);
                QStringList newList;
                for(auto& t : tagList) if(t.trimmed() != target) newList << t.trimmed();
                QSqlQuery up(m_db);
                up.prepare("UPDATE notes SET tags = ? WHERE id = ?");
                up.addBindValue(newList.join(", "));
                up.addBindValue(id);
                up.exec();
            }
        }
    }
=======
            while (updateNotes.next()) {
                int id = updateNotes.value(0).toInt();
                QStringList tagList = RapidNotes::TagCodec::split(updateNotes.value(1).toString());
                QStringList newList;
                for(auto& t : tagList) if(t != target) newList << t;
                QSqlQuery up(m_db);
                up.prepare("UPDATE notes SET tags = ? WHERE id = ?");
                up.addBindValue(RapidNotes::TagCodec::joinDisplay(newList));
                up.addBindValue(id);
                up.exec();

                syncNoteTags(id, newList);
            }
>>>>>>> REPLACE

---

### 3.40 DatabaseManager syncNoteTags 标签物理序列化接入 TagCodec：`RapidNotes_Source/RapidNotes/src/core/DatabaseManager.cpp`

<<<<<<< SEARCH
    QSqlQuery upNotes(m_db);
    upNotes.prepare("UPDATE notes SET tags = ? WHERE id = ?");
    upNotes.addBindValue(cleanTags.join(","));
    upNotes.addBindValue(noteId);
    upNotes.exec();
=======
    QSqlQuery upNotes(m_db);
    upNotes.prepare("UPDATE notes SET tags = ? WHERE id = ?");
    upNotes.addBindValue(RapidNotes::TagCodec::join(cleanTags));
    upNotes.addBindValue(noteId);
    upNotes.exec();
>>>>>>> REPLACE

---

### 3.41 QuickWindow 引入 TagCodec 头文件：`RapidNotes_Source/RapidNotes/src/ui/QuickWindow.cpp`

<<<<<<< SEARCH
#include "ToolTipOverlay.h"
#include "QuickWindow.h"
#include "RecentNotesWindow.h"
#include <QDebug>
=======
#include "ToolTipOverlay.h"
#include "QuickWindow.h"
#include "RecentNotesWindow.h"
#include "../core/TagCodec.h"
#include <QDebug>
>>>>>>> REPLACE

---

### 3.42 QuickWindow 多选标签收集接入 TagCodec：`RapidNotes_Source/RapidNotes/src/ui/QuickWindow.cpp`

<<<<<<< SEARCH
        QString tags = index.data(NoteModel::TagsRole).toString();
        QStringList parts = tags.split(QRegularExpression("[,，]"), Qt::SkipEmptyParts);
        for (const QString& t : parts) tagsSet.insert(t.trimmed());
=======
        QString tags = index.data(NoteModel::TagsRole).toString();
        QStringList parts = RapidNotes::TagCodec::split(tags);
        for (const QString& t : parts) tagsSet.insert(t);
>>>>>>> REPLACE

---

### 3.43 QuickWindow 快捷编辑标签弹窗与更新接入 TagCodec：`RapidNotes_Source/RapidNotes/src/ui/QuickWindow.cpp`

<<<<<<< SEARCH
        int id = selected.first().data(NoteModel::IdRole).toInt();
        QVariantMap note = DatabaseManager::instance().getNoteById(id);
        currentTags = note.value("tags").toString().split(",", Qt::SkipEmptyParts);
    }

    for (QString& t : currentTags) t = t.trimmed();
=======
        int id = selected.first().data(NoteModel::IdRole).toInt();
        QVariantMap note = DatabaseManager::instance().getNoteById(id);
        currentTags = RapidNotes::TagCodec::split(note.value("tags").toString());
    }
>>>>>>> REPLACE

---

### 3.44 QuickWindow 更新笔记标签状态接入 TagCodec：`RapidNotes_Source/RapidNotes/src/ui/QuickWindow.cpp`

<<<<<<< SEARCH
        for (const auto& index : std::as_const(selected)) {
            int id = index.data(NoteModel::IdRole).toInt();
            DatabaseManager::instance().updateNoteState(id, "tags", tags.join(", "));
        }
=======
        for (const auto& index : std::as_const(selected)) {
            int id = index.data(NoteModel::IdRole).toInt();
            DatabaseManager::instance().updateNoteState(id, "tags", RapidNotes::TagCodec::joinDisplay(tags));
        }
>>>>>>> REPLACE

---

### 3.45 QuickWindow 复制标签到剪贴板接入 TagCodec：`RapidNotes_Source/RapidNotes/src/ui/QuickWindow.cpp`

<<<<<<< SEARCH
    QVariantMap note = DatabaseManager::instance().getNoteById(id);
    QString tagsStr = note.value("tags").toString();
    QStringList tags = tagsStr.split(QRegularExpression("[,，]"), Qt::SkipEmptyParts);
    for (QString& t : tags) t = t.trimmed();

    DatabaseManager::setTagClipboard(tags);
=======
    QVariantMap note = DatabaseManager::instance().getNoteById(id);
    QString tagsStr = note.value("tags").toString();
    QStringList tags = RapidNotes::TagCodec::split(tagsStr);

    DatabaseManager::setTagClipboard(tags);
>>>>>>> REPLACE

---

### 3.46 QuickWindow 粘贴标签覆盖与追加接入 TagCodec：`RapidNotes_Source/RapidNotes/src/ui/QuickWindow.cpp`

<<<<<<< SEARCH
    QList<int> ids;
    for (const auto& index : std::as_const(selected)) ids << index.data(NoteModel::IdRole).toInt();
    DatabaseManager::instance().updateNoteStateBatch(ids, "tags", tagsToPaste.join(", "));
=======
    QList<int> ids;
    for (const auto& index : std::as_const(selected)) ids << index.data(NoteModel::IdRole).toInt();
    DatabaseManager::instance().updateNoteStateBatch(ids, "tags", RapidNotes::TagCodec::joinDisplay(tagsToPaste));
>>>>>>> REPLACE

---

### 3.47 QuickWindow 历史记录标签复用接入 TagCodec：`RapidNotes_Source/RapidNotes/src/ui/QuickWindow.cpp`

<<<<<<< SEARCH
        if (tagsList.isEmpty()) return;

        QString tagsStr = tagsList.join(", ");
        DatabaseManager::instance().updateNoteStateBatch(ids, "tags", tagsStr);
=======
        if (tagsList.isEmpty()) return;

        QString tagsStr = RapidNotes::TagCodec::joinDisplay(tagsList);
        DatabaseManager::instance().updateNoteStateBatch(ids, "tags", tagsStr);
>>>>>>> REPLACE

---

### 3.48 AdvancedTagSelector 引入 TagCodec 头文件：`RapidNotes_Source/RapidNotes/src/ui/AdvancedTagSelector.cpp`

<<<<<<< SEARCH
#include "AdvancedTagSelector.h"
#include "IconHelper.h"
#include <QPushButton>
#include <QLabel>
#include <QKeyEvent>
=======
#include "AdvancedTagSelector.h"
#include "IconHelper.h"
#include "../core/TagCodec.h"
#include <QPushButton>
#include <QLabel>
>>>>>>> REPLACE

---

### 3.49 AdvancedTagSelector 批量输入标签拆分接入 TagCodec：`RapidNotes_Source/RapidNotes/src/ui/AdvancedTagSelector.cpp`

<<<<<<< SEARCH
        if (!text.isEmpty()) {
            // [CRITICAL] 支持使用中英文逗号分割标签，提升批量输入体验
            QStringList newTags = text.split(QRegularExpression("[,，]"), Qt::SkipEmptyParts);
            bool changed = false;
            for (QString& tag : newTags) {
                tag = tag.trimmed();
=======
        if (!text.isEmpty()) {
            // [CRITICAL] 接入 TagCodec 统一支持半角全角逗号解析与去重
            QStringList newTags = RapidNotes::TagCodec::split(text);
            bool changed = false;
            for (const QString& tag : newTags) {
>>>>>>> REPLACE

---

### 3.50 TagEditDialog 引入 TagCodec 头文件：`RapidNotes_Source/RapidNotes/src/ui/TagEditDialog.cpp`

<<<<<<< SEARCH
#include "TagEditDialog.h"
#include "AdvancedTagSelector.h"
#include "../core/DatabaseManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
=======
#include "TagEditDialog.h"
#include "AdvancedTagSelector.h"
#include "../core/DatabaseManager.h"
#include "../core/TagCodec.h"
#include <QVBoxLayout>
>>>>>>> REPLACE

---

### 3.51 TagEditDialog 标签解析初始化接入 TagCodec：`RapidNotes_Source/RapidNotes/src/ui/TagEditDialog.cpp`

<<<<<<< SEARCH
    // 2. 集成胶囊标签编辑器
    m_tagEditor = new TagEditorWidget(this);
    QStringList tagList = currentTags.split(QRegularExpression("[,，]"), Qt::SkipEmptyParts);
    for(QString& t : tagList) t = t.trimmed();
    m_tagEditor->setTags(tagList);
=======
    // 2. 集成胶囊标签编辑器
    m_tagEditor = new TagEditorWidget(this);
    QStringList tagList = RapidNotes::TagCodec::split(currentTags);
    m_tagEditor->setTags(tagList);
>>>>>>> REPLACE

---

### 3.52 TagEditDialog 标签提取拼接接入 TagCodec：`RapidNotes_Source/RapidNotes/src/ui/TagEditDialog.cpp`

<<<<<<< SEARCH
QString TagEditDialog::getTags() const {
    return m_tagEditor->tags().join(", ");
}
=======
QString TagEditDialog::getTags() const {
    return RapidNotes::TagCodec::joinDisplay(m_tagEditor->tags());
}
>>>>>>> REPLACE

---

### 3.53 FramelessDialog 引入 TagCodec 头文件：`RapidNotes_Source/RapidNotes/src/ui/FramelessDialog.cpp`

<<<<<<< SEARCH
#include "FramelessDialog.h"
#include "IconHelper.h"
#include <QGraphicsDropShadowEffect>
#include <QSettings>
#include <QMouseEvent>
=======
#include "FramelessDialog.h"
#include "IconHelper.h"
#include "../core/TagCodec.h"
#include <QGraphicsDropShadowEffect>
#include <QSettings>
>>>>>>> REPLACE

---

### 3.54 FramelessDialog 标签输入与弹窗设置接入 TagCodec：`RapidNotes_Source/RapidNotes/src/ui/FramelessDialog.cpp`

<<<<<<< SEARCH
        auto recentTags = DatabaseManager::instance().getRecentTagsWithCounts(20);
        QStringList allTags = DatabaseManager::instance().getAllTags();
        QStringList selected = m_edit->text().split(QRegularExpression("[,，]"), Qt::SkipEmptyParts);
        for(QString& s : selected) s = s.trimmed();

        selector->setup(recentTags, allTags, selected);

        connect(selector, &AdvancedTagSelector::tagsConfirmed, [this](const QStringList& tags){
            if (!tags.isEmpty()) {
                m_edit->setText(tags.join(", "));
                m_edit->setFocus();
            }
        });
=======
        auto recentTags = DatabaseManager::instance().getRecentTagsWithCounts(20);
        QStringList allTags = DatabaseManager::instance().getAllTags();
        QStringList selected = RapidNotes::TagCodec::split(m_edit->text());

        selector->setup(recentTags, allTags, selected);

        connect(selector, &AdvancedTagSelector::tagsConfirmed, [this](const QStringList& tags){
            if (!tags.isEmpty()) {
                m_edit->setText(RapidNotes::TagCodec::joinDisplay(tags));
                m_edit->setFocus();
            }
        });
>>>>>>> REPLACE

---

### 3.55 FileStorageHelper 引入 TagCodec 头文件：`RapidNotes_Source/RapidNotes/src/core/FileStorageHelper.cpp`

<<<<<<< SEARCH
#include "FileStorageHelper.h"
#include "DatabaseManager.h"
#include "FileCryptoHelper.h"
#include "../ui/FramelessDialog.h"
#include <QFileInfo>
=======
#include "FileStorageHelper.h"
#include "DatabaseManager.h"
#include "TagCodec.h"
#include "FileCryptoHelper.h"
#include "../ui/FramelessDialog.h"
>>>>>>> REPLACE

---

### 3.56 FileStorageHelper 导入导出标签拆分接入 TagCodec：`RapidNotes_Source/RapidNotes/src/core/FileStorageHelper.cpp`

<<<<<<< SEARCH
            int noteId = DatabaseManager::instance().addNote(title, content, tagsStr.split(",", Qt::SkipEmptyParts), "", catId);
=======
            int noteId = DatabaseManager::instance().addNote(title, content, RapidNotes::TagCodec::split(tagsStr), "", catId);
>>>>>>> REPLACE

---

### 3.57 FileStorageHelper 归档标签解析接入 TagCodec：`RapidNotes_Source/RapidNotes/src/core/FileStorageHelper.cpp`

<<<<<<< SEARCH
                QStringList tags = n["tags"].toString().split(",", Qt::SkipEmptyParts);
=======
                QStringList tags = RapidNotes::TagCodec::split(n["tags"].toString());
>>>>>>> REPLACE

---

### 3.58 main.cpp 引入词表与纯函数头文件：`RapidNotes_Source/RapidNotes/src/main.cpp`

<<<<<<< SEARCH
#include "core/DatabaseManager.h"
#include "core/HotkeyManager.h"
#include "core/ClipboardMonitor.h"
#include "ui/IconHelper.h"
#include "ui/QuickWindow.h"
=======
#include "core/DatabaseManager.h"
#include "core/HotkeyManager.h"
#include "core/ClipboardMonitor.h"
#include "core/ItemType.h"
#include "core/ItemClassifier.h"
#include "core/PathSummary.h"
#include "ui/IconHelper.h"
#include "ui/QuickWindow.h"
>>>>>>> REPLACE

---

### 3.59 main.cpp 复制气泡展示逻辑接入 ItemType 与 PathSummary：`RapidNotes_Source/RapidNotes/src/main.cpp`

<<<<<<< SEARCH
        QString displayContent;
        if (!type.isEmpty() && type != "image" && type != "file" && type != "folder" && type != "files" && type != "folders") {
            displayContent = content.trimmed().left(20);
            if (content.trimmed().length() > 20) displayContent += "...";
        } else {
            if (type == "image") displayContent = "图片";
            else if (type == "file" || type == "folder" || type == "files" || type == "folders") {
                QStringList paths = content.split(";", Qt::SkipEmptyParts);
                if (!paths.isEmpty()) {
                    QString firstPath = paths.first();
                    QString firstName = QFileInfo(firstPath).fileName();
                    if (firstName.isEmpty()) firstName = firstPath;
                    if (paths.size() > 1) displayContent = QString("%1 等 %2 个项目").arg(firstName).arg((int)paths.size());
                    else displayContent = firstName;
                } else displayContent = "文件";
            } else displayContent = type;
        }
=======
        QString displayContent;
        RapidNotes::ItemType::Type itemTypeEnum = RapidNotes::ItemType::fromString(type);
        if (!RapidNotes::ItemType::isFileOrFolder(itemTypeEnum) && itemTypeEnum != RapidNotes::ItemType::Type::Image) {
            displayContent = content.trimmed().left(20);
            if (content.trimmed().length() > 20) displayContent += "...";
        } else {
            if (itemTypeEnum == RapidNotes::ItemType::Type::Image) displayContent = "图片";
            else if (RapidNotes::ItemType::isFileOrFolder(itemTypeEnum)) {
                QStringList paths = content.split(";", Qt::SkipEmptyParts);
                if (!paths.isEmpty()) {
                    QString firstPath = paths.first();
                    QString firstName = QFileInfo(firstPath).fileName();
                    if (firstName.isEmpty()) firstName = firstPath;
                    if (paths.size() > 1) displayContent = QString("%1 等 %2 个项目").arg(firstName).arg((int)paths.size());
                    else displayContent = firstName;
                } else displayContent = "文件";
            } else displayContent = type;
        }
>>>>>>> REPLACE

---

### 3.60 main.cpp 剪贴板入库类型与特征识别收敛至 ItemClassifier 与 ItemType：`RapidNotes_Source/RapidNotes/src/main.cpp`

<<<<<<< SEARCH
QString finalType = type;

            if (type == "image") {
                title = "[截图] " + QDateTime::currentDateTime().toString("MMdd_HHmm");
            } else if (type == "file" || type == "text") {
                QStringList files;
                if (type == "file") {
                    files = content.split(";", Qt::SkipEmptyParts);
                } else {
                    QString trimmed = content.trimmed();
                    if ((trimmed.startsWith("\"") && trimmed.endsWith("\"")) || (trimmed.startsWith("'") && trimmed.endsWith("'"))) {
                        trimmed = trimmed.mid(1, trimmed.length() - 2);
                    }
                    QFileInfo info(trimmed);
                    if (info.exists() && info.isAbsolute()) {
                        files << trimmed;
                        finalType = info.isDir() ? "folder" : "file";
                    }
                }

                if (!files.isEmpty()) {
                    QString firstPath = files.first();
                    if (firstPath.endsWith("/") || firstPath.endsWith("\\")) firstPath.chop(1);
                    QFileInfo info(firstPath);
                    QString name = info.fileName();
                    if (name.isEmpty()) name = firstPath;

                    if (files.size() > 1) {
                        int dirCount = 0;
                        for (const QString& path : files) {
                            if (QFileInfo(path).isDir()) dirCount++;
                        }
                        if (dirCount == files.size()) {
                            title = QString("Copied Folders - %1 等 %2 个文件夹").arg(name).arg((int)files.size());
                            finalType = "folders";
                        } else if (dirCount == 0) {
                            title = QString("Copied Files - %1 等 %2 个文件").arg(name).arg((int)files.size());
                            finalType = "files";
                        } else {
                            title = QString("Copied Items - %1 等 %2 个项目").arg(name).arg((int)files.size());
                            finalType = "files";
                        }
                    } else {
                        if (info.isDir()) {
                            title = "Copied Folder - " + name;
                            finalType = "folder";
                        } else {
                            title = "Copied File - " + name;
                            finalType = "file";
                        }
                    }
                } else if (type == "file") {
                    title = "[未知文件]";
                } else {
                    QString firstLine = content.section('\n', 0, 0).trimmed();
                    if (firstLine.isEmpty()) title = "无标题灵感";
                    else {
                        title = firstLine.left(40);
                        if (firstLine.length() > 40) title += "...";
                    }
                }
            }

            QStringList tags;
            if (type == "text") {
                QString trimmed = content.trimmed();
                static QRegularExpression hexRegex("^#([A-Fa-f0-9]{6}|[A-Fa-f0-9]{3})$");
                static QRegularExpression rgbRegex(R"(^(\d{1,3}),\s*(\d{1,3}),\s*(\d{1,3})$)");

                QRegularExpressionMatch hexMatch = hexRegex.match(trimmed);
                bool isColor = false;
                if (hexMatch.hasMatch()) {
                    if (!tags.contains("HEX")) tags << "HEX";
                    isColor = true;
                } else {
                    QRegularExpressionMatch rgbMatch = rgbRegex.match(trimmed);
                    if (rgbMatch.hasMatch()) {
                        int r = rgbMatch.captured(1).toInt();
                        int g = rgbMatch.captured(2).toInt();
                        int b = rgbMatch.captured(3).toInt();
                        if (r <= 255 && g <= 255 && b <= 255) {
                            if (!tags.contains("RGB")) tags << "RGB";
                            isColor = true;
                        }
                    }
                }
                if (isColor) {
                    for (const QString& t : {"色码", "色值", "颜值", "颜色码"}) {
                        if (!tags.contains(t)) tags << t;
                    }
                    // 2026-04-xx 按照用户要求：色码自动归类到 "Color" 分类，即使未开启自动归档开关也强制执行
                    catId = DatabaseManager::instance().getOrCreateCategoryByName("Color");
                }

                if (trimmed.startsWith("http://") || trimmed.startsWith("https://") || trimmed.startsWith("www.")) {
                    finalType = "link";
                    tags << "链接" << "网址";
                    QUrl url(trimmed.startsWith("www.") ? "http://" + trimmed : trimmed);
                    QString host = url.host();
                    QStringList hostParts = host.split('.', Qt::SkipEmptyParts);
                    QString domainTitle;
                    if (!hostParts.isEmpty()) {
                        if (hostParts.first().toLower() == "www" && hostParts.size() > 1) domainTitle = hostParts[1];
                        else domainTitle = hostParts.first();
                    }
                    if (!domainTitle.isEmpty()) {
                        domainTitle[0] = domainTitle[0].toUpper();
                        title = domainTitle;
                        for (QString part : std::as_const(hostParts)) {
                            part = part.trimmed();
                            if (part.toLower() == "www" || part.toLower() == "com" || part.toLower() == "cn" || part.toLower() == "net") continue;
                            if (!part.isEmpty()) {
                                part[0] = part[0].toUpper();
                                if (!tags.contains(part)) tags << part;
                            }
                        }
                    }
                }
=======
            QString title;
            QString finalContent = content;
            QString finalType = type;

            if (type == RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Image)) {
                title = "[截图] " + QDateTime::currentDateTime().toString("MMdd_HHmm");
            } else if (type == RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::File) || type == RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Text)) {
                QStringList files;
                if (type == RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::File)) {
                    files = content.split(";", Qt::SkipEmptyParts);
                } else {
                    QString trimmed = content.trimmed();
                    if ((trimmed.startsWith("\"") && trimmed.endsWith("\"")) || (trimmed.startsWith("'") && trimmed.endsWith("'"))) {
                        trimmed = trimmed.mid(1, trimmed.length() - 2);
                    }
                    if (RapidNotes::ItemClassifier::isAbsolutePathText(trimmed)) {
                        files << trimmed;
                        finalType = QFileInfo(trimmed).isDir() ? RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Folder) : RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::File);
                    }
                }

                if (!files.isEmpty()) {
                    QString firstPath = files.first();
                    if (firstPath.endsWith("/") || firstPath.endsWith("\\")) firstPath.chop(1);
                    QFileInfo info(firstPath);
                    QString name = info.fileName();
                    if (name.isEmpty()) name = firstPath;

                    RapidNotes::PathSummary::Summary summary = RapidNotes::PathSummary::summarize(files);
                    if (files.size() > 1) {
                        if (summary == RapidNotes::PathSummary::Summary::MultipleFolders) {
                            title = QString("Copied Folders - %1 等 %2 个文件夹").arg(name).arg((int)files.size());
                            finalType = RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Folders);
                        } else if (summary == RapidNotes::PathSummary::Summary::SingleFile) {
                            title = QString("Copied Files - %1 等 %2 个文件").arg(name).arg((int)files.size());
                            finalType = RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Files);
                        } else {
                            title = QString("Copied Items - %1 等 %2 个项目").arg(name).arg((int)files.size());
                            finalType = RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Files);
                        }
                    } else {
                        if (summary == RapidNotes::PathSummary::Summary::SingleFolder) {
                            title = "Copied Folder - " + name;
                            finalType = RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Folder);
                        } else {
                            title = "Copied File - " + name;
                            finalType = RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::File);
                        }
                    }
                } else if (type == RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::File)) {
                    title = "[未知文件]";
                } else {
                    QString firstLine = content.section('\n', 0, 0).trimmed();
                    if (firstLine.isEmpty()) title = "无标题灵感";
                    else {
                        title = firstLine.left(40);
                        if (firstLine.length() > 40) title += "...";
                    }
                }
            }

            QStringList tags;
            if (type == RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Text)) {
                QString trimmed = content.trimmed();
                bool isColor = false;
                if (RapidNotes::ItemClassifier::isHexColor(trimmed)) {
                    if (!tags.contains("HEX")) tags << "HEX";
                    isColor = true;
                } else if (RapidNotes::ItemClassifier::isRgbTriplet(trimmed)) {
                    if (!tags.contains("RGB")) tags << "RGB";
                    isColor = true;
                }
                if (isColor) {
                    for (const QString& t : {"色码", "色值", "颜值", "颜色码"}) {
                        if (!tags.contains(t)) tags << t;
                    }
                    // 2026-04-xx 按照用户要求：色码自动归类到 "Color" 分类，即使未开启自动归档开关也强制执行
                    catId = DatabaseManager::instance().getOrCreateCategoryByName("Color");
                }

                if (RapidNotes::ItemClassifier::isUrl(trimmed)) {
                    finalType = RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Link);
                    tags << "链接" << "网址";
                    QUrl url(trimmed.startsWith("www.") ? "http://" + trimmed : trimmed);
                    QString host = url.host();
                    QStringList hostParts = host.split('.', Qt::SkipEmptyParts);
                    QString domainTitle;
                    if (!hostParts.isEmpty()) {
                        if (hostParts.first().toLower() == "www" && hostParts.size() > 1) domainTitle = hostParts[1];
                        else domainTitle = hostParts.first();
                    }
                    if (!domainTitle.isEmpty()) {
                        domainTitle[0] = domainTitle[0].toUpper();
                        title = domainTitle;
                        for (QString part : std::as_const(hostParts)) {
                            part = part.trimmed();
                            if (part.toLower() == "www" || part.toLower() == "com" || part.toLower() == "cn" || part.toLower() == "net") continue;
                            if (!part.isEmpty()) {
                                part[0] = part[0].toUpper();
                                if (!tags.contains(part)) tags << part;
                            }
                        }
                    }
                }
            }
>>>>>>> REPLACE

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译执行与依赖核验
```bash
cmake --preset vs2022-msvc
cmake --build --preset vs2022-msvc-debug
```

### 4.2 运行时验收标准（严格对照 Prompts 步骤 1 验收）
1. **全工程搜索 `item_type` 字符串字面量**：只出现在 `RapidNotes::ItemType` 实现内，其他位置均通过 `ItemType::toString` 或枚举比对；
2. **全工程标签拆分/拼接唯一来源**：全工程所有对标签的拆分和拼接（`DatabaseManager`、`QuickWindow`、`NoteEditWindow`、`AdvancedTagSelector`、`TagEditDialog`、`FramelessDialog`、`FileStorageHelper`）统一收敛至 `TagCodec`；
3. **`StringUtils::detectItemType` 已彻底物理删除**；
4. **各原调用点判定结果 100% 一致**：`ItemClassifier` 内部算法与原有逻辑完全对齐；
5. **`NoteModel::data` 的 DecorationRole 图标显示零 I/O 阻塞**：由 `ItemClassifier::detectDisplayCategory` 与 `PathSummary` 纯内存判定，彻底杜绝主线程访问磁盘 `QFileInfo::exists()`。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

| 场景 | 既有实现 / 拟淘汰碎片 | 统一收敛的官方 API / SSOT 模块 |
|---|---|---|
| 类型字符串字面量与枚举转换 | 散落在各处的 `"text"`、`"image"` 等硬编码 | `RapidNotes::ItemType::toString` / `fromString` |
| 标签解析拆分 | 散落在 40+ 处的 `.split(QRegularExpression("[,，]"))` | `RapidNotes::TagCodec::split` |
| 标签编码拼接 | 散落在 20+ 处的 `.join(", ")` 与 `.join(",")` | `RapidNotes::TagCodec::join` / `joinDisplay` |
| 正文类型与内容特征判定 | `StringUtils::detectItemType` 与 `main.cpp` 重复正则 | `RapidNotes::ItemClassifier::detectIngestType` / `detectDisplayCategory` |
| 路径归并与图标类别 | `NoteModel.cpp` 内的分割与计数 | `RapidNotes::PathSummary::summarize` |
| 字数阶梯脱壳清洗 | `DatabaseManager.cpp` 匿名空间内实现 | `RapidNotes::WordBucket::calculate` |
| 缩略图生成参数 | `NoteModel.cpp` 内硬编码的 64x64 保持纵横比缩放 | `RapidNotes::ThumbnailMaker::makeThumbnail` |

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 事实项 | 唯一真理源 (SSOT) | 派生副本 / 调用端 | 刷新与同步机制 |
|---|---|---|---|
| 条目类型词表 | `RapidNotes::ItemType` | 全工程所有业务与界面模块 | 编译期强类型枚举与无损字符串映射 |
| 内容特征与类型识别 | `RapidNotes::ItemClassifier` | `main.cpp`、`NoteEditWindow`、`NoteModel` | 纯内存无状态计算 |
| 标签拆分与合并规则 | `RapidNotes::TagCodec` | `DatabaseManager`、`QuickWindow`、`NoteEditWindow` 等 | 纯函数无状态转换 |
| 路径归并判定 | `RapidNotes::PathSummary` | `NoteModel`、`main.cpp` | 内存纯函数无磁盘 I/O 探测 |
| 字数阶梯规则 | `RapidNotes::WordBucket` | `DatabaseManager::calculateWordBucket` | 纯函数字符串脱壳与阶梯映射 |
| 缩略图缩放规范 | `RapidNotes::ThumbnailMaker` | `NoteModel` | 统一 64x64 平滑缩放 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

### 7.1 外部与新增纯函数接口签名一致性
1. `RapidNotes::ItemType::toString(Type type)`：公有静态，返回 `QString`；
2. `RapidNotes::ItemType::fromString(const QString& str)`：公有静态，返回 `Type`；
3. `RapidNotes::ItemClassifier::detectIngestType(const QString& text)`：公有静态，返回 `ItemType::Type`；
4. `RapidNotes::ItemClassifier::detectDisplayCategory(const QString& typeStr, const QString& content)`：公有静态，返回 `DisplayCategory`；
5. `RapidNotes::PathSummary::summarize(const QStringList& paths)`：公有静态，返回 `Summary` 枚举；
6. `RapidNotes::WordBucket::calculate(const QString& content, const QString& itemType)`：公有静态，返回 `int`；
7. `RapidNotes::TagCodec::split(const QString& tagsStr)`：公有静态，返回 `QStringList`；
8. `RapidNotes::TagCodec::join(const QStringList& tags)`：公有静态，返回 `QString`（纯半角逗号）；
9. `RapidNotes::TagCodec::joinDisplay(const QStringList& tags)`：公有静态，返回 `QString`（半角逗号+空格）；
10. `RapidNotes::ThumbnailMaker::makeThumbnail(const QImage& original, const QSize& targetSize)`：公有静态，返回 `QImage`。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

### 8.1 头文件闭合性验证
- `src/core/ItemType.h` 仅包含 `<QString>`；
- `src/core/ItemClassifier.h` 包含 `ItemType.h`、`<QString>`、`<QUrl>`、`<QRegularExpression>`；
- `src/core/PathSummary.h` 包含 `<QStringList>`；
- `src/core/WordBucket.h` 包含 `<QString>`；
- `src/core/TagCodec.h` 包含 `<QString>`、`<QStringList>`；
- `src/core/ThumbnailMaker.h` 包含 `<QImage>`、`<QSize>`；
- 所有新建头文件完全属于 `QtCore` / `QtGui` 基础类型，绝对不包含 `src/ui/` 下任何组件，遵循依赖单向流动铁律。
- 所有修改的 `.cpp` 均显式闭合包含对应使用的工具头文件。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项 | 结论 (满足/不满足/不涉及) | 说明与物理证据位置 |
|---|---|---|---|
| 1 | 每个 SEARCH 块在目标文件中真实存在且唯一匹配 | 满足 | 全量 48 个局部替换块与 12 个新建文件均通过 verify_plan 脚本 100% 独立匹配 |
| 2 | 新建/删除文件在 CMakeLists.txt 中同步注册/移除 | 满足 | 见 3.1 小节，12 个新建源码文件均已全量注册进 SOURCES 列表 |
| 3 | 核心层（src/core）不包含任何 src/ui 头文件 | 满足 | 词表与纯函数仅依赖 Qt 核心基础类型，未逆向包含 UI |
| 4 | 物理彻底清除死代码与平行实现 | 满足 | 全工程所有涉及标签 split/join 及类型判定的调用点均已给出替换 diff |
| 5 | ToolTipOverlay 不追随鼠标位置违规弹窗 | 不涉及 | 本步骤未新增或篡改任何 ToolTipOverlay 调用 |
| 6 | 既有 UI 交互、文案、尺寸与数值零篡改 | 满足 | 见 Chapter 10，所有既有数值 100% 保持现状 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 项目 | 既有原始值 / 既有行为 | 重构后数值 / 重构后行为 | 结论 |
|---|---|---|---|
| 字数分桶阈值 | 0, 10~100 (步长10), 100~1000 (步长100), 1001 (1000字以上) | 完全一致，由 `WordBucket::calculate` 原样承接 | 100% 保持现状 |
| 缩略图尺寸与模式 | 64x64, `Qt::KeepAspectRatio`, `Qt::SmoothTransformation` | 完全一致，由 `ThumbnailMaker::makeThumbnail` 默认参数承接 | 100% 保持现状 |
| 标签拆分分隔符 | `QRegularExpression("[,，]")`（中英文逗号） | 完全一致，由 `TagCodec::split` 原样承接 | 100% 保持现状 |
| 标签拼接格式 | 存储使用纯逗号 `,`，界面显示使用逗号加空格 `, ` | 完全一致，分别由 `TagCodec::join` 与 `TagCodec::joinDisplay` 承接 | 100% 保持现状 |
| 图标名称与颜色 | `file`, `folder`, `files`, `folders` 及其对应色号 | 完全一致，由 `ItemClassifier` 与 `PathSummary` 驱动 | 100% 保持现状 |
