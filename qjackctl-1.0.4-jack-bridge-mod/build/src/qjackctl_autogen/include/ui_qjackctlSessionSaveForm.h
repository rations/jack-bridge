/*
qjackctl - An Audio/MIDI multi-track sequencer.

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
** Form generated from reading UI file 'qjackctlSessionSaveForm.ui'
**
** Created by: Qt User Interface Compiler version 6.4.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QJACKCTLSESSIONSAVEFORM_H
#define UI_QJACKCTLSESSIONSAVEFORM_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_qjackctlSessionSaveForm
{
public:
    QVBoxLayout *vboxLayout;
    QLabel *SessionNameTextLabel;
    QLineEdit *SessionNameLineEdit;
    QHBoxLayout *hboxLayout;
    QLabel *SessionDirTextLabel;
    QSpacerItem *spacerItem;
    QHBoxLayout *hboxLayout1;
    QComboBox *SessionDirComboBox;
    QToolButton *SessionDirToolButton;
    QSpacerItem *spacerItem1;
    QHBoxLayout *hboxLayout2;
    QCheckBox *SessionSaveVersionCheckBox;
    QDialogButtonBox *DialogButtonBox;

    void setupUi(QDialog *qjackctlSessionSaveForm)
    {
        if (qjackctlSessionSaveForm->objectName().isEmpty())
            qjackctlSessionSaveForm->setObjectName("qjackctlSessionSaveForm");
        qjackctlSessionSaveForm->resize(360, 180);
        qjackctlSessionSaveForm->setFocusPolicy(Qt::StrongFocus);
        const QIcon icon = QIcon(QString::fromUtf8(":/images/session1.png"));
        qjackctlSessionSaveForm->setWindowIcon(icon);
        vboxLayout = new QVBoxLayout(qjackctlSessionSaveForm);
        vboxLayout->setSpacing(4);
        vboxLayout->setContentsMargins(8, 8, 8, 8);
        vboxLayout->setObjectName("vboxLayout");
        SessionNameTextLabel = new QLabel(qjackctlSessionSaveForm);
        SessionNameTextLabel->setObjectName("SessionNameTextLabel");

        vboxLayout->addWidget(SessionNameTextLabel);

        SessionNameLineEdit = new QLineEdit(qjackctlSessionSaveForm);
        SessionNameLineEdit->setObjectName("SessionNameLineEdit");
        SessionNameLineEdit->setMinimumSize(QSize(320, 0));

        vboxLayout->addWidget(SessionNameLineEdit);

        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(4);
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        hboxLayout->setObjectName("hboxLayout");
        SessionDirTextLabel = new QLabel(qjackctlSessionSaveForm);
        SessionDirTextLabel->setObjectName("SessionDirTextLabel");

        hboxLayout->addWidget(SessionDirTextLabel);

        spacerItem = new QSpacerItem(20, 8, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem);


        vboxLayout->addLayout(hboxLayout);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(4);
        hboxLayout1->setContentsMargins(0, 0, 0, 0);
        hboxLayout1->setObjectName("hboxLayout1");
        SessionDirComboBox = new QComboBox(qjackctlSessionSaveForm);
        SessionDirComboBox->setObjectName("SessionDirComboBox");
        SessionDirComboBox->setMinimumSize(QSize(320, 0));
        SessionDirComboBox->setEditable(true);

        hboxLayout1->addWidget(SessionDirComboBox);

        SessionDirToolButton = new QToolButton(qjackctlSessionSaveForm);
        SessionDirToolButton->setObjectName("SessionDirToolButton");
        SessionDirToolButton->setMinimumSize(QSize(22, 22));
        SessionDirToolButton->setMaximumSize(QSize(24, 24));
        SessionDirToolButton->setFocusPolicy(Qt::TabFocus);

        hboxLayout1->addWidget(SessionDirToolButton);


        vboxLayout->addLayout(hboxLayout1);

        spacerItem1 = new QSpacerItem(20, 8, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout->addItem(spacerItem1);

        hboxLayout2 = new QHBoxLayout();
        hboxLayout2->setSpacing(4);
        hboxLayout2->setContentsMargins(0, 0, 0, 0);
        hboxLayout2->setObjectName("hboxLayout2");
        SessionSaveVersionCheckBox = new QCheckBox(qjackctlSessionSaveForm);
        SessionSaveVersionCheckBox->setObjectName("SessionSaveVersionCheckBox");

        hboxLayout2->addWidget(SessionSaveVersionCheckBox);

        DialogButtonBox = new QDialogButtonBox(qjackctlSessionSaveForm);
        DialogButtonBox->setObjectName("DialogButtonBox");
        DialogButtonBox->setOrientation(Qt::Horizontal);
        DialogButtonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        hboxLayout2->addWidget(DialogButtonBox);


        vboxLayout->addLayout(hboxLayout2);

#if QT_CONFIG(shortcut)
        SessionNameTextLabel->setBuddy(SessionNameLineEdit);
        SessionDirTextLabel->setBuddy(SessionDirComboBox);
#endif // QT_CONFIG(shortcut)
        QWidget::setTabOrder(SessionNameLineEdit, SessionDirComboBox);
        QWidget::setTabOrder(SessionDirComboBox, SessionDirToolButton);
        QWidget::setTabOrder(SessionDirToolButton, DialogButtonBox);

        retranslateUi(qjackctlSessionSaveForm);

        QMetaObject::connectSlotsByName(qjackctlSessionSaveForm);
    } // setupUi

    void retranslateUi(QDialog *qjackctlSessionSaveForm)
    {
        qjackctlSessionSaveForm->setWindowTitle(QCoreApplication::translate("qjackctlSessionSaveForm", "Session", nullptr));
        SessionNameTextLabel->setText(QCoreApplication::translate("qjackctlSessionSaveForm", "&Name:", nullptr));
#if QT_CONFIG(tooltip)
        SessionNameLineEdit->setToolTip(QCoreApplication::translate("qjackctlSessionSaveForm", "Session name", nullptr));
#endif // QT_CONFIG(tooltip)
        SessionDirTextLabel->setText(QCoreApplication::translate("qjackctlSessionSaveForm", "&Directory:", nullptr));
#if QT_CONFIG(tooltip)
        SessionDirComboBox->setToolTip(QCoreApplication::translate("qjackctlSessionSaveForm", "Session directory", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        SessionDirToolButton->setToolTip(QCoreApplication::translate("qjackctlSessionSaveForm", "Browse for session directory", nullptr));
#endif // QT_CONFIG(tooltip)
        SessionDirToolButton->setText(QCoreApplication::translate("qjackctlSessionSaveForm", "...", nullptr));
#if QT_CONFIG(tooltip)
        SessionSaveVersionCheckBox->setToolTip(QCoreApplication::translate("qjackctlSessionSaveForm", "Save session versioning", nullptr));
#endif // QT_CONFIG(tooltip)
        SessionSaveVersionCheckBox->setText(QCoreApplication::translate("qjackctlSessionSaveForm", "&Versioning", nullptr));
    } // retranslateUi

};

namespace Ui {
    class qjackctlSessionSaveForm: public Ui_qjackctlSessionSaveForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QJACKCTLSESSIONSAVEFORM_H
