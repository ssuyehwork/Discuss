#include "ContentKeyHandler.h"
#include "../ContentPanel.h"
#include "../ThumbnailDelegate.h"
#include "../ToolTipOverlay.h"
#include "../ShellIconManager.h"
#include "../../core/TrashService.h"
#include "../../core/PermanentDeleteService.h"
#include "../../core/ClipboardService.h"
#include "../../core/ModelContract.h"

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

    // 1. 网格/自适应视图下的星级 Hitbox 点击计算
    ThumbnailDelegate* thumbDel = qobject_cast<ThumbnailDelegate*>(view->itemDelegateForIndex(index));
    if (thumbDel) {
        QStyleOptionViewItem opt;
        opt.rect = view->visualRect(index);
        opt.decorationSize = view->iconSize();
        if (opt.decorationSize.width() <= 0) opt.decorationSize = QSize(96, 96);
        ThumbnailDelegate::Metrics m = thumbDel->calculateMetrics(opt);

        bool isBanHit = m.banRect.contains(pos);
        int hitStar = -1;
        for (int i = 0; i < 5; ++i) {
            if (m.starRect(i).contains(pos)) {
                hitStar = i + 1;
                break;
            }
        }

        if (isBanHit || hitStar != -1) {
            bool isSelected = view->selectionModel() && view->selectionModel()->isSelected(index);
            if (!isSelected) return false;

            int newValue = isBanHit ? 0 : hitStar;
            auto selectedIndexes = view->selectionModel()->selectedIndexes();
            for (const auto& selIdx : selectedIndexes) {
                if (selIdx.column() == 0) {
                    m_panel->getProxyModel()->setData(selIdx, newValue, RatingRole);
                }
            }

            QAbstractItemView::EditTriggers currentTriggers = view->editTriggers();
            view->setEditTriggers(QAbstractItemView::NoEditTriggers);
            QTimer::singleShot(0, view, [view, currentTriggers]() {
                view->setEditTriggers(currentTriggers);
            });
            event->accept();
            return true;
        }
    }

    // 2. 列表视图（TreeView）第 2 列星级的 Hitbox 点击计算
    QTreeView* treeView = qobject_cast<QTreeView*>(view);
    if (treeView) {
        QModelIndex indexCol2 = index.model()->index(index.row(), 2, index.parent());
        QRect col2Rect = treeView->visualRect(indexCol2);

        int banW = 12;
        int starSize = 18;
        int banGap = 2;
        int starSpacing = -4;
        int totalW = banW + banGap + 5 * starSize + 4 * starSpacing;
        int startX = col2Rect.left() + (col2Rect.width() - totalW) / 2;

        QRect banHitbox(startX, col2Rect.top() + (col2Rect.height() - banW) / 2, banW, banW);
        bool isBanHit = banHitbox.contains(pos);
        int hitStar = -1;

        int starsStartX = startX + banW + banGap;
        for (int i = 0; i < 5; ++i) {
            QRect starRect(starsStartX + i * (starSize + starSpacing), col2Rect.top() + (col2Rect.height() - starSize) / 2, starSize, starSize);
            if (starRect.contains(pos)) {
                hitStar = i + 1;
                break;
            }
        }

        if (isBanHit || hitStar != -1) {
            bool isRowSelected = treeView->selectionModel() && treeView->selectionModel()->isRowSelected(index.row(), index.parent());
            if (!isRowSelected) return false;

            int newValue = isBanHit ? 0 : hitStar;
            auto selectedRows = treeView->selectionModel()->selectedRows();
            for (const auto& selRow : selectedRows) {
                QModelIndex targetIdx = treeView->model()->index(selRow.row(), 0, selRow.parent());
                m_panel->getProxyModel()->setData(targetIdx, newValue, RatingRole);
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

    // 4. Ctrl + Shift + C: 复制路径列表
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
            m_panel->performBatchRename();
            return true;
        }
    }

    // 5. 基础文件操作键
    if (keyEvent->key() == Qt::Key_F2) {
        view->edit(view->currentIndex());
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

    // 7. 空格键: QuickLook 预览 (受支持类型白名单过滤)
    if (keyEvent->key() == Qt::Key_Space) {
        QModelIndex idx = view->currentIndex();
        if (!idx.isValid() && view->selectionModel()) {
            auto selected = view->selectionModel()->selectedIndexes();
            if (!selected.isEmpty()) idx = selected.first();
        }

        if (idx.isValid()) {
            QString path = idx.data(PathRole).toString();
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
