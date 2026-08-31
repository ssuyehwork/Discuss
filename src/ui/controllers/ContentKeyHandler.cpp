#include "ContentKeyHandler.h"
#include "../ContentPanel.h"
#include "../CardLayoutEngine.h"
#include "../RatingBarLayout.h"
#include "../ToolTipOverlay.h"
#include "../ShellIconManager.h"
#include "../../core/TrashService.h"
#include "../../core/PermanentDeleteService.h"
#include "../../core/ClipboardService.h"
#include "../../core/AppConfig.h"
#include "../../core/ModelContract.h"
#include "../../util/DiskIoService.h"
#include <QPointer>

#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QAbstractItemView>
#include <QTreeView>
#include <QLineEdit>
#include <QApplication>
#include <QClipboard>
#include <QFileInfo>
#include <QDir>
#include <QSet>
#include <QTimer>

namespace QuarkMeta {

ContentKeyHandler::ContentKeyHandler(ContentPanel* panel, QObject* parent)
    : QObject(parent), m_panel(panel) {
}

bool ContentKeyHandler::handleEvent(QObject* obj, QEvent* event) {
    if (!m_panel) return false;

    switch (event->type()) {
        case QEvent::Wheel:
            return handleWheel(obj, event);
        case QEvent::HoverEnter:
        case QEvent::Enter:
        case QEvent::HoverLeave:
        case QEvent::Leave:
            return handleTooltipHover(obj, event);
        case QEvent::MouseButtonPress:
            return handleMousePress(obj, event);
        case QEvent::KeyPress:
            return handleKeyPress(obj, event);
        default:
            break;
    }
    return false;
}

bool ContentKeyHandler::handleWheel(QObject* obj, QEvent* event) {
    Q_UNUSED(obj);
    QWheelEvent* wEvent = static_cast<QWheelEvent*>(event);
    if (wEvent->modifiers() & Qt::ControlModifier) {
        int deltaY = wEvent->angleDelta().y();
        int newZoom = m_panel->zoomLevel() + (deltaY > 0 ? 8 : -8);
        m_panel->setZoomLevel(newZoom);
        wEvent->accept();
        return true;
    }
    return false;
}

bool ContentKeyHandler::handleTooltipHover(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::HoverEnter || event->type() == QEvent::Enter) {
        QString text = obj->property("tooltipText").toString();
        if (!text.isEmpty()) {
            int timeout = (obj == m_panel->btnLayers() || 
                           obj == m_panel->btnToggleFolders() || 
                           obj == m_panel->btnToggleFiles()) ? 0 : 700;
            ToolTipOverlay::instance()->showText(QCursor::pos(), text, timeout);
        }
    } else if (event->type() == QEvent::HoverLeave || event->type() == QEvent::Leave) {
        ToolTipOverlay::hideTip();
    }
    return false;
}

bool ContentKeyHandler::handleMousePress(QObject* obj, QEvent* event) {
    QMouseEvent* mEvent = static_cast<QMouseEvent*>(event);
    if (mEvent->button() != Qt::LeftButton) return false;

    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(obj);
    if (!view) view = qobject_cast<QAbstractItemView*>(obj->parent());
    if (!view) return false;

    QPoint pos = mEvent->pos();
    if (obj == view && view->viewport()) {
        pos = view->viewport()->mapFrom(view, pos);
    }
    QModelIndex index = view->indexAt(pos);
    if (!index.isValid()) return false;

    // 1. 网格/自适应视图：归一化 Hitbox 查询
    if (view != m_panel->treeView()) {
        QRect itemRect = view->visualRect(index);
        CardLayout l = CardLayoutEngine::calculate(itemRect, m_panel->zoomLevel());
        int hitVal = l.hitStar(pos);

        if (hitVal != -1) {
            bool isSelected = view->selectionModel() && view->selectionModel()->isSelected(index);
            if (!isSelected) return false;

            auto selectedIndexes = view->selectionModel()->selectedIndexes();
            for (const auto& selIdx : selectedIndexes) {
                if (selIdx.column() == 0) {
                    m_panel->getProxyModel()->setData(selIdx, hitVal, RatingRole);
                }
            }

            QAbstractItemView::EditTriggers cur = view->editTriggers();
            view->setEditTriggers(QAbstractItemView::NoEditTriggers);
            QTimer::singleShot(0, view, [view, cur]() { view->setEditTriggers(cur); });
            event->accept();
            return true;
        }
    }

    // 2. 列表视图（TreeView）第 2 列星级 Hitbox 点击计算
    QTreeView* treeView = qobject_cast<QTreeView*>(view);
    if (treeView) {
        QModelIndex indexCol2 = index.model()->index(index.row(), 2, index.parent());
        QRect col2Rect = treeView->visualRect(indexCol2);

        RatingBarMetrics rm = RatingBarLayout::calculate(col2Rect, RatingBarMode::TreeRow);

        int hitStar = -1;
        if (rm.banRect.contains(pos)) hitStar = 0;
        else {
            for (int i = 0; i < 5; ++i) {
                if (rm.starRect(i).contains(pos)) { hitStar = i + 1; break; }
            }
        }

        if (hitStar != -1) {
            bool isRowSelected = treeView->selectionModel() && treeView->selectionModel()->isRowSelected(index.row(), index.parent());
            if (!isRowSelected) return false;

            auto selectedRows = treeView->selectionModel()->selectedRows();
            for (const auto& selRow : selectedRows) {
                QModelIndex targetIdx = treeView->model()->index(selRow.row(), 0, selRow.parent());
                m_panel->getProxyModel()->setData(targetIdx, hitStar, RatingRole);
            }

            QAbstractItemView::EditTriggers currentTriggers = treeView->editTriggers();
            treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
            QTimer::singleShot(0, treeView, [treeView, currentTriggers]() {
                treeView->setEditTriggers(currentTriggers);
            });
            event->accept();
            return true;
        }
    }
    return false;
}

bool ContentKeyHandler::handleKeyPress(QObject* obj, QEvent* event) {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    if (qobject_cast<QLineEdit*>(QApplication::focusWidget())) return false;

    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(obj);
    if (!view) view = qobject_cast<QAbstractItemView*>(obj->parent());
    if (!view) return false;

    // 1. Ctrl + 0~5: 评级
    if ((keyEvent->modifiers() & Qt::ControlModifier) && (keyEvent->key() >= Qt::Key_0 && keyEvent->key() <= Qt::Key_5)) {
        int rating = keyEvent->key() - Qt::Key_0;
        auto indexes = view->selectionModel()->selectedIndexes();
        for (const auto& idx : indexes) {
            if (idx.column() == 0) m_panel->getProxyModel()->setData(idx, rating, RatingRole);
        }
        return true;
    }

    // 2. Alt + D: 置顶/取消置顶
    if (((keyEvent->modifiers() & Qt::AltModifier) || (keyEvent->modifiers() & (Qt::AltModifier | Qt::WindowShortcut))) && (keyEvent->key() == Qt::Key_D)) {
        auto indexes = view->selectionModel()->selectedIndexes();
        for (const QModelIndex& idx : indexes) {
            if (idx.column() == 0) {
                bool current = idx.data(IsLockedRole).toBool();
                m_panel->getProxyModel()->setData(idx, !current, IsLockedRole);
            }
        }
        return true;
    }

    // 3. Alt + 1~9: 色标快速赋予
    if ((keyEvent->modifiers() & Qt::AltModifier) && (keyEvent->key() >= Qt::Key_1 && keyEvent->key() <= Qt::Key_9)) {
        static const QString colors[] = {
            "#E24B4A", "#EF9F27", "#FECF0E", "#639922",
            "#1D9E75", "#378ADD", "#7F77DD", "#5F5E5A", ""
        };
        QString colorValue = colors[keyEvent->key() - Qt::Key_1];

        auto indexes = view->selectionModel()->selectedIndexes();
        for (const auto& idx : indexes) {
            if (idx.column() == 0) {
                m_panel->getProxyModel()->setData(idx, colorValue, ColorRole);
                QString path = idx.data(PathRole).toString();
                QIcon coloredIcon = ShellIconManager::getFileIcon(path, 128);
                m_panel->getProxyModel()->setData(idx, coloredIcon, Qt::DecorationRole);
            }
        }
        return true;
    }

    // 4. Ctrl + Shift + C / R
    if (keyEvent->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
        if (keyEvent->key() == Qt::Key_C) {
            QStringList paths;
            auto indexes = view->selectionModel()->selectedIndexes();
            for (const auto& idx : indexes) {
                if (idx.column() == 0) paths << QDir::toNativeSeparators(idx.data(PathRole).toString());
            }
            if (!paths.isEmpty()) QApplication::clipboard()->setText(paths.join("\r\n"));
            return true;
        }
        if (keyEvent->key() == Qt::Key_R) {
            QString lastDragDest = AppConfig::instance().getValue("RecentVisited/LastDragDropDestination").toString();
            if (lastDragDest.isEmpty() || !QDir(lastDragDest).exists()) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), "尚未发生过拖拽移入操作或目标文件夹不存在", 1500, QColor("#e81123"));
                return true;
            }

            QStringList selectedPaths = m_panel->getSelectedPaths();
            if (selectedPaths.isEmpty()) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), "未选择任何项目", 1200, QColor("#e81123"));
                return true;
            }

            DiskIoContext ioCtx;
            ioCtx.sources = selectedPaths;
            ioCtx.destination = lastDragDest;
            ioCtx.isMove = true;

            QPointer<ContentPanel> weakPanel(m_panel);
            DiskIoService::instance().executeAsync(ioCtx, [weakPanel, lastDragDest](bool success) {
                QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, lastDragDest, success]() {
                    if (weakPanel) {
                        if (success) {
                            weakPanel->refreshAll();
                            QString folderName = QFileInfo(lastDragDest).fileName();
                            ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已移入到: %1").arg(folderName), 1500, QColor("#2ecc71"));
                        } else {
                            ToolTipOverlay::instance()->showText(QCursor::pos(), "移动失败：物理写入未能完成", 2000, QColor("#e81123"));
                        }
                    }
                });
            });
            return true;
        }
    }

    // 5. 基础文件操作键
    if (keyEvent->key() == Qt::Key_F2) {
        QStringList selectedPaths = m_panel->getSelectedPaths();
        if (selectedPaths.size() > 1) {
            m_panel->performBatchRename();
        } else {
            QModelIndex idx = view->currentIndex();
            if (!idx.isValid() && view->selectionModel()) {
                auto selected = view->selectionModel()->selectedIndexes();
                if (!selected.isEmpty()) idx = selected.first();
            }
            if (idx.isValid()) {
                QModelIndex nameIdx = idx.sibling(idx.row(), 0);
                view->setCurrentIndex(nameIdx);
                view->edit(nameIdx);
            }
        }
        return true;
    }
    if (keyEvent->key() == Qt::Key_Delete) {
        if (keyEvent->modifiers() & Qt::ShiftModifier) {
            PermanentDeleteService::instance().execute(m_panel->getSelectedPaths(), m_panel);
        } else {
            TrashService::instance().moveToTrash(m_panel->getSelectedPaths(), m_panel);
        }
        return true;
    }

    // 6. Ctrl + C / X / V / Shift+N
    if (keyEvent->modifiers() & Qt::ControlModifier) {
        if ((keyEvent->modifiers() & Qt::ShiftModifier) && keyEvent->key() == Qt::Key_N) {
            m_panel->createNewItem("folder");
            return true;
        }
        if (keyEvent->key() == Qt::Key_C && !(keyEvent->modifiers() & Qt::ShiftModifier)) {
            ClipboardService::instance().copyItems(m_panel->getSelectedPaths());
            return true;
        }
        if (keyEvent->key() == Qt::Key_X) {
            ClipboardService::instance().cutItems(m_panel->getSelectedPaths());
            return true;
        }
        if (keyEvent->key() == Qt::Key_V) {
            if (m_panel->canPaste()) {
                ClipboardService::instance().executePaste(m_panel->currentPath(), m_panel);
            }
            return true;
        }
    }

    // 7. 空格键: QuickLook 预览
    if (keyEvent->key() == Qt::Key_Space) {
        QModelIndex idx = view->currentIndex();
        if (!idx.isValid() && view->selectionModel()) {
            auto selected = view->selectionModel()->selectedIndexes();
            if (!selected.isEmpty()) idx = selected.first();
        }

        if (idx.isValid()) {
            // 🚀 确保即使焦点处于列表视图第 1~6 列，也能正确从第 0 列 sibling 获取完整 PathRole 属性
            QModelIndex nameIdx = idx.sibling(idx.row(), 0);
            QString path = nameIdx.data(PathRole).toString();
            if (!path.isEmpty()) {
                QFileInfo info(path);
                if (!info.isDir()) {
                    QString ext = info.suffix().toLower();
                    static const QSet<QString> whiteList = {
                        "jpg", "jpeg", "png", "bmp", "webp", "gif", "ico", "cur", "ani", "psd", "ai", "eps", "pdf", "svg",
                        "txt", "md", "markdown", "log", "cpp", "h", "hpp", "c", "py", "js", "css", "html", "json", "xml", "ini", "conf", "yaml", "yml"
                    };
                    if (whiteList.contains(ext)) {
                        emit m_panel->requestQuickLook(path);
                    }
                }
            }
        }
        return true;
    }

    // 8. 导航键
    if (keyEvent->key() == Qt::Key_Backspace) {
        QDir dir(m_panel->currentPath());
        if (dir.cdUp()) emit m_panel->directorySelected(dir.absolutePath());
        return true;
    }
    if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
        m_panel->onDoubleClicked(view->currentIndex());
        return true;
    }
    if ((keyEvent->modifiers() & Qt::ControlModifier) && keyEvent->key() == Qt::Key_Backslash) {
        ContentPanel::ViewMode nextMode = ContentPanel::ListView;
        if (m_panel->currentViewMode() == ContentPanel::ListView) nextMode = ContentPanel::GridView;
        else if (m_panel->currentViewMode() == ContentPanel::GridView) nextMode = ContentPanel::JustifiedViewMode;
        else if (m_panel->currentViewMode() == ContentPanel::JustifiedViewMode) nextMode = ContentPanel::ListView;
        m_panel->setViewMode(nextMode);
        return true;
    }

    return false;
}

} // namespace QuarkMeta