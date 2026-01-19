/*
JACK Audio Connection Kit - Qt GUI Interface.

   Copyright (C) 2003-2021, rncbc aka Rui Nuno Capela. All rights reserved.

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
** Form generated from reading UI file 'qjackctlConnectionsForm.ui'
**
** Created by: Qt User Interface Compiler version 6.4.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QJACKCTLCONNECTIONSFORM_H
#define UI_QJACKCTLCONNECTIONSFORM_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QWidget>
#include "qjackctlConnect.h"

QT_BEGIN_NAMESPACE

class Ui_qjackctlConnectionsForm
{
public:
    QGridLayout *gridLayout;
    QTabWidget *ConnectionsTabWidget;
    QWidget *AudioConnectTab;
    QGridLayout *gridLayout1;
    qjackctlConnectView *AudioConnectView;
    QHBoxLayout *hboxLayout;
    QPushButton *AudioConnectPushButton;
    QPushButton *AudioDisconnectPushButton;
    QPushButton *AudioDisconnectAllPushButton;
    QSpacerItem *spacerItem;
    QPushButton *AudioExpandAllPushButton;
    QSpacerItem *spacerItem1;
    QPushButton *AudioRefreshPushButton;
    QWidget *MidiConnectTab;
    QGridLayout *gridLayout2;
    qjackctlConnectView *MidiConnectView;
    QHBoxLayout *hboxLayout1;
    QPushButton *MidiConnectPushButton;
    QPushButton *MidiDisconnectPushButton;
    QPushButton *MidiDisconnectAllPushButton;
    QSpacerItem *spacerItem2;
    QPushButton *MidiExpandAllPushButton;
    QSpacerItem *spacerItem3;
    QPushButton *MidiRefreshPushButton;
    QWidget *AlsaConnectTab;
    QGridLayout *gridLayout3;
    qjackctlConnectView *AlsaConnectView;
    QHBoxLayout *hboxLayout2;
    QPushButton *AlsaConnectPushButton;
    QPushButton *AlsaDisconnectPushButton;
    QPushButton *AlsaDisconnectAllPushButton;
    QSpacerItem *spacerItem4;
    QPushButton *AlsaExpandAllPushButton;
    QSpacerItem *spacerItem5;
    QPushButton *AlsaRefreshPushButton;

    void setupUi(QWidget *qjackctlConnectionsForm)
    {
        if (qjackctlConnectionsForm->objectName().isEmpty())
            qjackctlConnectionsForm->setObjectName("qjackctlConnectionsForm");
        qjackctlConnectionsForm->resize(480, 320);
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(qjackctlConnectionsForm->sizePolicy().hasHeightForWidth());
        qjackctlConnectionsForm->setSizePolicy(sizePolicy);
        QFont font;
        qjackctlConnectionsForm->setFont(font);
        const QIcon icon = QIcon(QString::fromUtf8(":/images/connections1.png"));
        qjackctlConnectionsForm->setWindowIcon(icon);
        gridLayout = new QGridLayout(qjackctlConnectionsForm);
        gridLayout->setSpacing(4);
        gridLayout->setContentsMargins(4, 4, 4, 4);
        gridLayout->setObjectName("gridLayout");
        ConnectionsTabWidget = new QTabWidget(qjackctlConnectionsForm);
        ConnectionsTabWidget->setObjectName("ConnectionsTabWidget");
        AudioConnectTab = new QWidget();
        AudioConnectTab->setObjectName("AudioConnectTab");
        gridLayout1 = new QGridLayout(AudioConnectTab);
        gridLayout1->setSpacing(4);
        gridLayout1->setContentsMargins(4, 4, 4, 4);
        gridLayout1->setObjectName("gridLayout1");
        AudioConnectView = new qjackctlConnectView(AudioConnectTab);
        AudioConnectView->setObjectName("AudioConnectView");
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(AudioConnectView->sizePolicy().hasHeightForWidth());
        AudioConnectView->setSizePolicy(sizePolicy1);
        AudioConnectView->setFocusPolicy(Qt::TabFocus);

        gridLayout1->addWidget(AudioConnectView, 0, 0, 1, 1);

        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(4);
        hboxLayout->setContentsMargins(4, 4, 4, 4);
        hboxLayout->setObjectName("hboxLayout");
        AudioConnectPushButton = new QPushButton(AudioConnectTab);
        AudioConnectPushButton->setObjectName("AudioConnectPushButton");
        const QIcon icon1 = QIcon(QString::fromUtf8(":/images/connect1.png"));
        AudioConnectPushButton->setIcon(icon1);

        hboxLayout->addWidget(AudioConnectPushButton);

        AudioDisconnectPushButton = new QPushButton(AudioConnectTab);
        AudioDisconnectPushButton->setObjectName("AudioDisconnectPushButton");
        const QIcon icon2 = QIcon(QString::fromUtf8(":/images/disconnect1.png"));
        AudioDisconnectPushButton->setIcon(icon2);

        hboxLayout->addWidget(AudioDisconnectPushButton);

        AudioDisconnectAllPushButton = new QPushButton(AudioConnectTab);
        AudioDisconnectAllPushButton->setObjectName("AudioDisconnectAllPushButton");
        const QIcon icon3 = QIcon(QString::fromUtf8(":/images/disconnectall1.png"));
        AudioDisconnectAllPushButton->setIcon(icon3);

        hboxLayout->addWidget(AudioDisconnectAllPushButton);

        spacerItem = new QSpacerItem(8, 8, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem);

        AudioExpandAllPushButton = new QPushButton(AudioConnectTab);
        AudioExpandAllPushButton->setObjectName("AudioExpandAllPushButton");
        const QIcon icon4 = QIcon(QString::fromUtf8(":/images/expandall1.png"));
        AudioExpandAllPushButton->setIcon(icon4);

        hboxLayout->addWidget(AudioExpandAllPushButton);

        spacerItem1 = new QSpacerItem(8, 8, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem1);

        AudioRefreshPushButton = new QPushButton(AudioConnectTab);
        AudioRefreshPushButton->setObjectName("AudioRefreshPushButton");
        const QIcon icon5 = QIcon(QString::fromUtf8(":/images/refresh1.png"));
        AudioRefreshPushButton->setIcon(icon5);

        hboxLayout->addWidget(AudioRefreshPushButton);


        gridLayout1->addLayout(hboxLayout, 1, 0, 1, 1);

        ConnectionsTabWidget->addTab(AudioConnectTab, QString());
        MidiConnectTab = new QWidget();
        MidiConnectTab->setObjectName("MidiConnectTab");
        gridLayout2 = new QGridLayout(MidiConnectTab);
        gridLayout2->setSpacing(4);
        gridLayout2->setContentsMargins(4, 4, 4, 4);
        gridLayout2->setObjectName("gridLayout2");
        MidiConnectView = new qjackctlConnectView(MidiConnectTab);
        MidiConnectView->setObjectName("MidiConnectView");
        sizePolicy1.setHeightForWidth(MidiConnectView->sizePolicy().hasHeightForWidth());
        MidiConnectView->setSizePolicy(sizePolicy1);
        MidiConnectView->setFocusPolicy(Qt::TabFocus);

        gridLayout2->addWidget(MidiConnectView, 0, 0, 1, 1);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(4);
        hboxLayout1->setContentsMargins(4, 4, 4, 4);
        hboxLayout1->setObjectName("hboxLayout1");
        MidiConnectPushButton = new QPushButton(MidiConnectTab);
        MidiConnectPushButton->setObjectName("MidiConnectPushButton");
        MidiConnectPushButton->setIcon(icon1);

        hboxLayout1->addWidget(MidiConnectPushButton);

        MidiDisconnectPushButton = new QPushButton(MidiConnectTab);
        MidiDisconnectPushButton->setObjectName("MidiDisconnectPushButton");
        MidiDisconnectPushButton->setIcon(icon2);

        hboxLayout1->addWidget(MidiDisconnectPushButton);

        MidiDisconnectAllPushButton = new QPushButton(MidiConnectTab);
        MidiDisconnectAllPushButton->setObjectName("MidiDisconnectAllPushButton");
        MidiDisconnectAllPushButton->setIcon(icon3);

        hboxLayout1->addWidget(MidiDisconnectAllPushButton);

        spacerItem2 = new QSpacerItem(8, 8, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacerItem2);

        MidiExpandAllPushButton = new QPushButton(MidiConnectTab);
        MidiExpandAllPushButton->setObjectName("MidiExpandAllPushButton");
        MidiExpandAllPushButton->setIcon(icon4);

        hboxLayout1->addWidget(MidiExpandAllPushButton);

        spacerItem3 = new QSpacerItem(8, 8, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacerItem3);

        MidiRefreshPushButton = new QPushButton(MidiConnectTab);
        MidiRefreshPushButton->setObjectName("MidiRefreshPushButton");
        MidiRefreshPushButton->setIcon(icon5);

        hboxLayout1->addWidget(MidiRefreshPushButton);


        gridLayout2->addLayout(hboxLayout1, 1, 0, 1, 1);

        ConnectionsTabWidget->addTab(MidiConnectTab, QString());
        AlsaConnectTab = new QWidget();
        AlsaConnectTab->setObjectName("AlsaConnectTab");
        gridLayout3 = new QGridLayout(AlsaConnectTab);
        gridLayout3->setSpacing(4);
        gridLayout3->setContentsMargins(4, 4, 4, 4);
        gridLayout3->setObjectName("gridLayout3");
        AlsaConnectView = new qjackctlConnectView(AlsaConnectTab);
        AlsaConnectView->setObjectName("AlsaConnectView");
        sizePolicy1.setHeightForWidth(AlsaConnectView->sizePolicy().hasHeightForWidth());
        AlsaConnectView->setSizePolicy(sizePolicy1);
        AlsaConnectView->setFocusPolicy(Qt::TabFocus);

        gridLayout3->addWidget(AlsaConnectView, 0, 0, 1, 1);

        hboxLayout2 = new QHBoxLayout();
        hboxLayout2->setSpacing(4);
        hboxLayout2->setContentsMargins(4, 4, 4, 4);
        hboxLayout2->setObjectName("hboxLayout2");
        AlsaConnectPushButton = new QPushButton(AlsaConnectTab);
        AlsaConnectPushButton->setObjectName("AlsaConnectPushButton");
        AlsaConnectPushButton->setIcon(icon1);

        hboxLayout2->addWidget(AlsaConnectPushButton);

        AlsaDisconnectPushButton = new QPushButton(AlsaConnectTab);
        AlsaDisconnectPushButton->setObjectName("AlsaDisconnectPushButton");
        AlsaDisconnectPushButton->setIcon(icon2);

        hboxLayout2->addWidget(AlsaDisconnectPushButton);

        AlsaDisconnectAllPushButton = new QPushButton(AlsaConnectTab);
        AlsaDisconnectAllPushButton->setObjectName("AlsaDisconnectAllPushButton");
        AlsaDisconnectAllPushButton->setIcon(icon3);

        hboxLayout2->addWidget(AlsaDisconnectAllPushButton);

        spacerItem4 = new QSpacerItem(8, 8, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout2->addItem(spacerItem4);

        AlsaExpandAllPushButton = new QPushButton(AlsaConnectTab);
        AlsaExpandAllPushButton->setObjectName("AlsaExpandAllPushButton");
        AlsaExpandAllPushButton->setIcon(icon4);

        hboxLayout2->addWidget(AlsaExpandAllPushButton);

        spacerItem5 = new QSpacerItem(8, 8, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout2->addItem(spacerItem5);

        AlsaRefreshPushButton = new QPushButton(AlsaConnectTab);
        AlsaRefreshPushButton->setObjectName("AlsaRefreshPushButton");
        AlsaRefreshPushButton->setIcon(icon5);

        hboxLayout2->addWidget(AlsaRefreshPushButton);


        gridLayout3->addLayout(hboxLayout2, 1, 0, 1, 1);

        ConnectionsTabWidget->addTab(AlsaConnectTab, QString());

        gridLayout->addWidget(ConnectionsTabWidget, 0, 0, 1, 1);

        QWidget::setTabOrder(ConnectionsTabWidget, AudioConnectView);
        QWidget::setTabOrder(AudioConnectView, AudioConnectPushButton);
        QWidget::setTabOrder(AudioConnectPushButton, AudioDisconnectPushButton);
        QWidget::setTabOrder(AudioDisconnectPushButton, AudioDisconnectAllPushButton);
        QWidget::setTabOrder(AudioDisconnectAllPushButton, AudioExpandAllPushButton);
        QWidget::setTabOrder(AudioExpandAllPushButton, AudioRefreshPushButton);
        QWidget::setTabOrder(AudioRefreshPushButton, MidiConnectView);
        QWidget::setTabOrder(MidiConnectView, MidiConnectPushButton);
        QWidget::setTabOrder(MidiConnectPushButton, MidiDisconnectPushButton);
        QWidget::setTabOrder(MidiDisconnectPushButton, MidiDisconnectAllPushButton);
        QWidget::setTabOrder(MidiDisconnectAllPushButton, MidiExpandAllPushButton);
        QWidget::setTabOrder(MidiExpandAllPushButton, MidiRefreshPushButton);
        QWidget::setTabOrder(MidiRefreshPushButton, AlsaConnectView);
        QWidget::setTabOrder(AlsaConnectView, AlsaConnectPushButton);
        QWidget::setTabOrder(AlsaConnectPushButton, AlsaDisconnectPushButton);
        QWidget::setTabOrder(AlsaDisconnectPushButton, AlsaDisconnectAllPushButton);
        QWidget::setTabOrder(AlsaDisconnectAllPushButton, AlsaExpandAllPushButton);
        QWidget::setTabOrder(AlsaExpandAllPushButton, AlsaRefreshPushButton);

        retranslateUi(qjackctlConnectionsForm);

        ConnectionsTabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(qjackctlConnectionsForm);
    } // setupUi

    void retranslateUi(QWidget *qjackctlConnectionsForm)
    {
        qjackctlConnectionsForm->setWindowTitle(QCoreApplication::translate("qjackctlConnectionsForm", "Connections", nullptr));
#if QT_CONFIG(tooltip)
        AudioConnectPushButton->setToolTip(QCoreApplication::translate("qjackctlConnectionsForm", "Connect currently selected ports", nullptr));
#endif // QT_CONFIG(tooltip)
        AudioConnectPushButton->setText(QCoreApplication::translate("qjackctlConnectionsForm", "&Connect", nullptr));
#if QT_CONFIG(tooltip)
        AudioDisconnectPushButton->setToolTip(QCoreApplication::translate("qjackctlConnectionsForm", "Disconnect currently selected ports", nullptr));
#endif // QT_CONFIG(tooltip)
        AudioDisconnectPushButton->setText(QCoreApplication::translate("qjackctlConnectionsForm", "&Disconnect", nullptr));
#if QT_CONFIG(tooltip)
        AudioDisconnectAllPushButton->setToolTip(QCoreApplication::translate("qjackctlConnectionsForm", "Disconnect all currently connected ports", nullptr));
#endif // QT_CONFIG(tooltip)
        AudioDisconnectAllPushButton->setText(QCoreApplication::translate("qjackctlConnectionsForm", "Disconnect &All", nullptr));
#if QT_CONFIG(tooltip)
        AudioExpandAllPushButton->setToolTip(QCoreApplication::translate("qjackctlConnectionsForm", "Expand all client ports", nullptr));
#endif // QT_CONFIG(tooltip)
        AudioExpandAllPushButton->setText(QCoreApplication::translate("qjackctlConnectionsForm", "E&xpand All", nullptr));
#if QT_CONFIG(tooltip)
        AudioRefreshPushButton->setToolTip(QCoreApplication::translate("qjackctlConnectionsForm", "Refresh current connections view", nullptr));
#endif // QT_CONFIG(tooltip)
        AudioRefreshPushButton->setText(QCoreApplication::translate("qjackctlConnectionsForm", "&Refresh", nullptr));
        ConnectionsTabWidget->setTabText(ConnectionsTabWidget->indexOf(AudioConnectTab), QCoreApplication::translate("qjackctlConnectionsForm", "Audio", nullptr));
#if QT_CONFIG(tooltip)
        MidiConnectPushButton->setToolTip(QCoreApplication::translate("qjackctlConnectionsForm", "Connect currently selected ports", nullptr));
#endif // QT_CONFIG(tooltip)
        MidiConnectPushButton->setText(QCoreApplication::translate("qjackctlConnectionsForm", "&Connect", nullptr));
#if QT_CONFIG(tooltip)
        MidiDisconnectPushButton->setToolTip(QCoreApplication::translate("qjackctlConnectionsForm", "Disconnect currently selected ports", nullptr));
#endif // QT_CONFIG(tooltip)
        MidiDisconnectPushButton->setText(QCoreApplication::translate("qjackctlConnectionsForm", "&Disconnect", nullptr));
#if QT_CONFIG(tooltip)
        MidiDisconnectAllPushButton->setToolTip(QCoreApplication::translate("qjackctlConnectionsForm", "Disconnect all currently connected ports", nullptr));
#endif // QT_CONFIG(tooltip)
        MidiDisconnectAllPushButton->setText(QCoreApplication::translate("qjackctlConnectionsForm", "Disconnect &All", nullptr));
#if QT_CONFIG(tooltip)
        MidiExpandAllPushButton->setToolTip(QCoreApplication::translate("qjackctlConnectionsForm", "Expand all client ports", nullptr));
#endif // QT_CONFIG(tooltip)
        MidiExpandAllPushButton->setText(QCoreApplication::translate("qjackctlConnectionsForm", "E&xpand All", nullptr));
#if QT_CONFIG(tooltip)
        MidiRefreshPushButton->setToolTip(QCoreApplication::translate("qjackctlConnectionsForm", "Refresh current connections view", nullptr));
#endif // QT_CONFIG(tooltip)
        MidiRefreshPushButton->setText(QCoreApplication::translate("qjackctlConnectionsForm", "&Refresh", nullptr));
        ConnectionsTabWidget->setTabText(ConnectionsTabWidget->indexOf(MidiConnectTab), QCoreApplication::translate("qjackctlConnectionsForm", "MIDI", nullptr));
#if QT_CONFIG(tooltip)
        AlsaConnectPushButton->setToolTip(QCoreApplication::translate("qjackctlConnectionsForm", "Connect currently selected ports", nullptr));
#endif // QT_CONFIG(tooltip)
        AlsaConnectPushButton->setText(QCoreApplication::translate("qjackctlConnectionsForm", "&Connect", nullptr));
#if QT_CONFIG(tooltip)
        AlsaDisconnectPushButton->setToolTip(QCoreApplication::translate("qjackctlConnectionsForm", "Disconnect currently selected ports", nullptr));
#endif // QT_CONFIG(tooltip)
        AlsaDisconnectPushButton->setText(QCoreApplication::translate("qjackctlConnectionsForm", "&Disconnect", nullptr));
#if QT_CONFIG(tooltip)
        AlsaDisconnectAllPushButton->setToolTip(QCoreApplication::translate("qjackctlConnectionsForm", "Disconnect all currently connected ports", nullptr));
#endif // QT_CONFIG(tooltip)
        AlsaDisconnectAllPushButton->setText(QCoreApplication::translate("qjackctlConnectionsForm", "Disconnect &All", nullptr));
#if QT_CONFIG(tooltip)
        AlsaExpandAllPushButton->setToolTip(QCoreApplication::translate("qjackctlConnectionsForm", "Expand all client ports", nullptr));
#endif // QT_CONFIG(tooltip)
        AlsaExpandAllPushButton->setText(QCoreApplication::translate("qjackctlConnectionsForm", "E&xpand All", nullptr));
#if QT_CONFIG(tooltip)
        AlsaRefreshPushButton->setToolTip(QCoreApplication::translate("qjackctlConnectionsForm", "Refresh current connections view", nullptr));
#endif // QT_CONFIG(tooltip)
        AlsaRefreshPushButton->setText(QCoreApplication::translate("qjackctlConnectionsForm", "&Refresh", nullptr));
        ConnectionsTabWidget->setTabText(ConnectionsTabWidget->indexOf(AlsaConnectTab), QCoreApplication::translate("qjackctlConnectionsForm", "ALSA", nullptr));
    } // retranslateUi

};

namespace Ui {
    class qjackctlConnectionsForm: public Ui_qjackctlConnectionsForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QJACKCTLCONNECTIONSFORM_H
