#include "SearchController.h"
#include "SearchHistoryPanel.h"
#include "ContentPanel.h"
#include "../core/SearchHistoryService.h"
#include "UiHelper.h"
#include "StyleLibrary.h"
#include <QHBoxLayout>

using namespace QuarkMeta::Style;

namespace QuarkMeta {

SearchController::SearchController(QWidget* parent)
    : QObject(parent) {
    m_searchContainer = new QWidget(parent);
    m_searchContainer->setStyleSheet("background: transparent;");
    QHBoxLayout* searchLayout = new QHBoxLayout(m_searchContainer);
    searchLayout->setContentsMargins(0, 0, 0, 0);

    m_searchEdit = new QLineEdit(m_searchContainer);
    m_searchEdit->setPlaceholderText("搜索...");
    m_searchEdit->setFixedSize(230, 32);
    m_searchEdit->addAction(UiHelper::getIcon("search", TextMuted), QLineEdit::LeadingPosition);
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setStyleSheet(QString(
        "QLineEdit { background: %1; border: 1px solid %2;"
        "  border-radius: 6px;"
        "  color: %3; padding-left: 5px; padding-right: 5px; }"
        "QLineEdit:focus { border: 1px solid %4; }"
    ).arg(qssColor(BackgroundDeep)).arg(qssColor(BorderColor)).arg(qssColor(TextMain)).arg(qssColor(PrimaryBlue)));

    searchLayout->addWidget(m_searchEdit);

    m_searchHistoryPanel = new SearchHistoryPanel(parent);
    m_searchHistoryPanel->setCategory("global");
    m_searchHistoryPanel->setHistory(SearchHistoryService::instance().getHistory("global"));

    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(300);
}

void SearchController::bindContentPanel(ContentPanel* contentPanel) {
    m_contentPanel = contentPanel;
    if (!m_contentPanel) return;

    connect(m_searchEdit, &QLineEdit::returnPressed, this, [this]() {
        doSearch(m_searchEdit->text().trimmed());
    });

    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (text.isEmpty()) {
            m_searchTimer->stop();
            doSearch("");
        } else {
            m_searchTimer->start();
        }
    });

    connect(m_searchTimer, &QTimer::timeout, this, [this]() {
        doSearch(m_searchEdit->text().trimmed());
    });

    m_searchEdit->installEventFilter(this);

    connect(m_searchHistoryPanel, &SearchHistoryPanel::historyItemClicked, this, [this](const QString& keyword) {
        m_searchEdit->setText(keyword);
        doSearch(keyword);
    });
}

void SearchController::doSearch(const QString& keyword) {
    if (!m_contentPanel) return;
    m_contentPanel->search(keyword);
    if (!keyword.isEmpty()) {
        SearchHistoryService::instance().appendSearch("global", keyword);
        m_searchHistoryPanel->setHistory(SearchHistoryService::instance().getHistory("global"));
    }
    m_searchHistoryPanel->hide();
    emit searchExecuted();
}

bool SearchController::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::MouseButtonDblClick && watched == m_searchEdit) {
        auto history = SearchHistoryService::instance().getHistory("global");
        if (!history.isEmpty()) {
            m_searchHistoryPanel->setHistory(history);
            m_searchHistoryPanel->showBelow(m_searchEdit);
        }
        return true;
    }
    return QObject::eventFilter(watched, event);
}

} // namespace QuarkMeta
