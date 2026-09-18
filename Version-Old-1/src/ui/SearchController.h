#pragma once

#include <QObject>
#include <QLineEdit>
#include <QTimer>
#include <QWidget>
#include <QEvent>

namespace QuarkMeta {

class SearchHistoryPanel;
class ContentPanel;

class SearchController : public QObject {
    Q_OBJECT
public:
    explicit SearchController(QWidget* parent = nullptr);
    ~SearchController() override = default;

    QWidget* toolbarWidget() const { return m_searchContainer; }
    QLineEdit* searchEdit() const { return m_searchEdit; }
    SearchHistoryPanel* historyPanel() const { return m_searchHistoryPanel; }

    void bindContentPanel(ContentPanel* contentPanel);

signals:
    void searchExecuted();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void doSearch(const QString& keyword);

    QWidget* m_searchContainer = nullptr;
    QLineEdit* m_searchEdit = nullptr;
    QTimer* m_searchTimer = nullptr;
    SearchHistoryPanel* m_searchHistoryPanel = nullptr;
    ContentPanel* m_contentPanel = nullptr;
};

} // namespace QuarkMeta
