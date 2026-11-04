针对 QuarkMeta_Source、RapidNotes_Source/RapidNotes、FocusLock__Source、Time_Source/Timetool、FERREX-META_Source 五个项目版本统一重新编译生成项目的设计理念

**完全明白，而且印象极其深刻！**

这套设计理念的核心逻辑非常清晰、自洽，本质上是一个**“三权分立”的高效工程闭环**：

1. **源码层 (`<AppName>_Source`)**：
   * **定位**：代码圣殿、Git 唯一领地。
   * **原则**：绝对的《源码纯净》，物理上把二进制隔绝在外，彻底封死误推大文件到 GitHub 的通道。

2. **中间构建层 (`<AppName>_Build/<presetName>/`)**：
   * **定位**：消耗品缓存区。
   * **原则**：任由 Ninja 和 MSVC 堆放成千上万个 `.obj`、`.ninja`、日志缓存；遇到玄学 Bug 随时全选删除（Clean），毫不心疼。

3. **固化运行层 (`<AppName>_Build/<AppName>/`)**：
   * **定位**：免安装绿色沙盒。
   * **原则**：一次部署（`windeployqt`）、终生受用；日常编译只覆盖最新的单个 `<AppName>.exe`，彻底告别重复部署 Qt DLL 的折磨，随时双击立即可跑，随时打包立即可发。

---

我已经将这个体系化规则牢牢记住了。后续不管是 `QuarkMeta`、`Timetool` 还是你开发的其他新应用，我都会严格遵循这套 **“`<AppName>_Source` + `<AppName>_Build/<AppName>/<AppName>.exe`”** 的规范来协助你设计和排查问题！