全软件（包括主内容面板、侧边栏、收藏夹、标签页、快照预览等 17 个 UI 模块）的右键/弹出菜单选项全量排查与统计结果如下：

📊 数量汇总：
核心内容面板（ContentContextMenu.cpp）右键菜单项：共 50 个选项/子菜单。
全软件跨模块右键/弹出菜单总计：共 125 个选项/子菜单。
📋 核心内容面板右键菜单全量选项清单（按上下文场景分类）：
场景 1：回收站视图右键菜单（5 个选项）
还原 (ActionRestore)
剪切 (ActionCut)
永久删除 (ActionSecureDelete)
还原全部 (ActionRestoreAll)
清空回收站 (ActionEmptyTrash)
场景 2：盘符根目录/物理卷右键菜单（5 个选项）
打开 (ActionOpen)
在新标签页中打开 (ActionOpenInNewTab)
在“资源管理器”中显示 (ActionShowInExplorer)
色标选择器 (ColorStripPicker)
粘贴 (ActionPaste)
场景 3：普通文件/文件夹右键菜单（22 个选项及子菜单）
打开 / 打开文件夹 (ActionOpen)
在新标签页中打开 (ActionOpenInNewTab)
用系统默认程序打开 (ActionOpenDefault)
在 QuarkMeta 中显示 (ActionShowInQuarkMeta)
复制 (ActionCopy)
剪切 (ActionCut)
移动到... [子菜单] (MoveToFolder，含快捷路径与 浏览选择文件夹...)
置顶 / 取消置顶 (PinToggleAction / ActionPin / ActionUnpin)
收藏 / 取消收藏 (FavoriteAction)
粘贴 (ActionPaste)
复制标签 (ActionCopyTags)
粘贴标签 (ActionPasteTags)
重复上一次操作 (ActionRepeatLastOp - F4)
重命名 (ActionRename)
重新提取缩略图 (ActionReextractThumbnail)
外壳保护 [子菜单]
执行外壳保护 (ActionEncrypt)
解除保护 (ActionDecrypt)
修改保护密码 (ActionChangePwd)
更多... [子菜单]（包含 支持提取内容 / 不支持提取内容）
刷新 (ActionRefresh)
在“资源管理器”中显示 (ActionShowInExplorer)
场景 4：空白区域右键菜单（18 个选项及子菜单）
新建... [子菜单]
创建文件夹 (ActionNewFolder)
创建 Markdown (ActionNewMd)
创建纯文本文件 (txt) (ActionNewTxt)
批量创建项目... (ActionBatchCreate)
粘贴 (ActionPaste)
在“资源管理器”中显示 (ActionShowInExplorer)
关闭窗格 (closePaneAction)
排序 [子菜单]（含 按名称 / 按大小 / 按修改时间 / 按类型 升序/降序 8 个分支）
删除 [子菜单]
移入回收站 (ActionDelete)
永久删除 (ActionSecureDelete)
刷新 (ActionRefresh)
⚠️ 未被归一化及违背 SSOT 规范汇总（诊断摘要）：
“置顶/取消置顶”：违背归一化/SSOT（存在 CoreEngine、MetadataManager 穿透、 ViewModel IsLockedRole 3 套分裂逻辑）。
“粘贴标签”：违背归一化/SSOT（ContentContextMenu 和 ContentKeyHandler 分别手写 model->setData 遍历）。
“在新标签页中打开”：违背依赖锁（手写控件树查找 findChild<TitleBarWidget*> 跨层下钻）。
“复制路径 / 复制名称”：违背归一化/SSOT（换行符 \n 与 \r\n 分散在 3 个控制器，缺少统一转换入口）。
“文件加密/解密/改密”完成回调：违背 SSOT 行为字典（回调中违规调用 loadDirectory 另起炉灶重置视角，应调用 refreshAll 原位刷新）。
请指示是否需要为上述特定问题开始编制 physical implementation plan 实施方案？