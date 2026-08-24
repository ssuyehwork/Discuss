#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QScrollArea>
#include <QPushButton>
#include <QLineEdit>
#include <QSlider>
#include <QMap>
#include <QStringList>
#include "ScanStats.h"
#include "MetaPanel.h" // 引用 FlowLayout

namespace QuarkMeta {

class SearchHistoryPanel;

// ─── 自定义勾选框 ──────────────────────────────────────────────────
class StyledCheckBox : public QCheckBox {
    Q_OBJECT
public:
    explicit StyledCheckBox(QWidget* parent = nullptr);
protected:
    void paintEvent(QPaintEvent* event) override;
};

// ─── 可整行点击的行控件 ────────────────────────────────────────────
class ClickableRow : public QWidget {
    Q_OBJECT
public:
    explicit ClickableRow(StyledCheckBox* cb, QWidget* parent = nullptr);
protected:
    void mousePressEvent(QMouseEvent* e) override;
    void enterEvent(QEnterEvent* e) override;
    void leaveEvent(QEvent* e) override;
private:
    StyledCheckBox* m_cb;
};

struct FilterState {
    QList<int>   ratings;
    QStringList  colors;
    QString      keyword;
    QStringList  types;
    QStringList  createDates;   // "YYYY-MM-DD"
    QStringList  modifyDates;

    // 2026-07-xx 按照 Plan-30：链接、备注及大小筛选
    enum Presence { All, Yes, No };
    Presence linkPresence = All;
    Presence notePresence = All;

    enum AspectRatio { AspectAny, Horizontal, Vertical, Square, Ratio169 };
    AspectRatio ratio = AspectAny;

    long long minSize = -1; // 字节单位，-1 表示不限制
    long long maxSize = -1;

    QString typeFilterText;
    QString createDateFilterText;
    QString modifyDateFilterText;

    bool showFolders = true; // 2026-07-xx 按照 Plan-73：显示/隐藏文件夹
    bool showFiles = true;   // 2026-07-xx 按照 Plan-73：显示/隐藏文件
    bool showHidden = false; // 默认不显示操作系统隐藏属性项目

    enum DuplicatePresence { DupAll, DuplicateOnly, UniqueOnly };
    DuplicatePresence duplicatePresence = DupAll;

    bool noThumbnailOnly = false;

    bool isEmpty() const {
        return ratings.isEmpty() && colors.isEmpty() && keyword.isEmpty() && types.isEmpty() &&
               createDates.isEmpty() && modifyDates.isEmpty() &&
               linkPresence == All && notePresence == All && ratio == AspectAny &&
               minSize == -1 && maxSize == -1 &&
               typeFilterText.trimmed().isEmpty() && createDateFilterText.trimmed().isEmpty() &&
               modifyDateFilterText.trimmed().isEmpty() && duplicatePresence == DupAll &&
               !noThumbnailOnly;
    }
};

/**
 * @brief 筛选面板 — 动态 Adobe Bridge 风格
 *
 * 由 MainWindow 在目录切换后调用 populate() 驱动数据填充。
 * 每行整体可点击（不需要对准复选框）。
 */
class FilterPanel : public QFrame {
    Q_OBJECT

public:
    explicit FilterPanel(QWidget* parent = nullptr);
    ~FilterPanel() override = default;


    void populateStats(const QuarkMeta::ScanStats& stats);
    void populate(const QuarkMeta::ScanStats& stats) { populateStats(stats); }
    void populate(
        const QMap<int, int>&        ratingCounts,
        const QMap<QString, int>&    colorCounts,
        const QMap<QString, int>&    typeCounts,
        const QMap<QString, int>&    createDateCounts,
        const QMap<QString, int>&    modifyDateCounts,
        int                          emptyFolderCount
    );

    FilterState currentFilter() const { return m_filter; }

    /**
     * @brief 增量同步 UI 状态，避免 rebuildGroups 导致的暴力重构
     */
    void syncUIFromFilterState();

    /**
     * @brief 外部驱动颜色选择（逻辑中枢：同步最近筛选与过滤状态）
     */
    void selectColor(const QColor& color);

    /**
     * @brief 更新数据源感知状态
     * 2026-07-xx 按照 Plan-118：物理源下隐藏逻辑标签等筛选项
     */
    void setMirrorSource(bool isMirror);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    void filterChanged(const FilterState& state);

public slots:
    void clearAllFilters(bool force = false);

private:
    void rebuildGroups();
    void updateHeaderStatus();
    void rebuildDateCheckboxes(bool isCreateDate, bool descending); // 2026-07-xx Plan-92: 日期重排支持

    // 2026-05-17 根因修复：增加 outHdrLayout 参数，让调用方直接往标题行布局追加按钮
    // 彻底替代绝对定位方案，消除非布局子控件撑高 wrapper 导致的留白
    QWidget*   buildGroup(const QString& title, QVBoxLayout*& outContentLayout,
                          QHBoxLayout** outHdrLayout = nullptr);
    QCheckBox* addFilterRow(QVBoxLayout* layout, const QString& label,
                            int count, const QColor& dotColor = Qt::transparent);

    static QMap<QString, QColor> s_colorMap();

    FilterState m_filter;

    QuarkMeta::ScanStats m_currentStats;

    QMap<int, int>      m_ratingCounts;
    QMap<QString, int>  m_colorCounts;
    QMap<QString, int>  m_typeCounts;
    QMap<QString, int>  m_createDateCounts;
    QMap<QString, int>  m_modifyDateCounts;
    int                 m_emptyFolderCount = 0;

    bool m_createDateDesc = true; // 2026-07-xx Plan-92: 日期降序标记
    bool m_modifyDateDesc = true;

    QVBoxLayout*  m_mainLayout      = nullptr;
    QScrollArea*  m_scrollArea      = nullptr;
    QWidget*      m_container       = nullptr;
    QVBoxLayout*  m_containerLayout = nullptr;
    QPushButton*  m_btnPin          = nullptr; // 2026-06-23 按照用户要求：新增筛选器锁定按钮
    QPushButton*  m_btnClearAll     = nullptr;
    QPushButton*  m_btnToggleGroups = nullptr; // 2026-07-xx 按照 Plan-77：全局折叠/展开按钮
    QLabel*       m_iconLabel       = nullptr;
    QLabel*       m_titleLabel      = nullptr;

    QList<QPushButton*> m_groupHeaders; // 跟踪所有分组标题以支持全局控制

    // 2026-07-xx 按照 Plan-118：数据源感知分组引用
    QWidget* m_groupRating = nullptr;
    QWidget* m_groupColor = nullptr;
    QWidget* m_groupLink = nullptr;
    QWidget* m_groupNote = nullptr;
    QWidget* m_groupRatio = nullptr;
    QWidget* m_groupDuplicate = nullptr;


    QLineEdit*    m_editType        = nullptr;
    QLineEdit*    m_editCreateDate  = nullptr;
    QLineEdit*    m_editModifyDate  = nullptr;
    QVBoxLayout*  m_createDateLayout = nullptr; // 2026-07-xx Plan-92: 日期布局指针
    QVBoxLayout*  m_modifyDateLayout = nullptr;

    bool          m_isFilterPinned = false;    // 2026-06-23 按照用户要求：筛选器锁定状态

    SearchHistoryPanel* m_historyPanel = nullptr;
    
    // 辅助方法：处理历史记录
    void saveFilterHistory(const QString& key, const QString& text);
    QStringList getFilterHistory(const QString& key) const;

private slots:
    void onToggleAllGroupsClicked();
};

} // namespace QuarkMeta
