# QuickLookWindow 冗余多媒体代码彻底净化与物理根除 —— Modification_Plan-38.md

> 状态：待批准执行（尚未获得用户"批准执行"指令）

## 1. 任务背景
在空格键打开的快速预览界面（QuickLookWindow）中，虽然之前的对话明确要求彻底根除关于多媒体播放的功能，但代码库里仍然残留着多媒体容器、播放/暂停控制按钮、进度条、音频占位框以及与之配套的 `renderMedia`、`resetMedia` 等冗余多媒体相关声明与功能体。
本方案将对该冗余多媒体代码进行彻底的物理根除与净化，确保代码库完全无冗余多媒体成分。

## 2. 问题定位
- **定位模块 1（src/ui/QuickLookWindow.h）**：
  残存的多媒体开关宏 `ARCMETA_HAS_MULTIMEDIA`、多媒体类头文件声明、大批多媒体成员变量（`m_mediaContainer`、`m_videoWidget`、`m_mediaPlayer`、`m_audioOutput`、`m_playBtn`、`m_timeSlider`、`m_timeLabel`、`m_audioPlaceholder`）、以及 `renderMedia`、`resetMedia`、`formatTime`。
- **定位模块 2（src/ui/QuickLookWindow.cpp）**：
  - 残余的静态多媒体后缀列表 `AUDIO_EXTS` 与 `VIDEO_EXTS`。
  - `setupUi` 中大段的多媒体组件创建、滑块布局与按钮绑定的构建逻辑。
  - `keyPressEvent` 中对键盘按键 `P` 播放/暂停的处理逻辑。
  - `preview` / `previewFile` 调用 `renderMedia` 流程。
  - `renderMedia()`、`resetMedia()`、`formatTime()` 函数实现。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 彻底根除关于多媒体播放的功能 | 彻底物理删除 `QuickLookWindow.h` 和 `QuickLookWindow.cpp` 中的所有多媒体相关成员变量、组件创建、函数声明 and 按键 `P` 控制事件，关闭多媒体宏定义。 | ✅ 一致 |

## 4. 详细解决方案

### 4.1 物理删除 `QuickLookWindow.h` 的残留代码
彻底移去如下定义和成员：
- 物理删除 `ARCMETA_HAS_MULTIMEDIA` 宏及关联的 `<QMediaPlayer>` 等头文件引入。
- 物理删除以下声明：
  ```cpp
  void renderMedia(const QString& path);
  void resetMedia();
  QString formatTime(qint64 ms);
  ```
- 物理删除以下多媒体界面与引擎实例变量：
  ```cpp
  // 媒体播放器组件
  QWidget* m_mediaContainer = nullptr;
  QWidget* m_videoWidget = nullptr;
  QObject* m_mediaPlayer = nullptr;
  QObject* m_audioOutput = nullptr;
  QPushButton* m_playBtn = nullptr;
  QSlider* m_timeSlider = nullptr;
  QLabel* m_timeLabel = nullptr;
  QLabel* m_audioPlaceholder = nullptr; 
  ```

### 4.2 物理删除 `QuickLookWindow.cpp` 的残留逻辑与构建
- **删除多媒体静态后缀**：
  物理删除定义：
  ```cpp
  static const QSet<QString> AUDIO_EXTS = { ... };
  static const QSet<QString> VIDEO_EXTS = { ... };
  ```
- **删除 `setupUi` 中的构建流程**：
  删除 `m_mediaContainer` 的创建、多媒体布局（`mediaLayout`）、进度滑块和播放按钮初始化。
- **删除快捷键 `P` 逻辑**：
  从 `keyPressEvent` 中彻底移除 `if (event->key() == Qt::Key_P) { ... }` 块。
- **删除实现函数**：
  完全抹除 `renderMedia`、`resetMedia`、`formatTime` 的实现。
- **重新分流音视频文件（转为系统图标降级）**：
  在 `preview(const QString& filePath)` 中，移除涉及音频和视频调用 `renderMedia` 的分支。对于音频、视频等文件，统一走默认的降级显示（即系统图标+不预览提示）。

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] 模块/文件：`src/ui/QuickLookWindow.h` 与 `src/ui/QuickLookWindow.cpp` (多媒体彻底物理大扫除)

**明确禁止越界修改的范围：**
- [ ] MFT 与 USN 底层逻辑——不修改。
- [ ] 滚动条样式修改逻辑（已在 Modification_Plan-37 中处理）——不修改。

## 6. 实现准则与预警【核心】
1. 完全不保留对多媒体框架的任何声明、条件宏以及未用变量，绝不在代码库中造成变量污染。
2. 删除多媒体代码后，进行全项目无警告重构构建。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求（写具体内容，不写引用） | 本方案是否符合 |
|-------------|----------------------|----------------|
| 快速预览 (QuickLook) 进阶规范 | 必须采用“黑名单拦截+白名单准入”的双重防御机制。严禁预览文件夹、安装包 (.msi, .exe)、系统库 (.dll, .sys) 及各类压缩包。 | ✅ 符合。彻底删除音视频格式的主动预览，将其并入不支持预览的拦截格式，全部统一走系统大图标防御性展示，不留任何安全和崩溃隐患。 |
