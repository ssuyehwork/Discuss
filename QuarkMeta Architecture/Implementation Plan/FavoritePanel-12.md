# Implementation Plan - FavoritePanel-12

This implementation plan expands the `FavoritePanel` icon picker to 50 vector SVG icons arranged in a 5-column grid (5×10 layout).

## 1. Overview
- **50 Vector SVG Icons**: Expand `builtInIcons` array in `FavoritePanel.cpp` from 10 icons to 50 high-fidelity vector icons (`folder_filled`, `category`, `image_filled`, `clock_filled`, `star_filled`, `heart_filled`, `lock_filled`, `book`, `settings_filled`, `globe_filled`, `home_filled`, `tag_filled`, `bookmark_filled`, `code`, `terminal`, `music_filled`, `video_filled`, `camera_filled`, `key`, `shield_filled`, `database`, `hard_drive`, `cloud_filled`, `cpu`, `zap_filled`, `sparkles_filled`, `flag_filled`, `gift_filled`, `award_filled`, `trash_filled`, `user_filled`, `users_filled`, `mail_filled`, `message_filled`, `phone_filled`, `map_pin_filled`, `compass_filled`, `sun_filled`, `moon_filled`, `calendar_filled`, `today_filled`, `search_filled`, `grid_filled`, `layout_filled`, `table_filled`, `bell_filled`, `inbox_filled`, `copy_filled`, `save_filled`, `wand_filled`).
- **5×10 Compact Grid Layout**: Icon picker sub-menu presents a 5-column × 10-row grid of 28x28px compact buttons (`QPushButton`) with clean styling.

## 2. Modified Files List
- `src/ui/FavoritePanel.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/FavoritePanel.cpp`
```diff
<<<<<<< SEARCH
        static const QList<QPair<QString, QString>> builtInIcons = {
            {"默认文件夹", "folder_filled"},
            {"层级分类", "category"},
            {"照片媒体", "image_filled"},
            {"时钟历史", "clock_filled"},
            {"星标收藏", "star_filled"},
            {"爱心常用", "heart_filled"},
            {"加密安全", "lock_filled"},
            {"图书文档", "book"},
            {"配置管理", "settings_filled"},
            {"网络球体", "globe_filled"}
        };
=======
        static const QList<QPair<QString, QString>> builtInIcons = {
            {"默认文件夹", "folder_filled"},
            {"层级分类", "category"},
            {"照片媒体", "image_filled"},
            {"时钟历史", "clock_filled"},
            {"星标收藏", "star_filled"},
            {"爱心常用", "heart_filled"},
            {"加密安全", "lock_filled"},
            {"图书文档", "book"},
            {"配置管理", "settings_filled"},
            {"网络球体", "globe_filled"},
            {"主页主路径", "home_filled"},
            {"标签标记", "tag_filled"},
            {"书签指示", "bookmark_filled"},
            {"代码源码", "code"},
            {"终端命令行", "terminal"},
            {"音频音乐", "music_filled"},
            {"视频影视", "video_filled"},
            {"摄影相机", "camera_filled"},
            {"密钥钥匙", "key"},
            {"盾牌防护", "shield_filled"},
            {"数据库源", "database"},
            {"物理硬盘", "hard_drive"},
            {"云端同步", "cloud_filled"},
            {"处理器芯片", "cpu"},
            {"闪电极速", "zap_filled"},
            {"魔法火花", "sparkles_filled"},
            {"旗帜标记", "flag_filled"},
            {"礼物珍藏", "gift_filled"},
            {"奖星勋章", "award_filled"},
            {"回收废弃", "trash_filled"},
            {"个人专属", "user_filled"},
            {"团队共享", "users_filled"},
            {"邮件通信", "mail_filled"},
            {"消息通知", "message_filled"},
            {"电话联系", "phone_filled"},
            {"地理定位", "map_pin_filled"},
            {"罗盘指南", "compass_filled"},
            {"日光白天", "sun_filled"},
            {"夜间月亮", "moon_filled"},
            {"日历日程", "calendar_filled"},
            {"今日任务", "today_filled"},
            {"搜索检索", "search_filled"},
            {"九宫网格", "grid_filled"},
            {"布局排版", "layout_filled"},
            {"数据表格", "table_filled"},
            {"提醒铃铛", "bell_filled"},
            {"收件收纳", "inbox_filled"},
            {"副本复制", "copy_filled"},
            {"磁盘保存", "save_filled"},
            {"魔棒工具", "wand_filled"}
        };
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Rebuild the project using CMake:
   ```bash
   cmake -B build
   cmake --build build
   ```
2. Test right-click in `FavoritePanel`:
   - Open "切换图标" sub-menu: Verify 50 vector SVG icon buttons render neatly in a 5×10 grid.
