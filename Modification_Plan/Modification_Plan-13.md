# 借鉴 FERREX-META 重构专业级 QuickLookWindow 预览窗口方案 —— Modification_Plan-13.md

## 1. 任务背景
在当前版本中，`QuickLookWindow` 快速预览窗口的设计存在着严重的格式死角和逻辑硬伤：
- **类的设计臃肿**：全部事件杂糅在单个类中，大图拖动极易失效，且缺失鼠标双击还原比例和拖拽手势。
- **文本乱码严重**：硬编码使用 `QString::fromUtf8()` 解码全部文本，遇到 GBK 或 UTF-16 文本时直接满屏乱码；且遇到非文本的二进制文件（`.bin` / `.dat`）仍然无脑加载，体验极差。
- **按键事件拦截冲突**：文本框获得焦点后，空格键会被其底层组件吞噬用于向下翻页，导致全局 `QShortcut` 关闭快捷键直接失效。
- **大图缩放边缘破碎**：直接加载大图交给 QGraphicsView 的低质量插值算法，导致缩小显示时产生大面积斑驳和锯齿。

为了解决上述问题，我们将完全参考并借鉴 `FERREX-META` 的优秀成熟设计，对当前版本的 `QuickLookWindow` 进行专业级重构，并且**特别明确：不包含“音视频播放”功能**。

---

## 2. 问题定位与移植对账
- **视图与外层窗口解耦**：
  在 `src/ui/` 目录下，剥离出独立的 `QuickLookGraphicsView`。重写 `wheelEvent`、`mouseDoubleClickEvent`、`mousePressEvent` 和 `mouseReleaseEvent`，实现手势自适应抓手及双击无极切换 100% 原始尺寸。
- **智能编码解码器注入**：
  在 `QuickLookWindow` 中注入 `detectEncoding(const QByteArray& fileData)` 和 `isBinary(const QByteArray& fileData)` 校验。读取前 128KB 文本时，若为二进制，自动阻断并优雅退避到显示系统大图标；若为正常文本，自动探测 UTF-8、GBK/ANSI、UTF-16LE/BE 并调用对应的解码机制。
- **事件拦截过滤器拦截**：
  废弃容易被吞噬的 QShortcut。为 `m_textPreview` (QPlainTextEdit) 安装专属事件过滤器，在 `eventFilter` 里最高物理优先级精准拦截 `Qt::Key_Space` 键，执行 `hide()`，彻底解决按键冲突死锁。
- **重采样降锯齿处理**：
  在 `renderImage` 和 `renderProfessionalImage` 中，加载原图后，若其尺寸超出视口大小，优先使用 QImage 的 `SmoothTransformation`（面积平均重采样）对其进行高质量预缩放，解决高频细节锯齿（Moiré 纹），同时对 SVG 实施 1:1 矢量渲染。

---

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 重构预览并且是按照 FERREX-META 去重构。 | 参考并引入其独立的视图管理类 `QuickLookGraphicsView` 和空格过滤、文本智能探测等核心机制。 | ✅ 一致 |
| 2    | 另外不需要包含“音视频播放”。 | 本方案重构中完全剔除了 QMediaPlayer 播放器、时间轴滑块及视频组件，仅聚焦于文本与图像的高精、防乱码预览。 | ✅ 一致 |
| 3    | 解决大图缩小锯齿、文本 GBK 乱码和文本区空格键不关闭冲突。 | 在 `Modification_Plan-13.md` 中设计双击 100% 比例切换、GBK 自适应解码及文本事件拦截器，完全解决上述痛点。 | ✅ 一致 |

---

## 4. 详细解决方案

### 4.1 类解耦：构建独立的 `QuickLookGraphicsView` (参考 FERREX-META)
在 `src/ui/QuickLookWindow.h` 和 `QuickLookWindow.cpp` 中拆分出独立的视图类，接管图像显示和手势：
```cpp
class QuickLookGraphicsView : public QGraphicsView {
    Q_OBJECT
public:
    explicit QuickLookGraphicsView(QWidget* parent = nullptr);
    void setPixmap(const QPixmap& pixmap);
    void fitImage();
    void setZoomOriginal();
    void clear();

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void updateCursor();
    QGraphicsScene* m_scene = nullptr;
    QGraphicsPixmapItem* m_pixmapItem = nullptr;
    double m_currentScale = 1.0;
    bool m_isFitMode = true;
};
```
在实现中：
*   **双击事件**：如果当前处于 `m_isFitMode`（自适应），双击调用 `setZoomOriginal()`（切换 1.0 原始比例）；反之双击调用 `fitImage()`，操作极其丝滑。
*   **拖动光标**：在 `updateCursor()` 中，检测当前图片在当前比例下是否超出了视口大小。若超出，将鼠标光标切换为 `Qt::OpenHandCursor`（手掌手势），按下时切换为 `Qt::ClosedHandCursor`（抓握手势），并允许用户自由拖拽移动视口，彻底消除“大图无法拖拽查看边缘”的傻逼体验。

---

### 4.2 智能字符集感知（自适应 GBK / ANSI TXT 预览）
在 `QuickLookWindow` 中实现智能编码感知算法，完全根治乱码和二进制穿透：
```cpp
// 1. 二进制强力校验，防止非文本数据穿透乱码
bool QuickLookWindow::isBinary(const QByteArray& fileData) {
    if (fileData.isEmpty()) return false;
    int checkLen = std::min<int>(fileData.size(), 1024);
    int continuousNull = 0;
    for (int i = 0; i < checkLen; ++i) {
        if (fileData[i] == '\0') {
            continuousNull++;
            if (continuousNull > 2) return true; // 连续 Null 字符，判定为二进制
        } else {
            continuousNull = 0;
        }
    }
    return false;
}

// 2. 编码智能探测
QString QuickLookWindow::detectEncoding(const QByteArray& fileData) {
    if (fileData.startsWith("\xEF\xBB\xBF")) return "UTF-8";
    if (fileData.startsWith("\xFF\xFE")) return "UTF-16LE";
    if (fileData.startsWith("\xFE\xFF")) return "UTF-16BE";

    // 统计 UTF-8 连续多字节指纹特征数
    int utf8Count = 0;
    for (int i = 0; i < fileData.size() - 2; ++i) {
        unsigned char c = (unsigned char)fileData[i];
        if (c >= 0xC0 && c <= 0xDF) {
            if ((unsigned char)fileData[i+1] >= 0x80 && (unsigned char)fileData[i+1] <= 0xBF) { utf8Count++; i++; }
        } else if (c >= 0xE0 && c <= 0xEF) {
            if ((unsigned char)fileData[i+1] >= 0x80 && (unsigned char)fileData[i+1] <= 0xBF &&
                (unsigned char)fileData[i+2] >= 0x80 && (unsigned char)fileData[i+2] <= 0xBF) { utf8Count += 2; i += 2; }
        }
    }
    return (utf8Count > 0) ? "UTF-8" : "GBK"; // 默认 GBK，完美解决本地中文字符乱码
}
```
并在 `renderText` 读取文本时，根据 `detectEncoding` 返回的名称调用 `QStringDecoder`（Qt 6 API）或 `fromLocal8Bit`（GBK/ANSI 解码），保证全格式文本完美自适应显示。

---

### 4.3 物理拦截空格冲突：高优先级文本框 `eventFilter`
为了彻底解决文本框拥有焦点时空格键被吞噬导致无法关闭窗口的冲突，我们在 `QuickLookWindow` 的 `eventFilter` 中实施最高优先级拦截：
```cpp
bool QuickLookWindow::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_textPreview && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Space) {
            hide(); // 100% 拦截并秒退窗口！
            return true; // 彻底切断该事件向 QPlainTextEdit 传递，不触发向下翻页
        }
    }
    return QWidget::eventFilter(watched, event);
}
```
同时在初始化 `m_textPreview` 时，显式调用：
```cpp
m_textPreview->installEventFilter(this);
```

---

### 4.4 面积平均高质量缩放处理（重采样消除锯齿）
在 `renderImage` 和 `renderProfessionalImage` 中，读取大图后，使用高质量算法：
```cpp
    QSize viewSize = m_graphicsView->size();
    if (!viewSize.isEmpty() && (img.width() > viewSize.width() || img.height() > viewSize.height())) {
        // 利用 Qt 原生面积加权平均采样（SmoothTransformation），高精度预缩放到视口大小
        img = img.scaled(viewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    QPixmap pix = QPixmap::fromImage(img);
```
这保证了当一张 4000x3000 的大图被塞入 800x600 的预览窗口时，由于经历了高质量的 QImage 预降采样，其缩小的显示效果依然极致细腻、圆润，完全没有直接 Pixmap 暴力缩小时产生的刺眼锯齿和摩尔纹。

---

## 5. 修改边界声明【红线】

**本次方案涉及范围：**
- [ ] 模块/文件：`src/ui/QuickLookWindow.h` 与 `src/ui/QuickLookWindow.cpp`
  - 新增并实现 `QuickLookGraphicsView` 类。
  - 在 `QuickLookWindow` 中实现 `detectEncoding()`、`isBinary()`。
  - 注册文本框的事件过滤器 `eventFilter`。
  - 重写 `renderImage` 的 `SmoothTransformation` 重采样预降噪算法。

**明确禁止越界修改的范围：**
- [ ] 严禁引入任何有关 `QMediaPlayer` 或 `QVideoWidget` 等多媒体音视频组件代码（按照用户最新约束：不包含音视频播放）。

---

## 6. 实现准则与预警【核心】
1. **防止 DPI 多屏缩放偏差**：
   - 移除原先手动设置的 `setDevicePixelRatio` 逻辑，交给解耦后的 QGraphicsView 以原始 Pixmap 像素去自动处理设备物理像素对位，消除高 DPI 屏下的二次模糊和边缘锯齿。
2. **文本内存映射安全边界**：
   - 对于海量文本大文件（如 `.log`），`renderText` 读取时必须保留 `file.read(128 * 1024)`（只读取前 128KB 预览缓冲）的限制，防止多线程加载超大文本产生严重的 UI 挂起和死锁。

---

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 快速预览 (QuickLook) 规范 | 针对文件夹及不可预览文件严禁激活 QuickLook，且大文件需自动降级 HD 缩略图引擎，全程开启 SmoothPixmapTransform。 | ✅ 符合。本方案将完全按照 Memories.md 第 10、11 条关于 QuickLook 性能和画质规范的铁律来实现。 |
| 滚动条样式 | 滚动条宽度 10px、圆角 3px、Handle 颜色对齐 BorderColor (#333333)。 | ✅ 符合。样式表设置完美保持该像素样式。 |
