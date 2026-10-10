# Implementation Plan - AddBuiltInIcons.md

## Overview
Add 10 requested icons into the icon picker grid in `ContextMenuFactory::buildIconPickerMenu`.

## Icons List to Add
1. `{"全部数据", "all_data"}`
2. `{"托管文件夹", "folder_managed"}`
3. `{"托管文件", "file_managed"}`
4. `{"ZIP压缩包", "zip"}`
5. `{"下载图标", "download-svgrepo-com"}`
6. `{"云端下载", "cloud_download"}`
7. `{"Word文档", "file_word"}`
8. `{"磁盘保存", "save_filled"}`
9. `{"棕榈树", "palm_tree"}`
10. `{"代码文件夹", "folder-code-svgrepo-com"}` (mapped from user's `older-code-svgrepo-com.svg`)

## Modified Files List
- `src/ui/controllers/ContextMenuFactory.cpp`

## Detailed Changes

### `src/ui/controllers/ContextMenuFactory.cpp`

```
<<<<<<< SEARCH
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
        {"OneNote笔记", "onenote"}, {"下载中心", "download"}, {"全部数据", "all_data"},
        {"托管文件夹", "folder_managed"}, {"托管文件", "file_managed"}, {"ZIP压缩包", "zip"},
        {"下载图标", "download-svgrepo-com"}, {"云端下载", "cloud_download"}, {"Word文档", "file_word"},
        {"棕榈树", "palm_tree"}, {"代码文件夹", "folder-code-svgrepo-com"}
    };
>>>>>>> REPLACE
```

## Build & Verification
1. Build QuarkMeta.
2. Open icon picker in FavoritePanel / LibraryPanel right-click menu.
3. Verify that all 10 new icons appear in the picker menu.
