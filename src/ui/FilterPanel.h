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
#include "MetaPanel.h"
#include "components/StyledCheckBox.h"
#include "components/ClickableRow.h"

namespace QuarkMeta {

class SearchHistoryPanel;

struct FilterState {
    QList<int>   ratings;
    QStringList  colors;
    QString      keyword;
    QStringList  types;
    QStringList  createDates;   // "YYYY-MM-DD"
    QStringList  modifyDates;

    enum Presence { All, Yes, No };
    Presence linkPresence = All;
    Presence notePresence = All;
    Presence tagPresence = All;

    enum AspectRatio { AspectAny, Horizontal, Vertical, Square, Ratio169 };
    AspectRatio ratio = AspectAny;

    long long minSize = -1;
    long long maxSize = -1;

    QString typeFilterText;
    QString createDateFilterText;
    QString modifyDateFilterText;

    bool showFolders = true;
    bool showFiles = true;
    bool showHidden = false;

    enum DuplicatePresence { DupAll, DuplicateOnly, UniqueOnly };
    DuplicatePresence duplicatePresence = DupAll;

    enum ThumbnailPresence { ThumbAll, HasThumbnail, NoThumbnail };
    ThumbnailPresence thumbnailPresence = ThumbAll;

    bool isEmpty() const {
        return ratings.isEmpty() && colors.isEmpty() && keyword.isEmpty() && types.isEmpty() &&
               createDates.isEmpty() && modifyDates.isEmpty() &&
               linkPresence == All && notePresence == All && tagPresence == All && ratio == AspectAny &&
               minSize == -1 && maxSize == -1 &&
               typeFilterText.trimmed().isEmpty() && createDateFilterText.trimmed().isEmpty() &&
               modifyDateFilterText.trimmed().isEmpty() && duplicatePresence == DupAll &&
               thumbnailPresence == ThumbAll;
    }
};

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

    void syncUIFromFilterState();
    void selectColor(const QColor& color);
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
    void rebuildDateCheckboxes(bool isCreateDate, bool descending);

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

    bool m_createDateDesc = true;
    bool m_modifyDateDesc = true;

    QVBoxLayout*  m_mainLayout      = nullptr;
    QScrollArea*  m_scrollArea      = nullptr;
    QWidget*      m_container       = nullptr;
    QVBoxLayout*  m_containerLayout = nullptr;
    QPushButton*  m_btnPin          = nullptr;
    QPushButton*  m_btnClearAll     = nullptr;
    QPushButton*  m_btnToggleGroups = nullptr;
    QLabel*       m_iconLabel       = nullptr;
    QLabel*       m_titleLabel      = nullptr;

    QList<QPushButton*> m_groupHeaders;

    QWidget* m_groupRating = nullptr;
    QWidget* m_groupColor = nullptr;
    QWidget* m_groupLink = nullptr;
    QWidget* m_groupNote = nullptr;
    QWidget* m_groupTag = nullptr;
    QWidget* m_groupRatio = nullptr;
    QWidget* m_groupDuplicate = nullptr;

    QLineEdit*    m_editType        = nullptr;
    QLineEdit*    m_editCreateDate  = nullptr;
    QLineEdit*    m_editModifyDate  = nullptr;
    QVBoxLayout*  m_createDateLayout = nullptr;
    QVBoxLayout*  m_modifyDateLayout = nullptr;

    bool          m_isFilterPinned = false;

    SearchHistoryPanel* m_historyPanel = nullptr;
    
    void saveFilterHistory(const QString& key, const QString& text);
    QStringList getFilterHistory(const QString& key) const;

private slots:
    void onToggleAllGroupsClicked();
};

} // namespace QuarkMeta
