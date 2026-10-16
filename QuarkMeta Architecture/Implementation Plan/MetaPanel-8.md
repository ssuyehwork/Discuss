【任务】元数据面板多选显示改造。只允许修改 MetaPanel.h、MetaPanel.cpp、PanelMediator.cpp 三个文件，其他文件一律不许动。

【背景】当前多选时：MetaPanel::setSelectedPaths 的多选分支调用 setImagePreview(QPixmap()) 把预览区隐藏，并把 m_nameEdit 设为纯文本"已选中 %1 个项目"，同时强制 setRating(0,false)、setColor("",false)；PanelMediator 的 updateMetaPanelFromPanel 只处理 paths 为空和 paths.size()==1 两种情况，多选时不读取任何项目的元数据。

【现有原文（供对照，若与仓库现状不一致，停下来如实说明，不得改写后继续）】
MetaPanel.cpp 多选分支开头：
    } else if (isMulti) {
        m_isInternalUpdating = true;
        setImagePreview(QPixmap());
        if (m_nameEdit) {
            m_nameEdit->setPlainText(QString("已选中 %1 个项目").arg(m_selectedPaths.size()));
        }
MetaPanel.cpp 多选分支中紧随其后的这两行：
        setRating(0, false);
        setColor(QString(""), false);
PanelMediator.cpp 的 updateMetaPanelFromPanel 中：
        if (paths.isEmpty()) { ... } else if (paths.size() == 1) { ... }
MetaPanel.h 现有预览相关公有接口：
    void setImagePreview(const QPixmap& pixmap);
    void setDefaultIconPreview(const QIcon& icon);

【目标行为】

一、顶部预览区：多选时显示叠放卡片
1. MetaPanel.h 新增一个公有方法（只新增，不改任何现有签名）：
   void setMultiSelectionPreview(const QList<QPixmap>& pixmaps);
2. 实现方式参照 MetaPanel::setDefaultIconPreview：逻辑画布 200x200，物理尺寸乘以 devicePixelRatioF()，画布 setDevicePixelRatio，结果设到 m_lblImagePreview，m_lblImagePreview 与 m_topPreviewBox 显示，最后 m_adjustTimer->start()。pixmaps 为空时等同于隐藏预览区（与 setImagePreview(QPixmap()) 的隐藏行为一致）。
3. 绘制规则：
   - 每张卡片按比例缩放到不超过 kMultiPreviewCardSize 的正方形内，居中于画布中心，圆角 4.0（与 setImagePreview 现有圆角一致），1px 描边 #4a4a4a。
   - 第 k 张（k 从 0 开始，对应选中顺序的第 k 个）的旋转角度：k==0 为 0 度；k>=1 时正负交替，k 为奇数取负、偶数取正，幅度为 kMultiPreviewAngleStep 乘以 ceil(k/2.0)（度）。围绕画布中心旋转。
   - 绘制顺序：从最后一张画到第一张，使第 0 张（选中顺序第一个）位于最上层。
4. 常量定义在 MetaPanel.cpp 的匿名命名空间，禁止散落魔法数字：
   kMultiPreviewCardSize = 130、kMultiPreviewAngleStep = 6.0、kMultiPreviewMaxCards = 5。
   PanelMediator 传入的 pixmaps 数量不得超过 kMultiPreviewMaxCards，由 MetaPanel 暴露该常量或在 setMultiSelectionPreview 内部截断，二选一，但上限只能有一个定义处。

二、"已选中 N 个项目"文字
1. MetaPanel::setSelectedPaths 多选分支中，m_nameEdit 的内容改为富文本：setHtml，文字仍为"已选中 N 个项目"，其中数字 N 单独用蓝色 #3498db 高亮（该颜色取自项目内既有的 #3498db，如 JustifiedView 分区标题），其余文字颜色保持不变。设置后调用 m_nameEdit->adjustHeight()。
2. 多选时不得显示任何项目的文件名。
3. 禁止修改 m_nameEdit 的 objectName、禁止新增 C++ 内联 setStyleSheet。

三、字段一致才显示
1. PanelMediator.cpp 的 updateMetaPanelFromPanel 新增 paths.size() > 1 的分支。对每一个选中路径，按单选分支同一套取值与回退逻辑取得 rating、color、tags、note、url（先取选中索引的角色值，为空再回退到 MetadataManager::instance().getMeta(...)）。
2. 规则：
   - 星级、颜色、备注、链接：全部选中项目取值相同，才用该值调用 metaPanel 的 setRating/setColor/setNote/setURL（均传 fromUser=false）；只要有任意不同，就传空值（星级 0、其余空字符串）。
   - 标签：取所有选中项目标签的交集，调用 setTags；交集为空则 setTags(空列表)。
3. 该分支还要收集选中顺序下前 kMultiPreviewMaxCards 个项目的缩略图并调用 setMultiSelectionPreview：该项目 HasThumbnailRole 为真时，用 DecorationRole 的 QIcon 取 128x128 的 pixmap（与单选分支一致）；否则也用其 DecorationRole 的 QIcon 取 128x128 的 pixmap。
4. 多选分支同时需调用 setPalettes({})，行为与现有多选一致。
5. MetaPanel::setSelectedPaths 多选分支中原有的 setRating(0,false)、setColor("",false)、setPalettes({}) 保留；它先执行，PanelMediator 之后再覆盖，这个执行顺序不得改变。

【禁止事项】
- 禁止修改任何现有公有/受保护接口签名，MetaPanel.h 只允许新增 setMultiSelectionPreview。
- 禁止改动 TagSelectorOverlay、ContentPanel、CoreEngine、MetadataManager 等其他文件。
- 禁止改变单选与零选的现有行为。
- 禁止用延时、重试、判空跳过等打补丁方式规避问题。
- 禁止顺手重构、重命名、调整无关代码，禁止改动任何现有数值。

【完成后汇报】只列出改动的文件与函数，以及新增的常量；不要解释过程。