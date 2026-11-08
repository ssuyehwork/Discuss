#include "CryptoActionHelper.h"
#include "../ContentPanel.h"
#include "../ToolTipOverlay.h"
#include "../dialogs/FramelessInputDialog.h"
#include "../../crypto/EncryptionManager.h"
#include "../../core/ModelContract.h"

#include <QAbstractItemView>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QPointer>
#include <QStringList>

namespace QuarkMeta {

void CryptoActionHelper::executeEncrypt(ContentPanel* panel, QAbstractItemView* view) {
    if (!panel || !view) return;

    FramelessInputDialog dlg("加密保护", "设置加密密码:", "", panel);
    dlg.setEchoMode(QLineEdit::Password);
    if (dlg.exec() == QDialog::Accepted) {
        QString pwd = dlg.text();
        if (pwd.isEmpty()) return;
        auto indexes = view->selectionModel()->selectedIndexes();
        QStringList targets;
        for (const auto& idx : indexes) {
            if (idx.column() == 0) targets << idx.data(PathRole).toString();
        }

        ToolTipOverlay::instance()->showToast("加密任务已在后台启动...", 2000);

        QPointer<ContentPanel> self(panel);
        EncryptionManager::instance().encryptBatchAsync(targets, pwd.toStdString(), [self](bool success) {
            if (self) self->refreshAll();
            ToolTipOverlay::instance()->showToast(
                success ? "加密任务处理完成" : "部分项目加密失败",
                1500,
                success ? QColor("#2ecc71") : QColor("#e81123")
            );
        });
    }
}

void CryptoActionHelper::executeDecrypt(ContentPanel* panel, QAbstractItemView* view) {
    if (!panel || !view) return;

    FramelessInputDialog dlg("解除外壳保护", "输入解密密码:", "", panel);
    dlg.setEchoMode(QLineEdit::Password);
    if (dlg.exec() == QDialog::Accepted) {
        QString pwd = dlg.text();
        if (pwd.isEmpty()) return;
        auto indexes = view->selectionModel()->selectedIndexes();
        QStringList targets;
        for (const auto& idx : indexes) {
            if (idx.column() == 0) targets << idx.data(PathRole).toString();
        }

        ToolTipOverlay::instance()->showToast("解密还原任务已在后台启动...", 2000);

        QPointer<ContentPanel> self(panel);
        EncryptionManager::instance().decryptBatchAsync(targets, pwd.toStdString(), [self](bool anySuccess) {
            if (self) self->refreshAll();
            if (anySuccess) {
                ToolTipOverlay::instance()->showToast("解除保护成功，文件已还原", 1500, QColor("#2ecc71"));
            } else {
                ToolTipOverlay::instance()->showToast("解密失败，请检查密码是否正确", 2000, QColor("#e74c3c"));
            }
        });
    }
}

void CryptoActionHelper::executeChangePassword(ContentPanel* panel, QAbstractItemView* view) {
    if (!panel || !view) return;

    FramelessInputDialog dlgOld("修改保护密码", "输入原密码:", "", panel);
    dlgOld.setEchoMode(QLineEdit::Password);
    if (dlgOld.exec() != QDialog::Accepted || dlgOld.text().isEmpty()) return;
    QString oldPwd = dlgOld.text();

    FramelessInputDialog dlgNew("修改保护密码", "输入新密码:", "", panel);
    dlgNew.setEchoMode(QLineEdit::Password);
    if (dlgNew.exec() != QDialog::Accepted || dlgNew.text().isEmpty()) return;
    QString newPwd = dlgNew.text();

    auto indexes = view->selectionModel()->selectedIndexes();
    QStringList targets;
    for (const auto& idx : indexes) {
        if (idx.column() == 0) targets << idx.data(PathRole).toString();
    }

    ToolTipOverlay::instance()->showToast("密码修改中...", 2000);

    QPointer<ContentPanel> self(panel);
    EncryptionManager::instance().changePasswordBatchAsync(
        targets, oldPwd.toStdString(), newPwd.toStdString(), [self](bool anySuccess) {
            if (self) self->refreshAll();
            if (anySuccess) {
                ToolTipOverlay::instance()->showToast("保护密码修改成功", 1500, QColor("#2ecc71"));
            } else {
                ToolTipOverlay::instance()->showToast("原密码错误，修改失败", 2000, QColor("#e74c3c"));
            }
        }
    );
}

} // namespace QuarkMeta
