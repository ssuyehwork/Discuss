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
#include "../../models/NoteModel.h"     // 【修正】根据实际目录调整 NoteModel.h 路径
#include "../../models/CategoryModel.h" // 【修正】根据实际目录调整 CategoryModel.h 路径
#include "../../core/DatabaseManager.h"
#include "../../core/TagCodec.h"
#include <QSortFilterProxyModel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QApplication>
#include <QCursor>

namespace RapidNotes {

QuickShortcutHandler::QuickShortcutHandler(QuickWindow* window, QObject* parent)
    : QObject(parent), m_window(window) {}

bool QuickShortcutHandler::isEditingFocus(QWidget* widget) const {
    if (!widget) return false;
    return qobject_cast<QLineEdit*>(widget) != nullptr ||
           qobject_cast<QTextEdit*>(widget) != nullptr ||
           qobject_cast<QPlainTextEdit*>(widget) != nullptr;
}

bool QuickShortcutHandler::handleEventFilter(QObject* watched, QEvent* event) {
    if (!m_window || m_window->isLocked()) return false;

    if (event->type() == QEvent::ShortcutOverride) {
        return handleShortcutOverride(static_cast<QKeyEvent*>(event));
    }

    if (event->type() == QEvent::KeyPress) {
        return handleKeyPress(watched, static_cast<QKeyEvent*>(event));
    }

    if (event->type() == QEvent::Wheel) {
        return handleWheelScroll(watched, static_cast<QWheelEvent*>(event));
    }

    return false;
}

bool QuickShortcutHandler::handleShortcutOverride(QKeyEvent* keyEvent) {
    if (keyEvent->key() == Qt::Key_S && (keyEvent->modifiers() & Qt::ControlModifier)) {
        if ((keyEvent->modifiers() & Qt::AltModifier) || !(keyEvent->modifiers() & Qt::ShiftModifier)) {
            keyEvent->accept();
            return true;
        }
    }
    return false;
}

bool QuickShortcutHandler::handleKeyPress(QObject* watched, QKeyEvent* keyEvent) {
    int key = keyEvent->key();
    auto mods = keyEvent->modifiers();

    // 1. 全局面板联动与筛选快捷键
    if (key == Qt::Key_E && (mods & Qt::AltModifier)) {
        m_window->toggleFilter();
        return true;
    }
    if (key == Qt::Key_R && (mods & Qt::AltModifier)) {
        m_window->toggleAllPanels();
        return true;
    }
    if (key == Qt::Key_G && mods == Qt::ControlModifier) {
        if (m_window->m_filterWrapper && m_window->m_filterWrapper->isVisible() && m_window->m_filterPanel) {
            m_window->m_filterPanel->toggleAllGroups();
        }
        return true;
    }
    if (key == Qt::Key_G && mods == (Qt::ControlModifier | Qt::ShiftModifier)) {
        if (m_window->m_filterWrapper && m_window->m_filterWrapper->isVisible() && m_window->m_filterPanel) {
            m_window->m_filterPanel->resetFilters();
        }
        return true;
    }

    // 2. 分类锁定与显隐切换拦截 (Ctrl+S / Ctrl+Alt+S)
    if (key == Qt::Key_S && (mods & Qt::ControlModifier)) {
        if (mods & Qt::AltModifier) {
            auto& db = DatabaseManager::instance();
            db.toggleLockedCategoriesVisibility();
            bool isHidden = db.isLockedCategoriesHidden();
            if (isHidden && m_window->m_currentFilterType == "category" && m_window->m_currentFilterValue != -1) {
                int catId = m_window->m_currentFilterValue.toInt();
                if (db.isCategoryLocked(catId) || !db.getCategoryPresetTags(catId).isNull()) {
                    m_window->m_currentFilterType = "all";
                    m_window->m_currentFilterValue = -1;
                }
            }
            m_window->refreshSidebar();
            m_window->refreshData();
            ToolTipOverlay::instance()->showText(QCursor::pos(), isHidden ?
                "<b style='color: #e67e22;'>[OK] 已隐藏加锁分类并强制重锁</b>" :
                "<b style='color: #2ecc71;'>[OK] 已显示所有分类并强制重锁</b>");
            return true;
        }

        if (!(mods & Qt::ShiftModifier)) {
            int catId = -1;
            QModelIndex sidebarIdx = m_window->m_partitionTree->currentIndex();
            if (sidebarIdx.isValid() && sidebarIdx.data(CategoryModel::TypeRole).toString() == "category") {
                catId = sidebarIdx.data(CategoryModel::IdRole).toInt();
            }
            if (catId == -1 && m_window->m_currentFilterType == "category" && m_window->m_currentFilterValue != -1) {
                catId = m_window->m_currentFilterValue.toInt();
            }
            if (catId != -1) {
                DatabaseManager::instance().lockCategory(catId);
                if (m_window->m_currentFilterType == "category" && m_window->m_currentFilterValue == catId) {
                    m_window->m_currentFilterType = "all";
                    m_window->m_currentFilterValue = -1;
                }
                m_window->refreshSidebar();
                m_window->refreshData();
                ToolTipOverlay::instance()->showText(QCursor::pos(), "<b style='color: #f39c12;'>[OK] 分类已物理锁定</b>");
                return true;
            }
        }
    }

    // 3. 全局 Esc 阶梯清空与退出机制
    if (key == Qt::Key_Escape) {
        return handleEscClear();
    }

    // 4. 输入框光标定向操作
    if (watched == m_window->m_searchEdit || watched == m_window->m_catSearchEdit ||
        watched == m_window->m_tagEdit || watched == m_window->m_pageInput) {
        QLineEdit* edit = qobject_cast<QLineEdit*>(watched);
        if (edit) {
            if (key == Qt::Key_Up) {
                edit->setCursorPosition(0);
                return true;
            } else if (key == Qt::Key_Down) {
                edit->setCursorPosition(edit->text().length());
                return true;
            }
        }
    }

    // 5. 侧边栏树形控件按键路由 (DropTreeView)
    if (watched == m_window->m_partitionTree || watched == m_window->m_systemTree) {
        if (key == Qt::Key_QuoteLeft || key == Qt::Key_Backspace) {
            m_window->m_currentFilterType = "all";
            m_window->m_currentFilterValue = -1;
            m_window->m_currentPage = 1;
            m_window->m_systemTree->selectionModel()->clearSelection();
            m_window->m_systemTree->setCurrentIndex(QModelIndex());
            m_window->m_partitionTree->selectionModel()->clearSelection();
            m_window->m_partitionTree->setCurrentIndex(QModelIndex());
            m_window->m_currentCategoryColor = "#4a90e2";
            m_window->updatePartitionStatus("");
            m_window->applyListTheme("");
            m_window->refreshData();
            ToolTipOverlay::instance()->showText(QCursor::pos(), "[OK] 已切换至全部数据");
            return true;
        }

        if (key == Qt::Key_F2 && watched == m_window->m_partitionTree) {
            QModelIndex cur = m_window->m_partitionTree->currentIndex();
            if (cur.isValid() && cur.data(CategoryModel::TypeRole).toString() == "category") {
                m_window->m_partitionTree->edit(cur);
            }
            return true;
        }

        if (key == Qt::Key_Tab && m_window->m_systemTree->parentWidget()->isVisible()) {
            m_window->m_listView->setFocus();
            auto* model = m_window->m_listView->model();
            if (model && !m_window->m_listView->currentIndex().isValid() && model->rowCount() > 0) {
                m_window->m_listView->setCurrentIndex(model->index(0, 0));
            }
            return true;
        }

        if (key == Qt::Key_Delete) {
            if (watched == m_window->m_partitionTree) {
                auto selected = m_window->m_partitionTree->selectionModel()->selectedIndexes();
                if (!selected.isEmpty()) {
                    QString msg = (selected.size() > 1) ? QString("确定要删除选中的 %1 个分类及其下所有内容吗？").arg(selected.size()) : "确定要删除选中的分类及其下所有内容吗？";
                    FramelessMessageBox dlg("确认删除", msg, m_window);
                    if (dlg.exec() == QDialog::Accepted && m_window->m_mediator) {
                        QList<int> ids;
                        for (const auto& idx : selected) {
                            if (idx.data(CategoryModel::TypeRole).toString() == "category") {
                                ids << idx.data(CategoryModel::IdRole).toInt();
                            }
                        }
                        m_window->m_mediator->requestDeleteCategories(ids, false);
                    }
                }
            }
            return true;
        }

        if ((key == Qt::Key_Up || key == Qt::Key_Down) && (mods & Qt::AltModifier)) {
            QModelIndex cur = (watched == m_window->m_partitionTree) ? m_window->m_partitionTree->currentIndex() : m_window->m_systemTree->currentIndex();
            if (cur.isValid() && cur.data(CategoryModel::TypeRole).toString() == "category") {
                int catId = cur.data(CategoryModel::IdRole).toInt();
                DatabaseManager::MoveDirection dir;
                if (key == Qt::Key_Up) {
                    dir = (mods & Qt::ShiftModifier) ? DatabaseManager::Top : DatabaseManager::Up;
                } else {
                    dir = (mods & Qt::ShiftModifier) ? DatabaseManager::Bottom : DatabaseManager::Down;
                }
                if (m_window->m_mediator && m_window->m_mediator->requestMoveCategory(catId, dir)) {
                    m_window->refreshSidebar();
                    return true;
                }
            }
        }
    }

    // 6. 笔记列表按键路由 (CleanListView)
    if (watched == m_window->m_listView) {
        if (key == Qt::Key_C && (mods & Qt::ControlModifier)) {
            m_window->doExtractContent();
            return true;
        }

        if (key == Qt::Key_QuoteLeft || key == Qt::Key_Backspace) {
            m_window->m_currentFilterType = "all";
            m_window->m_currentFilterValue = -1;
            m_window->m_currentPage = 1;
            m_window->m_systemTree->selectionModel()->clearSelection();
            m_window->m_systemTree->setCurrentIndex(QModelIndex());
            m_window->m_partitionTree->selectionModel()->clearSelection();
            m_window->m_partitionTree->setCurrentIndex(QModelIndex());
            m_window->m_currentCategoryColor = "#4a90e2";
            m_window->updatePartitionStatus("");
            m_window->applyListTheme("");
            m_window->refreshData();
            ToolTipOverlay::instance()->showText(QCursor::pos(), "[OK] 已切换至全部数据");
            return true;
        }

        if ((key == Qt::Key_Up || key == Qt::Key_Down) && (mods & Qt::AltModifier)) {
            DatabaseManager::MoveDirection dir;
            if (mods & Qt::ShiftModifier || mods & Qt::ControlModifier) {
                dir = (key == Qt::Key_Up) ? DatabaseManager::Top : DatabaseManager::Bottom;
            } else {
                dir = (key == Qt::Key_Up) ? DatabaseManager::Up : DatabaseManager::Down;
            }
            m_window->doMoveNote(dir);
            return true;
        }

        if (key == Qt::Key_F2) {
            QModelIndex cur = m_window->m_listView->currentIndex();
            if (cur.isValid()) {
                QString oldTitle = cur.data(NoteModel::TitleRole).toString();
                int noteId = cur.data(NoteModel::IdRole).toInt();
                TitleEditorDialog dlg(oldTitle, m_window);
                if (dlg.exec() == QDialog::Accepted) {
                    QString newTitle = dlg.getText().trimmed();
                    if (!newTitle.isEmpty() && newTitle != oldTitle && m_window->m_mediator) {
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

        if (key == Qt::Key_Tab && m_window->m_systemTree->parentWidget()->isVisible()) {
            if (m_window->m_partitionTree->isVisible()) {
                m_window->m_partitionTree->setFocus();
                if (!m_window->m_partitionTree->currentIndex().isValid()) {
                    m_window->m_partitionTree->setCurrentIndex(m_window->m_partitionProxyModel->index(0, 0));
                }
            } else if (m_window->m_systemTree->isVisible()) {
                m_window->m_systemTree->setFocus();
            }
            return true;
        }

        if (!(mods & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier))) {
            if (key >= Qt::Key_1 && key <= Qt::Key_4) {
                return handleListNumericNavigation(key);
            }
        }
    }

    return false;
}

bool QuickShortcutHandler::handleEscClear() {
    bool searchHasText = m_window->m_searchEdit && !m_window->m_searchEdit->text().isEmpty();
    bool tagHasText = m_window->m_tagEdit && !m_window->m_tagEdit->text().isEmpty();
    bool catSearchHasText = m_window->m_catSearchEdit && !m_window->m_catSearchEdit->text().isEmpty();

    if (searchHasText || tagHasText || catSearchHasText) {
        if (searchHasText) m_window->m_searchEdit->clear();
        if (tagHasText) m_window->m_tagEdit->clear();
        if (catSearchHasText) m_window->m_catSearchEdit->clear();
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
    if (!m_window || !m_window->m_model || m_window->m_model->rowCount() == 0) return false;

    QModelIndex cur = m_window->m_listView->currentIndex();
    int rowCount = m_window->m_model->rowCount();

    if (key == Qt::Key_1) {
        if (cur.isValid() && cur.row() == 0) {
            if (m_window->m_currentPage > 1) {
                m_window->m_currentPage--;
            } else {
                m_window->m_currentPage = m_window->m_totalPages;
            }
            m_window->refreshData();
            ToolTipOverlay::instance()->showText(QCursor::pos(),
                QString("<b style='color: #3498db;'>[上一页] 第 %1 / %2 页</b>").arg(m_window->m_currentPage).arg(m_window->m_totalPages), 2000);
            if (m_window->m_model->rowCount() > 0) {
                QModelIndex lastIdx = m_window->m_model->index(m_window->m_model->rowCount() - 1, 0);
                m_window->m_listView->setCurrentIndex(lastIdx);
                m_window->m_listView->scrollTo(lastIdx);
            }
        } else {
            QModelIndex firstIdx = m_window->m_model->index(0, 0);
            m_window->m_listView->setCurrentIndex(firstIdx);
            m_window->m_listView->scrollTo(firstIdx);
        }
        return true;
    }

    if (key == Qt::Key_2) {
        if (cur.isValid() && cur.row() == rowCount - 1) {
            if (m_window->m_currentPage < m_window->m_totalPages) {
                m_window->m_currentPage++;
            } else {
                m_window->m_currentPage = 1;
            }
            m_window->refreshData();
            ToolTipOverlay::instance()->showText(QCursor::pos(),
                QString("<b style='color: #2ecc71;'>[下一页] 第 %1 / %2 页</b>").arg(m_window->m_currentPage).arg(m_window->m_totalPages), 2000);
            if (m_window->m_model->rowCount() > 0) {
                QModelIndex firstIdx = m_window->m_model->index(0, 0);
                m_window->m_listView->setCurrentIndex(firstIdx);
                m_window->m_listView->scrollTo(firstIdx);
            }
        } else {
            QModelIndex lastIdx = m_window->m_model->index(rowCount - 1, 0);
            m_window->m_listView->setCurrentIndex(lastIdx);
            m_window->m_listView->scrollTo(lastIdx);
        }
        return true;
    }

    if (key == Qt::Key_3) {
        if (!cur.isValid()) {
            m_window->m_listView->setCurrentIndex(m_window->m_model->index(0, 0));
            return true;
        }
        int row = cur.row();
        if (row > 0) {
            QModelIndex prevIdx = m_window->m_model->index(row - 1, 0);
            m_window->m_listView->setCurrentIndex(prevIdx);
            m_window->m_listView->scrollTo(prevIdx);
        } else {
            if (m_window->m_currentPage > 1) {
                m_window->m_currentPage--;
            } else {
                m_window->m_currentPage = m_window->m_totalPages;
                ToolTipOverlay::instance()->showText(QCursor::pos(), "已回环至末页");
            }
            m_window->refreshData();
            if (m_window->m_model->rowCount() > 0) {
                QModelIndex lastIdx = m_window->m_model->index(m_window->m_model->rowCount() - 1, 0);
                m_window->m_listView->setCurrentIndex(lastIdx);
                m_window->m_listView->scrollTo(lastIdx);
            }
        }
        return true;
    }

    if (key == Qt::Key_4) {
        if (!cur.isValid()) {
            m_window->m_listView->setCurrentIndex(m_window->m_model->index(0, 0));
            return true;
        }
        int row = cur.row();
        if (row < rowCount - 1) {
            QModelIndex nextIdx = m_window->m_model->index(row + 1, 0);
            m_window->m_listView->setCurrentIndex(nextIdx);
            m_window->m_listView->scrollTo(nextIdx);
        } else {
            if (m_window->m_currentPage < m_window->m_totalPages) {
                m_window->m_currentPage++;
            } else {
                m_window->m_currentPage = 1;
                ToolTipOverlay::instance()->showText(QCursor::pos(), "已回环至首页");
            }
            m_window->refreshData();
            if (m_window->m_model->rowCount() > 0) {
                QModelIndex firstIdx = m_window->m_model->index(0, 0);
                m_window->m_listView->setCurrentIndex(firstIdx);
                m_window->m_listView->scrollTo(firstIdx);
            }
        }
        return true;
    }

    return false;
}

bool QuickShortcutHandler::handleWheelScroll(QObject* watched, QWheelEvent* wheelEvent) {
    if (watched != m_window->m_listView) return false;

    int delta = wheelEvent->angleDelta().y();
    QScrollBar* vb = m_window->m_listView->verticalScrollBar();
    if (!vb) return false;

    if (!m_lastWheelPageTimer.isValid() || m_lastWheelPageTimer.elapsed() > 600) {
        if (delta < 0 && vb->value() >= vb->maximum()) {
            if (m_window->m_currentPage < m_window->m_totalPages) {
                m_window->m_currentPage++;
                m_window->refreshData();
                m_lastWheelPageTimer.start();
                ToolTipOverlay::instance()->showText(QCursor::pos(),
                    QString("<b style='color: #2ecc71;'>[下一页] 第 %1 / %2 页</b>").arg(m_window->m_currentPage).arg(m_window->m_totalPages), 2000);
                return true;
            }
        } else if (delta > 0 && vb->value() <= vb->minimum()) {
            if (m_window->m_currentPage > 1) {
                m_window->m_currentPage--;
                m_window->refreshData();
                m_lastWheelPageTimer.start();
                ToolTipOverlay::instance()->showText(QCursor::pos(),
                    QString("<b style='color: #3498db;'>[上一页] 第 %1 / %2 页</b>").arg(m_window->m_currentPage).arg(m_window->m_totalPages), 2000);
                QTimer::singleShot(10, [vb]() { vb->setValue(vb->maximum()); });
                return true;
            }
        }
    }
    return false;
}

} // namespace RapidNotes