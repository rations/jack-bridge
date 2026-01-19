/*
JACK Audio Connection Kit - Qt GUI Interface.

   Copyright (C) 2003-2024, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 
*/

/********************************************************************************
** Form generated from reading UI file 'qjackctlPaletteForm.ui'
**
** Created by: Qt User Interface Compiler version 6.4.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QJACKCTLPALETTEFORM_H
#define UI_QJACKCTLPALETTEFORM_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_qjackctlPaletteForm
{
public:
    QVBoxLayout *vboxLayout;
    QGroupBox *nameBox;
    QHBoxLayout *hboxLayout;
    QComboBox *nameCombo;
    QPushButton *saveButton;
    QPushButton *deleteButton;
    QGroupBox *paletteBox;
    QGridLayout *gridLayout;
    QTreeView *paletteView;
    QLabel *generateLabel;
    qjackctlPaletteForm::ColorButton *generateButton;
    QPushButton *resetButton;
    QSpacerItem *spacerItem;
    QPushButton *importButton;
    QPushButton *exportButton;
    QSpacerItem *spacerItem1;
    QCheckBox *detailsCheck;
    QDialogButtonBox *dialogButtons;

    void setupUi(QDialog *qjackctlPaletteForm)
    {
        if (qjackctlPaletteForm->objectName().isEmpty())
            qjackctlPaletteForm->setObjectName("qjackctlPaletteForm");
        qjackctlPaletteForm->resize(534, 640);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(qjackctlPaletteForm->sizePolicy().hasHeightForWidth());
        qjackctlPaletteForm->setSizePolicy(sizePolicy);
        vboxLayout = new QVBoxLayout(qjackctlPaletteForm);
        vboxLayout->setObjectName("vboxLayout");
        nameBox = new QGroupBox(qjackctlPaletteForm);
        nameBox->setObjectName("nameBox");
        hboxLayout = new QHBoxLayout(nameBox);
        hboxLayout->setObjectName("hboxLayout");
        nameCombo = new QComboBox(nameBox);
        nameCombo->setObjectName("nameCombo");
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(nameCombo->sizePolicy().hasHeightForWidth());
        nameCombo->setSizePolicy(sizePolicy1);
        nameCombo->setMinimumSize(QSize(320, 0));
        nameCombo->setEditable(true);
        nameCombo->setInsertPolicy(QComboBox::NoInsert);

        hboxLayout->addWidget(nameCombo);

        saveButton = new QPushButton(nameBox);
        saveButton->setObjectName("saveButton");
        const QIcon icon = QIcon(QString::fromUtf8(":/images/formSave.png"));
        saveButton->setIcon(icon);

        hboxLayout->addWidget(saveButton);

        deleteButton = new QPushButton(nameBox);
        deleteButton->setObjectName("deleteButton");
        const QIcon icon1 = QIcon(QString::fromUtf8(":/images/formRemove.png"));
        deleteButton->setIcon(icon1);

        hboxLayout->addWidget(deleteButton);


        vboxLayout->addWidget(nameBox);

        paletteBox = new QGroupBox(qjackctlPaletteForm);
        paletteBox->setObjectName("paletteBox");
        gridLayout = new QGridLayout(paletteBox);
        gridLayout->setObjectName("gridLayout");
        paletteView = new QTreeView(paletteBox);
        paletteView->setObjectName("paletteView");
        paletteView->setMinimumSize(QSize(280, 360));
        paletteView->setAlternatingRowColors(true);

        gridLayout->addWidget(paletteView, 0, 0, 1, 8);

        generateLabel = new QLabel(paletteBox);
        generateLabel->setObjectName("generateLabel");

        gridLayout->addWidget(generateLabel, 1, 0, 1, 1);

        generateButton = new qjackctlPaletteForm::ColorButton(paletteBox);
        generateButton->setObjectName("generateButton");
        QSizePolicy sizePolicy2(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(generateButton->sizePolicy().hasHeightForWidth());
        generateButton->setSizePolicy(sizePolicy2);
        generateButton->setFocusPolicy(Qt::StrongFocus);

        gridLayout->addWidget(generateButton, 1, 1, 1, 1);

        resetButton = new QPushButton(paletteBox);
        resetButton->setObjectName("resetButton");
        const QIcon icon2 = QIcon(QString::fromUtf8(":/images/itemReset.png"));
        resetButton->setIcon(icon2);

        gridLayout->addWidget(resetButton, 1, 2, 1, 1);

        spacerItem = new QSpacerItem(8, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem, 1, 3, 1, 1);

        importButton = new QPushButton(paletteBox);
        importButton->setObjectName("importButton");
        const QIcon icon3 = QIcon(QString::fromUtf8(":/images/formOpen.png"));
        importButton->setIcon(icon3);

        gridLayout->addWidget(importButton, 1, 4, 1, 1);

        exportButton = new QPushButton(paletteBox);
        exportButton->setObjectName("exportButton");
        exportButton->setIcon(icon);

        gridLayout->addWidget(exportButton, 1, 5, 1, 1);

        spacerItem1 = new QSpacerItem(8, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem1, 1, 6, 1, 1);

        detailsCheck = new QCheckBox(paletteBox);
        detailsCheck->setObjectName("detailsCheck");

        gridLayout->addWidget(detailsCheck, 1, 7, 1, 1);


        vboxLayout->addWidget(paletteBox);

        dialogButtons = new QDialogButtonBox(qjackctlPaletteForm);
        dialogButtons->setObjectName("dialogButtons");
        dialogButtons->setOrientation(Qt::Horizontal);
        dialogButtons->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        vboxLayout->addWidget(dialogButtons);

        QWidget::setTabOrder(nameCombo, saveButton);
        QWidget::setTabOrder(saveButton, deleteButton);
        QWidget::setTabOrder(deleteButton, paletteView);
        QWidget::setTabOrder(paletteView, generateButton);
        QWidget::setTabOrder(generateButton, resetButton);
        QWidget::setTabOrder(resetButton, importButton);
        QWidget::setTabOrder(importButton, exportButton);
        QWidget::setTabOrder(exportButton, detailsCheck);
        QWidget::setTabOrder(detailsCheck, dialogButtons);

        retranslateUi(qjackctlPaletteForm);

        QMetaObject::connectSlotsByName(qjackctlPaletteForm);
    } // setupUi

    void retranslateUi(QDialog *qjackctlPaletteForm)
    {
        qjackctlPaletteForm->setWindowTitle(QCoreApplication::translate("qjackctlPaletteForm", "Color Themes", nullptr));
        nameBox->setTitle(QCoreApplication::translate("qjackctlPaletteForm", "Name", nullptr));
#if QT_CONFIG(tooltip)
        nameCombo->setToolTip(QCoreApplication::translate("qjackctlPaletteForm", "Current color palette name", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        saveButton->setToolTip(QCoreApplication::translate("qjackctlPaletteForm", "Save current color palette name", nullptr));
#endif // QT_CONFIG(tooltip)
        saveButton->setText(QCoreApplication::translate("qjackctlPaletteForm", "Save", nullptr));
#if QT_CONFIG(tooltip)
        deleteButton->setToolTip(QCoreApplication::translate("qjackctlPaletteForm", "Delete current color palette name", nullptr));
#endif // QT_CONFIG(tooltip)
        deleteButton->setText(QCoreApplication::translate("qjackctlPaletteForm", "Delete", nullptr));
        paletteBox->setTitle(QCoreApplication::translate("qjackctlPaletteForm", "Palette", nullptr));
#if QT_CONFIG(tooltip)
        paletteView->setToolTip(QCoreApplication::translate("qjackctlPaletteForm", "Current color palette", nullptr));
#endif // QT_CONFIG(tooltip)
        generateLabel->setText(QCoreApplication::translate("qjackctlPaletteForm", "Generate:", nullptr));
#if QT_CONFIG(tooltip)
        generateButton->setToolTip(QCoreApplication::translate("qjackctlPaletteForm", "Base color to generate palette", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        resetButton->setToolTip(QCoreApplication::translate("qjackctlPaletteForm", "Reset all current palette colors", nullptr));
#endif // QT_CONFIG(tooltip)
        resetButton->setText(QCoreApplication::translate("qjackctlPaletteForm", " Reset", nullptr));
#if QT_CONFIG(tooltip)
        importButton->setToolTip(QCoreApplication::translate("qjackctlPaletteForm", "Import a custom color theme (palette) from file", nullptr));
#endif // QT_CONFIG(tooltip)
        importButton->setText(QCoreApplication::translate("qjackctlPaletteForm", "Import...", nullptr));
#if QT_CONFIG(tooltip)
        exportButton->setToolTip(QCoreApplication::translate("qjackctlPaletteForm", "Export a custom color theme (palette) to file", nullptr));
#endif // QT_CONFIG(tooltip)
        exportButton->setText(QCoreApplication::translate("qjackctlPaletteForm", "Export...", nullptr));
        detailsCheck->setText(QCoreApplication::translate("qjackctlPaletteForm", "Show Details", nullptr));
    } // retranslateUi

};

namespace Ui {
    class qjackctlPaletteForm: public Ui_qjackctlPaletteForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QJACKCTLPALETTEFORM_H
