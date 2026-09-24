#pragma once

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace QuarkMeta {

class MetaInfoSection : public QWidget {
    Q_OBJECT
public:
    explicit MetaInfoSection(QWidget* parent = nullptr);
    ~MetaInfoSection() override = default;

    void updateInfo(const QString& name, const QString& type, const QString& size,
                    const QString& ctime, const QString& mtime, const QString& atime,
                    const QString& path, bool encrypted, int width = 0, int height = 0);

private:
    QVBoxLayout* m_mainLayout = nullptr;
    QLabel* lblType = nullptr;
    QLabel* lblSize = nullptr;
    QLabel* lblDimensions = nullptr;
    QLabel* lblCtime = nullptr;
    QLabel* lblMtime = nullptr;
    QLabel* lblAtime = nullptr;
    QLabel* lblEncrypted = nullptr;

    QLineEdit* m_pathEdit = nullptr;
    QPushButton* m_btnCopyPath = nullptr;
    QPushButton* m_btnOpenLocation = nullptr;

    void addInfoRow(QVBoxLayout* layout, const QString& label, QLabel*& valueLabel);
};

} // namespace QuarkMeta
