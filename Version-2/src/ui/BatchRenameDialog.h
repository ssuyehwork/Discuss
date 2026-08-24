#pragma once

#include "FramelessDialog.h"
#include <QRadioButton>
#include <QPointer>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QGroupBox>
#include <QList>
#include <QTimer>
#include <QTableWidget>
#include <vector>
#include <string>
#include "../meta/BatchRenameEngine.h"

namespace QuarkMeta {

class RuleRow;

/**
 * @brief 批量重命名高级对话框 (Adobe Bridge 风格)
 */
class BatchRenameDialog : public FramelessDialog {
    Q_OBJECT
public:
    explicit BatchRenameDialog(const std::vector<std::wstring>& originalPaths, QWidget* parent = nullptr);
    ~BatchRenameDialog() override = default;

    QString getFirstNewName() const { return m_firstNewName; }

private slots:
    void onAddRow();
    void updatePreview();
    void onExecute();
    void onBrowseTarget();
    void onImportPreset();
    void onExportPreset();
    void onDeleteCurrentPreset();
    void scheduleAutoSave();
    void doAutoSave();

private:
    void initContent();
    void applyTheme();

    std::vector<std::wstring> m_originalPaths;
    
    // 预设相关
    QComboBox* m_presetCombo = nullptr;
    QPushButton* m_btnImportPreset = nullptr; // 导入按钮
    QPushButton* m_btnExportPreset = nullptr; // 导出按钮
    QPushButton* m_btnDeletePreset = nullptr; // 删除预设按钮

    QTimer* m_autoSaveTimer = nullptr;

    // 目标操作
    QRadioButton* m_rbRename = nullptr;
    QRadioButton* m_rbMove = nullptr;
    QRadioButton* m_rbCopy = nullptr;
    QLineEdit* m_targetPathEdit = nullptr;
    QPushButton* m_btnBrowse = nullptr;
    
    // 命名规则
    QWidget* m_rulesContainer = nullptr;
    QVBoxLayout* m_rulesLayout = nullptr;
    QList<RuleRow*> m_ruleRows;
    
    // 右侧实时预览表格 (700px 宽度)
    QTableWidget* m_table = nullptr;

    // 左侧底部主执行按钮 (水平居中)
    QPushButton* m_btnExecute = nullptr;

    QString m_firstNewName;
};

} // namespace QuarkMeta
