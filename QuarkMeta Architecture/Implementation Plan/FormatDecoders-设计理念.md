基于 `FormatDecoders.cpp` 源码的严密逐行核对，**是的，您的理解完全准确！**

针对 **EPS** 文件的提取流程，代码正是严格按照**“先内存级内嵌提取（TIFF / EPSI） $\rightarrow$ 失败后再启动 Ghostscript 进程兜底”**的降级梯队设计的：

---

### 一、EPS 缩略图提取的严格时序链（`extractEpsPreview`）

```
[开始提取 EPS 文件]
       │
       ▼
【第 1 顺位：DOS 二进制头内嵌 TIFF 解析】 (耗时 < 1ms)
  • 检查文件头 4 字节魔数 `0xC5 0xD0 0xD3 0xC6`；
  • 读取头部 offset/length 直接定位内嵌 TIFF 数据；
  • 调用 `decodeTiffMemorySafely()` 纯内存秒解。
  • 命中 ──▶ 立即返回！
       │ (未命中 / 无二进制头)
       ▼
【第 2 顺位：ASCII EPSI 预览块解析】 (耗时 < 5ms)
  • 扫描 PostScript 文本中的 `%%BeginPreview:` ~ `%%EndPreview`；
  • 读取十六进制 ASCII 数据并由 `QByteArray::fromHex` 内存还原；
  • 命中 ──▶ 立即返回！
       │ (未命中 / 无内嵌预览图)
       ▼
【第 3 顺位：Ghostscript 矢量渲染引擎兜底】 (耗时 50ms ~ 2000ms)
  • 抢占并发信号量 `g_gsConcurrencyLimit`（限制最多 1 个 GS 进程，防止打爆 CPU）；
  • 启动 `gswin64c.exe` 进程，对 EPS 矢量代码进行实时光栅化渲染；
  • 命中 ──▶ 缓存并返回！
```

---

### 二、AI 文件提取的对应时序链（`extractAiPreview`）

AI 格式同样严格遵循**“先免渲染提取内嵌图，后起重量级引擎”**的降级原则：

1. **第 1 顺位**：解析 `%AI7_Thumbnail` ~ `%AI10_Thumbnail`（256 色索引调色板，纯内存秒级，耗时 < 1ms）；
2. **第 2 顺位**：解析 Adobe XMP 元数据 `<xmpGImg:image>` 中的 Base64 JPEG 缩略图（耗时 < 2ms）；
3. **第 3 顺位**：Ghostscript 矢量引擎渲染；
4. **第 4 顺位**：Windows 原生 PDF 渲染管道；
5. **第 5 顺位**：裸 JPEG/PNG 二进制流扫描；
6. **第 6 顺位**：Windows Shell 缩略图兜底。

---

### 三、这种设计的核心优势

- **90% 以上由 Adobe / 常见设计软件导出的 EPS/AI 文件**都会自带 TIFF 或 EPSI 预览流，前两个顺位在 **1~2 毫秒** 内纯内存直出；
- **重型的 Ghostscript 外部进程**只有在完全没有内嵌预览的“裸矢量 EPS”时才会被触发兜底，彻底避免了因频繁启动多进程导致的目录卡顿与 CPU 飙高。