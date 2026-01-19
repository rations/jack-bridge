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
** Form generated from reading UI file 'qjackctlSessionForm.ui'
**
** Created by: Qt User Interface Compiler version 6.4.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QJACKCTLSESSIONFORM_H
#define UI_QJACKCTLSESSIONFORM_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_qjackctlSessionForm
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QPushButton *LoadSessionPushButton;
    QPushButton *RecentSessionPushButton;
    QSpacerItem *spacerItem;
    QPushButton *SaveSessionPushButton;
    QSpacerItem *spacerItem1;
    QPushButton *UpdateSessionPushButton;
    QSplitter *InfraClientSplitter;
    QTreeWidget *SessionTreeView;
    QWidget *InfraClientWidget;
    QGridLayout *gridLayout;
    QTreeWidget *InfraClientListView;
    QPushButton *AddInfraClientPushButton;
    QPushButton *EditInfraClientPushButton;
    QPushButton *RemoveInfraClientPushButton;
    QSpacerItem *spacerItem2;

    void setupUi(QWidget *qjackctlSessionForm)
    {
        if (qjackctlSessionForm->objectName().isEmpty())
            qjackctlSessionForm->setObjectName("qjackctlSessionForm");
        qjackctlSessionForm->resize(480, 320);
        const QIcon icon = QIcon(QString::fromUtf8(":/images/session1.png"));
        qjackctlSessionForm->setWindowIcon(icon);
        verticalLayout = new QVBoxLayout(qjackctlSessionForm);
        verticalLayout->setSpacing(4);
        verticalLayout->setContentsMargins(4, 4, 4, 4);
        verticalLayout->setObjectName("verticalLayout");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(4);
        horizontalLayout->setObjectName("horizontalLayout");
        LoadSessionPushButton = new QPushButton(qjackctlSessionForm);
        LoadSessionPushButton->setObjectName("LoadSessionPushButton");
        const QIcon icon1 = QIcon(QString::fromUtf8(":/images/open1.png"));
        LoadSessionPushButton->setIcon(icon1);

        horizontalLayout->addWidget(LoadSessionPushButton);

        RecentSessionPushButton = new QPushButton(qjackctlSessionForm);
        RecentSessionPushButton->setObjectName("RecentSessionPushButton");

        horizontalLayout->addWidget(RecentSessionPushButton);

        spacerItem = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(spacerItem);

        SaveSessionPushButton = new QPushButton(qjackctlSessionForm);
        SaveSessionPushButton->setObjectName("SaveSessionPushButton");
        const QIcon icon2 = QIcon(QString::fromUtf8(":/images/save1.png"));
        SaveSessionPushButton->setIcon(icon2);

        horizontalLayout->addWidget(SaveSessionPushButton);

        spacerItem1 = new QSpacerItem(220, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(spacerItem1);

        UpdateSessionPushButton = new QPushButton(qjackctlSessionForm);
        UpdateSessionPushButton->setObjectName("UpdateSessionPushButton");
        const QIcon icon3 = QIcon(QString::fromUtf8(":/images/refresh1.png"));
        UpdateSessionPushButton->setIcon(icon3);

        horizontalLayout->addWidget(UpdateSessionPushButton);


        verticalLayout->addLayout(horizontalLayout);

        InfraClientSplitter = new QSplitter(qjackctlSessionForm);
        InfraClientSplitter->setObjectName("InfraClientSplitter");
        InfraClientSplitter->setOrientation(Qt::Vertical);
        SessionTreeView = new QTreeWidget(InfraClientSplitter);
        SessionTreeView->setObjectName("SessionTreeView");
        SessionTreeView->setRootIsDecorated(false);
        SessionTreeView->setUniformRowHeights(true);
        SessionTreeView->setSortingEnabled(false);
        SessionTreeView->setAllColumnsShowFocus(true);
        InfraClientSplitter->addWidget(SessionTreeView);
        InfraClientWidget = new QWidget(InfraClientSplitter);
        InfraClientWidget->setObjectName("InfraClientWidget");
        gridLayout = new QGridLayout(InfraClientWidget);
        gridLayout->setSpacing(4);
        gridLayout->setContentsMargins(4, 4, 4, 4);
        gridLayout->setObjectName("gridLayout");
        gridLayout->setContentsMargins(0, 0, 0, 0);
        InfraClientListView = new QTreeWidget(InfraClientWidget);
        InfraClientListView->setObjectName("InfraClientListView");
        InfraClientListView->setRootIsDecorated(false);
        InfraClientListView->setUniformRowHeights(true);
        InfraClientListView->setSortingEnabled(false);
        InfraClientListView->setAllColumnsShowFocus(true);

        gridLayout->addWidget(InfraClientListView, 0, 0, 4, 1);

        AddInfraClientPushButton = new QPushButton(InfraClientWidget);
        AddInfraClientPushButton->setObjectName("AddInfraClientPushButton");
        const QIcon icon4 = QIcon(QString::fromUtf8(":/images/add1.png"));
        AddInfraClientPushButton->setIcon(icon4);

        gridLayout->addWidget(AddInfraClientPushButton, 0, 1, 1, 1);

        EditInfraClientPushButton = new QPushButton(InfraClientWidget);
        EditInfraClientPushButton->setObjectName("EditInfraClientPushButton");
        const QIcon icon5 = QIcon(QString::fromUtf8(":/images/edit1.png"));
        EditInfraClientPushButton->setIcon(icon5);

        gridLayout->addWidget(EditInfraClientPushButton, 1, 1, 1, 1);

        RemoveInfraClientPushButton = new QPushButton(InfraClientWidget);
        RemoveInfraClientPushButton->setObjectName("RemoveInfraClientPushButton");
        const QIcon icon6 = QIcon(QString::fromUtf8(":/images/remove1.png"));
        RemoveInfraClientPushButton->setIcon(icon6);

        gridLayout->addWidget(RemoveInfraClientPushButton, 2, 1, 1, 1);

        spacerItem2 = new QSpacerItem(20, 8, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(spacerItem2, 3, 1, 1, 1);

        InfraClientSplitter->addWidget(InfraClientWidget);

        verticalLayout->addWidget(InfraClientSplitter);

        QWidget::setTabOrder(LoadSessionPushButton, RecentSessionPushButton);
        QWidget::setTabOrder(RecentSessionPushButton, SaveSessionPushButton);
        QWidget::setTabOrder(SaveSessionPushButton, UpdateSessionPushButton);
        QWidget::setTabOrder(UpdateSessionPushButton, SessionTreeView);
        QWidget::setTabOrder(SessionTreeView, InfraClientListView);
        QWidget::setTabOrder(InfraClientListView, AddInfraClientPushButton);
        QWidget::setTabOrder(AddInfraClientPushButton, EditInfraClientPushButton);
        QWidget::setTabOrder(EditInfraClientPushButton, RemoveInfraClientPushButton);

        retranslateUi(qjackctlSessionForm);

        QMetaObject::connectSlotsByName(qjackctlSessionForm);
    } // setupUi

    void retranslateUi(QWidget *qjackctlSessionForm)
    {
        qjackctlSessionForm->setWindowTitle(QCoreApplication::translate("qjackctlSessionForm", "Session", nullptr));
#if QT_CONFIG(tooltip)
        LoadSessionPushButton->setToolTip(QCoreApplication::translate("qjackctlSessionForm", "Load session", nullptr));
#endif // QT_CONFIG(tooltip)
        LoadSessionPushButton->setText(QCoreApplication::translate("qjackctlSessionForm", "&Load...", nullptr));
#if QT_CONFIG(tooltip)
        RecentSessionPushButton->setToolTip(QCoreApplication::translate("qjackctlSessionForm", "Recent session", nullptr));
#endif // QT_CONFIG(tooltip)
        RecentSessionPushButton->setText(QCoreApplication::translate("qjackctlSessionForm", "&Recent", nullptr));
#if QT_CONFIG(tooltip)
        SaveSessionPushButton->setToolTip(QCoreApplication::translate("qjackctlSessionForm", "Save session", nullptr));
#endif // QT_CONFIG(tooltip)
        SaveSessionPushButton->setText(QCoreApplication::translate("qjackctlSessionForm", "&Save", nullptr));
#if QT_CONFIG(tooltip)
        UpdateSessionPushButton->setToolTip(QCoreApplication::translate("qjackctlSessionForm", "Update session", nullptr));
#endif // QT_CONFIG(tooltip)
        UpdateSessionPushButton->setText(QCoreApplication::translate("qjackctlSessionForm", "Re&fresh", nullptr));
        QTreeWidgetItem *___qtreewidgetitem = SessionTreeView->headerItem();
        ___qtreewidgetitem->setText(2, QCoreApplication::translate("qjackctlSessionForm", "Command", nullptr));
        ___qtreewidgetitem->setText(1, QCoreApplication::translate("qjackctlSessionForm", "UUID", nullptr));
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("qjackctlSessionForm", "Client / Ports", nullptr));
#if QT_CONFIG(tooltip)
        SessionTreeView->setToolTip(QCoreApplication::translate("qjackctlSessionForm", "Session clients / connections", nullptr));
#endif // QT_CONFIG(tooltip)
        QTreeWidgetItem *___qtreewidgetitem1 = InfraClientListView->headerItem();
        ___qtreewidgetitem1->setText(1, QCoreApplication::translate("qjackctlSessionForm", "Infra-command", nullptr));
        ___qtreewidgetitem1->setText(0, QCoreApplication::translate("qjackctlSessionForm", "Infra-client", nullptr));
#if QT_CONFIG(tooltip)
        InfraClientListView->setToolTip(QCoreApplication::translate("qjackctlSessionForm", "Infra-clients / commands", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        AddInfraClientPushButton->setToolTip(QCoreApplication::translate("qjackctlSessionForm", "Add infra-client", nullptr));
#endif // QT_CONFIG(tooltip)
        AddInfraClientPushButton->setText(QCoreApplication::translate("qjackctlSessionForm", "&Add", nullptr));
#if QT_CONFIG(tooltip)
        EditInfraClientPushButton->setToolTip(QCoreApplication::translate("qjackctlSessionForm", "Edit infra-client", nullptr));
#endif // QT_CONFIG(tooltip)
        EditInfraClientPushButton->setText(QCoreApplication::translate("qjackctlSessionForm", "&Edit", nullptr));
#if QT_CONFIG(tooltip)
        RemoveInfraClientPushButton->setToolTip(QCoreApplication::translate("qjackctlSessionForm", "Remove infra-client", nullptr));
#endif // QT_CONFIG(tooltip)
        RemoveInfraClientPushButton->setText(QCoreApplication::translate("qjackctlSessionForm", "Re&move", nullptr));
    } // retranslateUi

};

namespace Ui {
    class qjackctlSessionForm: public Ui_qjackctlSessionForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QJACKCTLSESSIONFORM_H
