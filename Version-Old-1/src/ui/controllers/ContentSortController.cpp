#include "ContentSortController.h"
#include "../models/FilterProxyModel.h"
#include "../../core/AppConfig.h"

namespace QuarkMeta {

ContentSortController::ContentSortController(QObject* parent)
    : QObject(parent) {
    loadFromConfig();
}

void ContentSortController::loadFromConfig() {
    int savedType = AppConfig::instance().getValue("ContentPanel/RightClickSortType", static_cast<int>(SortType::SortByName)).toInt();
    int savedOrder = AppConfig::instance().getValue("ContentPanel/RightClickSortOrder", static_cast<int>(Qt::AscendingOrder)).toInt();

    m_sortType = static_cast<SortType>(savedType);
    m_sortOrder = static_cast<Qt::SortOrder>(savedOrder);
}

void ContentSortController::saveToConfig() {
    AppConfig::instance().setValue("ContentPanel/RightClickSortType", static_cast<int>(m_sortType));
    AppConfig::instance().setValue("ContentPanel/RightClickSortOrder", static_cast<int>(m_sortOrder));
    AppConfig::instance().sync();
}

void ContentSortController::setSortType(SortType type) {
    if (m_sortType != type) {
        m_sortType = type;
        saveToConfig();
        emit sortCriteriaChanged(m_sortType, m_sortOrder);
    }
}

void ContentSortController::setSortOrder(Qt::SortOrder order) {
    if (m_sortOrder != order) {
        m_sortOrder = order;
        saveToConfig();
        emit sortCriteriaChanged(m_sortType, m_sortOrder);
    }
}

void ContentSortController::setSortCriteria(SortType type, Qt::SortOrder order) {
    bool changed = (m_sortType != type || m_sortOrder != order);
    m_sortType = type;
    m_sortOrder = order;
    if (changed) {
        saveToConfig();
        emit sortCriteriaChanged(m_sortType, m_sortOrder);
    }
}

// 🚀【打通模型通信】：应用排序时必须同步将 m_sortType 灌入 FilterProxyModel！
void ContentSortController::applySortToModel(QSortFilterProxyModel* proxyModel) {
    if (!proxyModel) return;
    if (auto* proxy = qobject_cast<FilterProxyModel*>(proxyModel)) {
        proxy->setSortType(static_cast<int>(m_sortType));
        proxy->setSortOrder(m_sortOrder);
    }
    proxyModel->sort(0, m_sortOrder);
}

} // namespace QuarkMeta