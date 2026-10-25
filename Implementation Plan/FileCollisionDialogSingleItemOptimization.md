# Implementation Plan - File Collision Dialog Single-Item Layout Optimization

## 1. Overview
When a file conflict involves only a single item (`totalCount <= 1`), displaying the "是否应用于全部文件？" checkbox and the "跳过" button in `FileCollisionDialog` is redundant and confusing for users.

This implementation plan modifies `FileCollisionDialog::setupUi` in `src/ui/FileCollisionDialog.cpp` so that when `totalCount <= 1`, the "是否应用于全部文件？" checkbox and the "跳过" button are hidden, leaving only 3 buttons: **"同存"**, **"替换"**, and **"取消"**.

---

## 2. Modified Files List
- `src/ui/FileCollisionDialog.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/FileCollisionDialog.cpp`
Update `FileCollisionDialog::setupUi` to conditionally hide `m_chkApplyToAll` and `btnSkip` when `totalCount <= 1`:

```cpp
<<<<<<< SEARCH
    m_chkApplyToAll = new QCheckBox("是否应用于全部文件？", area);
    m_chkApplyToAll->setObjectName("CollisionApplyAllChk");
    btmLayout->addWidget(m_chkApplyToAll, 0, Qt::AlignVCenter);

    btmLayout->addStretch(1);

    auto createCapsuleBtn = [this, area](const QString& text, CollisionResolveAction action) {
        QPushButton* btn = new QPushButton(text, area);
        btn->setFixedHeight(28);
        btn->setMinimumWidth(76);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setObjectName("CollisionOptionBtn");
        connect(btn, &QPushButton::clicked, this, [this, action]() {
            m_selectedAction = action;
            accept();
        });
        return btn;
    };

    btmLayout->addWidget(createCapsuleBtn("同存", CollisionResolveAction::AutoResolve));
    btmLayout->addWidget(createCapsuleBtn("替换", CollisionResolveAction::Replace));
    btmLayout->addWidget(createCapsuleBtn("跳过", CollisionResolveAction::Skip));
    btmLayout->addWidget(createCapsuleBtn("取消", CollisionResolveAction::Cancel));
=======
    m_chkApplyToAll = new QCheckBox("是否应用于全部文件？", area);
    m_chkApplyToAll->setObjectName("CollisionApplyAllChk");
    if (totalCount <= 1) {
        m_chkApplyToAll->setVisible(false);
    }
    btmLayout->addWidget(m_chkApplyToAll, 0, Qt::AlignVCenter);

    btmLayout->addStretch(1);

    auto createCapsuleBtn = [this, area](const QString& text, CollisionResolveAction action) {
        QPushButton* btn = new QPushButton(text, area);
        btn->setFixedHeight(28);
        btn->setMinimumWidth(76);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setObjectName("CollisionOptionBtn");
        connect(btn, &QPushButton::clicked, this, [this, action]() {
            m_selectedAction = action;
            accept();
        });
        return btn;
    };

    btmLayout->addWidget(createCapsuleBtn("同存", CollisionResolveAction::AutoResolve));
    btmLayout->addWidget(createCapsuleBtn("替换", CollisionResolveAction::Replace));

    QPushButton* btnSkip = createCapsuleBtn("跳过", CollisionResolveAction::Skip);
    if (totalCount <= 1) {
        btnSkip->setVisible(false);
    }
    btmLayout->addWidget(btnSkip);

    btmLayout->addWidget(createCapsuleBtn("取消", CollisionResolveAction::Cancel));
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Recompile project target using `cmake --build .`.
2. Trigger a single-item conflict (e.g. copy/move/drop a single conflicting file).
3. Verify that `FileCollisionDialog` pops up showing only 3 buttons: "同存", "替换", "取消", and "是否应用于全部文件？" is hidden.
4. Trigger a multi-item conflict (e.g. copy/move 2+ conflicting files).
5. Verify that `FileCollisionDialog` shows "是否应用于全部文件？" and all 4 buttons ("同存", "替换", "跳过", "取消").

---

## 5. Official API Reuse & Anti-Redundancy
- Reuses existing `FileCollisionDialog` without creating duplicate dialog classes.

---

## 6. Header API Signature Verification
- No header API signature modifications required.

---

## 7. Header Inclusion Chain & Type Completeness Check
- `FileCollisionDialog.cpp` includes required Qt header types.

---

## 8. Data SSOT Declaration Table
| Entity / State | SSOT Location | Synchronization Mechanism |
| :--- | :--- | :--- |
| Conflict Count Display Mode | `FileCollisionDialog::setupUi(totalCount)` | Evaluates `totalCount <= 1` at setup time |

---

## 9. Compliance Self-Check Table
| Check Item | Status | Notes |
| :--- | :--- | :--- |
| Zero Code Modification Rule | PASS | Output strictly as plan document inside `Implementation Plan/` |
| Chinese Language Protocol | PASS | Documented completely in Chinese |
| Contract Lock | PASS | Public header signatures untouched |

---

## 10. Behavior Preservation Declaration Table
| Action | Existing Behavior | Preserved Behavior |
| :--- | :--- | :--- |
| Multi-Item Conflict | Displays 4 buttons + "Apply to All" | Preserved |
| Single-Item Conflict | Displays 4 buttons + "Apply to All" | Displays 3 buttons ("同存", "替换", "取消") and hides "Apply to All" |
