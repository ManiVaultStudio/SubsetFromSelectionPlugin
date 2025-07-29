#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QLineEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QString>

// Struct to hold subset options
struct SubsetOptions {
    // Option 1: true = include children, false = exclude children
    bool includeChildren;
    // Option 2: true = create as child, false = create as separate
    bool createSubsetAsChild;
};

// Dialog for subset transformation options
class TransformationDialog : public QDialog {
    Q_OBJECT
public:
    explicit TransformationDialog(QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Create subset from Selection");
        QVBoxLayout* layout = new QVBoxLayout(this);

        // Info label
        layout->addWidget(new QLabel("This will create a subset dataset based on selected indices for the dataset."));

        // Option 1: Include/Exclude children
        QGroupBox* childrenGroup = new QGroupBox("Child Datasets", this);
        QHBoxLayout* childrenLayout = new QHBoxLayout(childrenGroup);
        _includeChildren = new QRadioButton("Include children", this);
        _excludeChildren = new QRadioButton("Exclude children", this);
        _excludeChildren->setChecked(true);
        QButtonGroup* childrenButtonGroup = new QButtonGroup(this);
        childrenButtonGroup->addButton(_includeChildren);
        childrenButtonGroup->addButton(_excludeChildren);
        childrenLayout->addWidget(_includeChildren);
        childrenLayout->addWidget(_excludeChildren);
        childrenGroup->setLayout(childrenLayout);
        layout->addWidget(childrenGroup);

        // Option 2: Subset as child/separate
        QGroupBox* subsetGroup = new QGroupBox("Subset Placement", this);
        QHBoxLayout* subsetLayout = new QHBoxLayout(subsetGroup);
        _createSubsetAsChild = new QRadioButton("Create subset as child of parent", this);
        _createSubsetSeparate = new QRadioButton("Create subset as separate dataset", this);
        _createSubsetSeparate->setChecked(true);
        QButtonGroup* subsetButtonGroup = new QButtonGroup(this);
        subsetButtonGroup->addButton(_createSubsetAsChild);
        subsetButtonGroup->addButton(_createSubsetSeparate);
        subsetLayout->addWidget(_createSubsetAsChild);
        subsetLayout->addWidget(_createSubsetSeparate);
        subsetGroup->setLayout(subsetLayout);
        layout->addWidget(subsetGroup);

        // OK/Cancel buttons
        QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttonBox);

        // --- Logic for enabling/disabling Option 2 based on Option 1 ---
        // Initial state: Option 2 enabled (since _excludeChildren is checked by default)
        _createSubsetAsChild->setEnabled(true);
        _createSubsetSeparate->setEnabled(true);

        connect(_includeChildren, &QRadioButton::toggled, this, [this](bool checked) {
            if (checked) {
                _createSubsetSeparate->setChecked(true);
                _createSubsetAsChild->setEnabled(false);
                _createSubsetSeparate->setEnabled(false);
            }
            });

        connect(_excludeChildren, &QRadioButton::toggled, this, [this](bool checked) {
            if (checked) {
                _createSubsetAsChild->setEnabled(true);
                _createSubsetSeparate->setEnabled(true);
            }
            });
    }

    // Option 1: true = include children, false = exclude children
    bool includeChildren() const { return _includeChildren->isChecked(); }

    // Option 2: true = create as child, false = create as separate
    bool createSubsetAsChild() const { return _createSubsetAsChild->isChecked(); }

    // Gather all options into a struct
    SubsetOptions getOptions() const {
        return SubsetOptions{
            includeChildren(),
            createSubsetAsChild()
        };
    }

private:
    // Option 1
    QRadioButton* _includeChildren;
    QRadioButton* _excludeChildren;

    // Option 2
    QRadioButton* _createSubsetAsChild;
    QRadioButton* _createSubsetSeparate;
};