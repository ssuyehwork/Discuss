# Implementation Plan - RemoveUnwantedBuiltInIcons.md

## Overview
Remove 11 specified built-in icons from the icon picker menu in `ContextMenuFactory::buildIconPickerMenu`.

## Modified Icons List (11 items to remove)
1. `{"加密安全", "lock_filled"}` (Mark 7)
2. `{"图书文档", "book"}` (Mark 6)
3. `{"配置管理", "settings_filled"}` (Mark 5)
4. `{"音频音乐", "music_filled"}` (Mark 4)
5. `{"消息通知", "message_filled"}` (Mark 11)
6. `{"地理定位", "map_pin_filled"}` (Mark 10)
7. `{"日光白天", "sun_filled"}` (Mark 8)
8. `{"夜间月亮", "moon_filled"}` (Mark 9)
9. `{"九宫网格", "grid_filled"}` (Mark 3)
10. `{"布局排版", "layout_filled"}` (Mark 2)
11. `{"魔棒工具", "wand_filled"}` (Mark 1)

## Modified Files List
- `src/ui/controllers/ContextMenuFactory.cpp`

## Detailed Changes

### `src/ui/controllers/ContextMenuFactory.cpp`

```
<<<<<<< SEARCH
    static const QList<QPair<QString, QString>> builtInIcons = {
        {"默认文件夹", "folder_filled"}, {"照片媒体", "image_filled"}, {"相册图片", "image_picture"},
        {"时钟历史", "clock_filled"}, {"星标收藏", "star_filled"}, {"实心星标", "star_001"},
        {"空心星标", "star_002"}, {"爱心常用", "heart_filled"}, {"加密安全", "lock_filled"},
        {"图书文档", "book"}, {"附加文档", "document_attach"}, {"配置管理", "settings_filled"},
        {"网络球体", "globe_filled"}, {"主页主路径", "home_filled"}, {"标签标记", "tag_filled"},
        {"书签指示", "bookmark_filled"}, {"音频音乐", "music_filled"}, {"视频影视", "video_filled"},
        {"摄影相机", "camera_filled"}, {"盾牌防护", "shield_filled"}, {"物理硬盘", "hard_drive"},
        {"云端同步", "cloud_filled"}, {"闪电极速", "zap_filled"}, {"魔法火花", "sparkles_filled"},
        {"旗帜标记", "flag_filled"}, {"旗帜标示", "flag"}, {"礼物珍藏", "gift_filled"},
        {"奖星勋章", "award_filled"}, {"回收废弃", "trash_filled"}, {"邮件通信", "mail_filled"},
        {"消息通知", "message_filled"}, {"电话联系", "phone_filled"}, {"地理定位", "map_pin_filled"},
        {"日光白天", "sun_filled"}, {"夜间月亮", "moon_filled"}, {"日历日程", "calendar_filled"},
        {"今日任务", "today_filled"}, {"九宫网格", "grid_filled"}, {"布局排版", "layout_filled"},
        {"数据表格", "table_filled"}, {"磁盘保存", "save_filled"}, {"魔棒工具", "wand_filled"},
        {"附件剪辑", "paperclip"}, {"归档文件", "archive"},
        {"OneNote笔记", "onenote"}, {"下载中心", "download"}
    };
=======
    static const QList<QPair<QString, QString>> builtInIcons = {
        {"默认文件夹", "folder_filled"}, {"照片媒体", "image_filled"}, {"相册图片", "image_picture"},
        {"时钟历史", "clock_filled"}, {"星标收藏", "star_filled"}, {"实心星标", "star_001"},
        {"空心星标", "star_002"}, {"爱心常用", "heart_filled"}, {"附加文档", "document_attach"},
        {"网络球体", "globe_filled"}, {"主页主路径", "home_filled"}, {"标签标记", "tag_filled"},
        {"书签指示", "bookmark_filled"}, {"视频影视", "video_filled"}, {"摄影相机", "camera_filled"},
        {"盾牌防护", "shield_filled"}, {"物理硬盘", "hard_drive"}, {"云端同步", "cloud_filled"},
        {"闪电极速", "zap_filled"}, {"魔法火花", "sparkles_filled"}, {"旗帜标记", "flag_filled"},
        {"旗帜标示", "flag"}, {"礼物珍藏", "gift_filled"}, {"奖星勋章", "award_filled"},
        {"回收废弃", "trash_filled"}, {"邮件通信", "mail_filled"}, {"电话联系", "phone_filled"},
        {"日历日程", "calendar_filled"}, {"今日任务", "today_filled"}, {"数据表格", "table_filled"},
        {"磁盘保存", "save_filled"}, {"附件剪辑", "paperclip"}, {"归档文件", "archive"},
        {"OneNote笔记", "onenote"}, {"下载中心", "download"}
    };
>>>>>>> REPLACE
```

## Build & Verification
1. Build QuarkMeta.
2. Open icon picker in FavoritePanel / LibraryPanel right-click menu.
3. Verify that the 11 marked icons are no longer present in the grid picker.
