#include "QuickShortcutHandler.h"

#include "../QuickWindow.h"
#include "QuickMediator.h"
#include "../SearchLineEdit.h"
#include "../CleanListView.h"
#include "../ClickableLineEdit.h"
#include "../DropTreeView.h"
#include "../QuickFilterPanel.h"
#include "../TitleEditorDialog.h"
#include "../FramelessDialog.h"
#include "../ToolTipOverlay.h"
#include "../../models/NoteModel.h"
#include "../../models/CategoryModel.h"
#include "../../core/DatabaseManager.h"

#include <QAbstractItemModel>
#include <QCursor>
#include <QDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPointer>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QSet>
#include <QTimer>
#include <QWheelEvent>

namespace RapidNotes {

QuickShortcutHandler::QuickShortcutHandler(QuickWindow* window, QObject* parent)
    : QObject(parent),
      m_window(window) {
}

bool QuickShortcutHandler::handleEventFilter(QObject* watched, QEvent* event) {
    if (!m_window || !event || m_window->isLocked()) {
        return false;
    }

    switch (event->type()) {
    case QEvent::ShortcutOverride:
        return handleShortcutOverride(static_cast<QKeyEvent*>(event));
    case QEvent::KeyPress:
        return handleKeyPress(watched, static_cast<QKeyEvent*>(event));
    case QEvent::Wheel:
        return handleWheelScroll(watched, static_cast<QWheelEvent*>(event));
    default:
        return false;
    }
}

bool QuickShortcutHandler::handleShortcutOverride(QKeyEvent* keyEvent) {
    if (!keyEvent) {
        return false;
    }

    const auto modifiers = keyEvent->modifiers();
    if (keyEvent->key() == Qt::Key_S &&
        (modifiers & Qt::ControlModifier) &&
        ((modifiers & Qt::AltModifier) || !(modifiers & Qt::ShiftModifier))) {
        keyEvent->accept();
        return true;
    }

    return false;
}

bool QuickShortcutHandler::handleKeyPress(QObject* watched, QKeyEvent* keyEvent) {
    if (!m_window || !keyEvent) {
        return false;
    }

    const int key = keyEvent->key();
    const auto modifiers = keyEvent->modifiers();

    // 全局面板与筛选快捷键。
    if (key == Qt::Key_E && (modifiers & Qt::AltModifier)) {
        m_window->toggleFilter();
        return true;
    }
    if (key == Qt::Key_R && (modifiers & Qt::AltModifier)) {
        m_window->toggleAllPanels();
        return true;
    }
    if (key == Qt::Key_G && modifiers == Qt::ControlModifier) {
        if (m_window->m_filterWrapper && m_window->m_filterWrapper->isVisible() &&
            m_window->m_filterPanel) {
            m_window->m_filterPanel->toggleAllGroups();
        }
        return true;
    }
    if (key == Qt::Key_G &&
        modifiers == (Qt::ControlModifier | Qt::ShiftModifier)) {
        if (m_window->m_filterWrapper && m_window->m_filterWrapper->isVisible() &&
            m_window->m_filterPanel) {
            m_window->m_filterPanel->resetFilters();
        }
        return true;
    }

    // 分类锁定与锁定分类显隐：数据库写入只经由 QuickMediator。
    if (key == Qt::Key_S && (modifiers & Qt::ControlModifier)) {
        if (modifiers & Qt::AltModifier) {
            if (m_window->m_mediator) {
                const bool isHidden =
                    m_window->m_mediator->requestToggleLockedCategoriesVisibility();

                if (isHidden &&
                    m_window->m_currentFilterType == "category" &&
                    m_window->m_currentFilterValue != -1) {
                    const int categoryId = m_window->m_currentFilterValue.toInt();
                    auto& db = DatabaseManager::instance();
                    if (db.isCategoryLocked(categoryId) ||
                        !db.getCategoryPresetTags(categoryId).isNull()) {
                        m_window->m_currentFilterType = "all";
                        m_window->m_currentFilterValue = -1;
                    }
                }

                ToolTipOverlay::instance()->showText(
                    QCursor::pos(),
                    isHidden
                        ? "<b style='color: #e67e22;'>[OK] 已隐藏加锁分类并强制重锁</b>"
                        : "<b style='color: #2ecc71;'>[OK] 已显示所有分类并强制重锁</b>");
            }
            return true;
        }

        if (!(modifiers & Qt::ShiftModifier)) {
            int categoryId = -1;
            if (m_window->m_partitionTree) {
                const QModelIndex sidebarIndex =
                    m_window->m_partitionTree->currentIndex();
                if (sidebarIndex.isValid() &&
                    sidebarIndex.data(CategoryModel::TypeRole).toString() == "category") {
                    categoryId = sidebarIndex.data(CategoryModel::IdRole).toInt();
                }
            }

            if (categoryId == -1 &&
                m_window->m_currentFilterType == "category" &&
                m_window->m_currentFilterValue != -1) {
                categoryId = m_window->m_currentFilterValue.toInt();
            }

            if (categoryId != -1) {
                if (m_window->m_mediator) {
                    m_window->m_mediator->requestLockCategory(categoryId);
                }
                if (m_window->m_currentFilterType == "category" &&
                    m_window->m_currentFilterValue.toInt() == categoryId) {
                    m_window->m_currentFilterType = "all";
                    m_window->m_currentFilterValue = -1;
                }
                ToolTipOverlay::instance()->showText(
                    QCursor::pos(),
                    "<b style='color: #f39c12;'>[OK] 分类已物理锁定</b>");
                return true;
            }
        }
    }

    if (key == Qt::Key_Escape) {
        return handleEscClear();
    }

    // 输入框中的上下键移动光标到文本首尾。
    if (watched == m_window->m_searchEdit ||
        watched == m_window->m_catSearchEdit ||
        watched == m_window->m_tagEdit ||
        watched == m_window->m_pageInput) {
        if (auto* edit = qobject_cast<QLineEdit*>(watched)) {
            if (key == Qt::Key_Up) {
                edit->setCursorPosition(0);
                return true;
            }
            if (key == Qt::Key_Down) {
                edit->setCursorPosition(edit->text().length());
                return true;
            }
        }
    }

    if (watched == m_window->m_partitionTree ||
        watched == m_window->m_systemTree) {
        return handleTreeKeyPress(watched, keyEvent);
    }

    if (watched == m_window->m_listView) {
        return handleListKeyPress(keyEvent);
    }

    return false;
}

bool QuickShortcutHandler::handleTreeKeyPress(QObject* watched,
                                               QKeyEvent* keyEvent) {
    if (!m_window || !keyEvent) {
        return false;
    }

    const int key = keyEvent->key();
    const auto modifiers = keyEvent->modifiers();

    if (key == Qt::Key_QuoteLeft || key == Qt::Key_Backspace) {
        return switchToAllData();
    }

    if (key == Qt::Key_F2 && watched == m_window->m_partitionTree) {
        const QModelIndex current = m_window->m_partitionTree->currentIndex();
        if (current.isValid() &&
            current.data(CategoryModel::TypeRole).toString() == "category") {
            m_window->m_partitionTree->edit(current);
        }
        return true;
    }

    if (key == Qt::Key_Tab && isSidebarVisible()) {
        if (m_window->m_listView) {
            m_window->m_listView->setFocus();
            QAbstractItemModel* model = m_window->m_listView->model();
            if (model && !m_window->m_listView->currentIndex().isValid() &&
                model->rowCount() > 0) {
                m_window->m_listView->setCurrentIndex(model->index(0, 0));
            }
        }
        return true;
    }

    if (key == Qt::Key_Delete) {
        if (watched == m_window->m_partitionTree &&
            m_window->m_partitionTree->selectionModel()) {
            const QModelIndexList selected =
                m_window->m_partitionTree->selectionModel()->selectedIndexes();
            QSet<int> uniqueIds;
            for (const QModelIndex& index : selected) {
                if (index.data(CategoryModel::TypeRole).toString() == "category") {
                    uniqueIds.insert(index.data(CategoryModel::IdRole).toInt());
                }
            }

            if (!uniqueIds.isEmpty()) {
                const int count = uniqueIds.size();
                const QString message = count > 1
                    ? QString("确定要删除选中的 %1 个分类及其下所有内容吗？").arg(count)
                    : QString("确定要删除选中的分类及其下所有内容吗？");
                FramelessMessageBox dialog("确认删除", message, m_window);
                if (dialog.exec() == QDialog::Accepted && m_window->m_mediator) {
                    QList<int> ids;
                    ids.reserve(uniqueIds.size());
                    for (int id : uniqueIds) {
                        ids.append(id);
                    }
                    m_window->m_mediator->requestDeleteCategories(ids, false);
                }
            }
        } else if (watched == m_window->m_systemTree &&
                   m_window->m_systemTree) {
            const QModelIndex current = m_window->m_systemTree->currentIndex();
            if (current.isValid() &&
                current.data(CategoryModel::TypeRole).toString() == "trash") {
                FramelessMessageBox dialog(
                    "确认清空",
                    "确定要永久删除回收站中的所有内容吗？\n(此操作不可逆)",
                    m_window);
                if (dialog.exec() == QDialog::Accepted && m_window->m_mediator) {
                    m_window->m_mediator->requestEmptyTrash();
                }
            }
        }
        return true;
    }

    if ((key == Qt::Key_Up || key == Qt::Key_Down) &&
        (modifiers & Qt::AltModifier)) {
        DropTreeView* tree = watched == m_window->m_partitionTree
            ? m_window->m_partitionTree
            : m_window->m_systemTree;
        if (!tree) {
            return false;
        }

        const QModelIndex current = tree->currentIndex();
        if (current.isValid() &&
            current.data(CategoryModel::TypeRole).toString() == "category") {
            const int categoryId = current.data(CategoryModel::IdRole).toInt();
            const DatabaseManager::MoveDirection direction =
                key == Qt::Key_Up
                    ? ((modifiers & Qt::ShiftModifier)
                           ? DatabaseManager::Top
                           : DatabaseManager::Up)
                    : ((modifiers & Qt::ShiftModifier)
                           ? DatabaseManager::Bottom
                           : DatabaseManager::Down);

            if (m_window->m_mediator &&
                m_window->m_mediator->requestMoveCategory(categoryId, direction)) {
                return true;
            }
        }
    }

    return false;
}

bool QuickShortcutHandler::handleListKeyPress(QKeyEvent* keyEvent) {
    if (!m_window || !keyEvent) {
        return false;
    }

    const int key = keyEvent->key();
    const auto modifiers = keyEvent->modifiers();

    if (key == Qt::Key_C && (modifiers & Qt::ControlModifier)) {
        m_window->doExtractContent();
        return true;
    }

    if (key == Qt::Key_QuoteLeft || key == Qt::Key_Backspace) {
        return switchToAllData();
    }

    if ((key == Qt::Key_Up || key == Qt::Key_Down) &&
        (modifiers & Qt::AltModifier)) {
        const DatabaseManager::MoveDirection direction =
            (modifiers & (Qt::ShiftModifier | Qt::ControlModifier))
                ? (key == Qt::Key_Up ? DatabaseManager::Top
                                     : DatabaseManager::Bottom)
                : (key == Qt::Key_Up ? DatabaseManager::Up
                                     : DatabaseManager::Down);
        const QModelIndex current = m_window->m_listView->currentIndex();
        if (current.isValid() && m_window->m_mediator) {
            const int noteId = current.data(NoteModel::IdRole).toInt();
            m_window->m_mediator->requestMoveNote(
                noteId,
                direction,
                m_window->m_currentFilterType,
                m_window->m_currentFilterValue);
        }
        return true;
    }

    if (key == Qt::Key_F2) {
        const QModelIndex current = m_window->m_listView->currentIndex();
        if (current.isValid()) {
            const QString oldTitle = current.data(NoteModel::TitleRole).toString();
            const int noteId = current.data(NoteModel::IdRole).toInt();
            TitleEditorDialog dialog(oldTitle, m_window);
            if (dialog.exec() == QDialog::Accepted && m_window->m_mediator) {
                const QString newTitle = dialog.getText().trimmed();
                if (!newTitle.isEmpty() && newTitle != oldTitle) {
                    m_window->m_mediator->requestRenameNote(noteId, newTitle);
                }
            }
        }
        return true;
    }

    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        m_window->activateNote(m_window->m_listView->currentIndex());
        return true;
    }

    if (key == Qt::Key_Tab && isSidebarVisible()) {
        if (m_window->m_partitionTree && m_window->m_partitionTree->isVisible()) {
            m_window->m_partitionTree->setFocus();
            if (!m_window->m_partitionTree->currentIndex().isValid() &&
                m_window->m_partitionProxyModel &&
                m_window->m_partitionProxyModel->rowCount() > 0) {
                m_window->m_partitionTree->setCurrentIndex(
                    m_window->m_partitionProxyModel->index(0, 0));
            }
        } else if (m_window->m_systemTree &&
                   m_window->m_systemTree->isVisible()) {
            m_window->m_systemTree->setFocus();
        }
        return true;
    }

    if (!(modifiers & (Qt::ControlModifier |
                       Qt::AltModifier |
                       Qt::ShiftModifier)) &&
        key >= Qt::Key_1 && key <= Qt::Key_4) {
        return handleListNumericNavigation(key);
    }

    return false;
}

bool QuickShortcutHandler::switchToAllData() {
    if (!m_window) {
        return false;
    }

    m_window->m_currentFilterType = "all";
    m_window->m_currentFilterValue = -1;
    m_window->m_currentPage = 1;

    if (m_window->m_systemTree) {
        if (m_window->m_systemTree->selectionModel()) {
            m_window->m_systemTree->selectionModel()->clearSelection();
        }
        m_window->m_systemTree->setCurrentIndex(QModelIndex());
    }
    if (m_window->m_partitionTree) {
        if (m_window->m_partitionTree->selectionModel()) {
            m_window->m_partitionTree->selectionModel()->clearSelection();
        }
        m_window->m_partitionTree->setCurrentIndex(QModelIndex());
    }

    m_window->m_currentCategoryColor = "#4a90e2";
    m_window->updatePartitionStatus(QString());
    m_window->applyListTheme(QString());
    m_window->refreshData();
    ToolTipOverlay::instance()->showText(QCursor::pos(), "[OK] 已切换至全部数据");
    return true;
}

bool QuickShortcutHandler::handleEscClear() {
    if (!m_window) {
        return false;
    }

    const bool searchHasText =
        m_window->m_searchEdit && !m_window->m_searchEdit->text().isEmpty();
    const bool tagHasText =
        m_window->m_tagEdit && !m_window->m_tagEdit->text().isEmpty();
    const bool categorySearchHasText =
        m_window->m_catSearchEdit && !m_window->m_catSearchEdit->text().isEmpty();

    if (searchHasText || tagHasText || categorySearchHasText) {
        if (searchHasText) {
            m_window->m_searchEdit->clear();
        }
        if (tagHasText) {
            m_window->m_tagEdit->clear();
        }
        if (categorySearchHasText) {
            m_window->m_catSearchEdit->clear();
        }
        return true;
    }

    if (m_window->m_listView && m_window->m_listView->hasFocus()) {
        m_window->hide();
    } else if (m_window->m_listView) {
        m_window->m_listView->setFocus();
    } else {
        m_window->hide();
    }
    return true;
}

bool QuickShortcutHandler::handleListNumericNavigation(int key) {
    if (!m_window || !m_window->m_model || !m_window->m_listView ||
        m_window->m_model->rowCount() <= 0) {
        return false;
    }

    const QModelIndex current = m_window->m_listView->currentIndex();
    const int rowCount = m_window->m_model->rowCount();
    const int totalPages = qMax(1, m_window->m_totalPages);

    if (key == Qt::Key_1) {
        if (current.isValid() && current.row() == 0) {
            m_window->m_currentPage =
                m_window->m_currentPage > 1
                    ? m_window->m_currentPage - 1
                    : totalPages;
            m_window->refreshData();
            ToolTipOverlay::instance()->showText(
                QCursor::pos(),
                QString("<b style='color: #3498db;'>[上一页] 第 %1 / %2 页</b>")
                    .arg(m_window->m_currentPage)
                    .arg(m_window->m_totalPages),
                2000);
            const int newRowCount = m_window->m_model->rowCount();
            if (newRowCount > 0) {
                const QModelIndex last =
                    m_window->m_model->index(newRowCount - 1, 0);
                m_window->m_listView->setCurrentIndex(last);
                m_window->m_listView->scrollTo(last);
            }
        } else {
            const QModelIndex first = m_window->m_model->index(0, 0);
            m_window->m_listView->setCurrentIndex(first);
            m_window->m_listView->scrollTo(first);
        }
        return true;
    }

    if (key == Qt::Key_2) {
        if (current.isValid() && current.row() == rowCount - 1) {
            m_window->m_currentPage =
                m_window->m_currentPage < totalPages
                    ? m_window->m_currentPage + 1
                    : 1;
            m_window->refreshData();
            ToolTipOverlay::instance()->showText(
                QCursor::pos(),
                QString("<b style='color: #2ecc71;'>[下一页] 第 %1 / %2 页</b>")
                    .arg(m_window->m_currentPage)
                    .arg(m_window->m_totalPages),
                2000);
            if (m_window->m_model->rowCount() > 0) {
                const QModelIndex first = m_window->m_model->index(0, 0);
                m_window->m_listView->setCurrentIndex(first);
                m_window->m_listView->scrollTo(first);
            }
        } else {
            const QModelIndex last = m_window->m_model->index(rowCount - 1, 0);
            m_window->m_listView->setCurrentIndex(last);
            m_window->m_listView->scrollTo(last);
        }
        return true;
    }

    if (key == Qt::Key_3) {
        if (!current.isValid()) {
            m_window->m_listView->setCurrentIndex(m_window->m_model->index(0, 0));
            return true;
        }

        if (current.row() > 0) {
            const QModelIndex previous =
                m_window->m_model->index(current.row() - 1, 0);
            m_window->m_listView->setCurrentIndex(previous);
            m_window->m_listView->scrollTo(previous);
        } else {
            m_window->m_currentPage =
                m_window->m_currentPage > 1
                    ? m_window->m_currentPage - 1
                    : totalPages;
            if (m_window->m_currentPage == totalPages) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), "已回环至末页");
            }
            m_window->refreshData();
            const int newRowCount = m_window->m_model->rowCount();
            if (newRowCount > 0) {
                const QModelIndex last =
                    m_window->m_model->index(newRowCount - 1, 0);
                m_window->m_listView->setCurrentIndex(last);
                m_window->m_listView->scrollTo(last);
            }
        }
        return true;
    }

    if (key == Qt::Key_4) {
        if (!current.isValid()) {
            m_window->m_listView->setCurrentIndex(m_window->m_model->index(0, 0));
            return true;
        }

        if (current.row() < rowCount - 1) {
            const QModelIndex next =
                m_window->m_model->index(current.row() + 1, 0);
            m_window->m_listView->setCurrentIndex(next);
            m_window->m_listView->scrollTo(next);
        } else {
            m_window->m_currentPage =
                m_window->m_currentPage < totalPages
                    ? m_window->m_currentPage + 1
                    : 1;
            if (m_window->m_currentPage == 1) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), "已回环至首页");
            }
            m_window->refreshData();
            if (m_window->m_model->rowCount() > 0) {
                const QModelIndex first = m_window->m_model->index(0, 0);
                m_window->m_listView->setCurrentIndex(first);
                m_window->m_listView->scrollTo(first);
            }
        }
        return true;
    }

    return false;
}

bool QuickShortcutHandler::handleWheelScroll(QObject* watched,
                                              QWheelEvent* wheelEvent) {
    if (!m_window || !wheelEvent || watched != m_window->m_listView ||
        !m_window->m_listView) {
        return false;
    }

    const int delta = wheelEvent->angleDelta().y();
    QScrollBar* verticalScrollBar = m_window->m_listView->verticalScrollBar();
    if (!verticalScrollBar || delta == 0) {
        return false;
    }

    if (m_lastWheelPageTimer.isValid() &&
        m_lastWheelPageTimer.elapsed() <= 600) {
        return false;
    }

    if (delta < 0 &&
        verticalScrollBar->value() >= verticalScrollBar->maximum() &&
        m_window->m_currentPage < m_window->m_totalPages) {
        ++m_window->m_currentPage;
        m_window->refreshData();
        m_lastWheelPageTimer.start();
        ToolTipOverlay::instance()->showText(
            QCursor::pos(),
            QString("<b style='color: #2ecc71;'>[下一页] 第 %1 / %2 页</b>")
                .arg(m_window->m_currentPage)
                .arg(m_window->m_totalPages),
            2000);
        return true;
    }

    if (delta > 0 &&
        verticalScrollBar->value() <= verticalScrollBar->minimum() &&
        m_window->m_currentPage > 1) {
        --m_window->m_currentPage;
        m_window->refreshData();
        m_lastWheelPageTimer.start();
        ToolTipOverlay::instance()->showText(
            QCursor::pos(),
            QString("<b style='color: #3498db;'>[上一页] 第 %1 / %2 页</b>")
                .arg(m_window->m_currentPage)
                .arg(m_window->m_totalPages),
            2000);

        const QPointer<QScrollBar> guardedScrollBar(verticalScrollBar);
        QTimer::singleShot(10, [guardedScrollBar]() {
            if (guardedScrollBar) {
                guardedScrollBar->setValue(guardedScrollBar->maximum());
            }
        });
        return true;
    }

    return false;
}

bool QuickShortcutHandler::isSidebarVisible() const {
    if (!m_window || !m_window->m_systemTree) {
        return false;
    }
    QWidget* parent = m_window->m_systemTree->parentWidget();
    return parent && parent->isVisible();
}

} // namespace RapidNotes
