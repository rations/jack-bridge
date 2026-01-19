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
** Form generated from reading UI file 'qjackctlSocketForm.ui'
**
** Created by: Qt User Interface Compiler version 6.4.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QJACKCTLSOCKETFORM_H
#define UI_QJACKCTLSOCKETFORM_H

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
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_qjackctlSocketForm
{
public:
    QVBoxLayout *vboxLayout;
    QTabWidget *SocketTabWidget;
    QWidget *tab;
    QGridLayout *gridLayout;
    QLabel *SocketNameTextLabel;
    QLineEdit *SocketNameLineEdit;
    QComboBox *ClientNameComboBox;
    QPushButton *PlugAddPushButton;
    QLabel *PlugNameTextLabel;
    QComboBox *PlugNameComboBox;
    QTreeWidget *PlugListView;
    QPushButton *PlugEditPushButton;
    QPushButton *PlugRemovePushButton;
    QLabel *ClientNameTextLabel;
    QPushButton *PlugDownPushButton;
    QPushButton *PlugUpPushButton;
    QSpacerItem *spacerItem;
    QCheckBox *ExclusiveCheckBox;
    QLabel *SocketForwardTextLabel;
    QComboBox *SocketForwardComboBox;
    QGroupBox *SocketTypeGroupBox;
    QHBoxLayout *hboxLayout;
    QRadioButton *AudioRadioButton;
    QRadioButton *MidiRadioButton;
    QRadioButton *AlsaRadioButton;
    QDialogButtonBox *DialogButtonBox;

    void setupUi(QDialog *qjackctlSocketForm)
    {
        if (qjackctlSocketForm->objectName().isEmpty())
            qjackctlSocketForm->setObjectName("qjackctlSocketForm");
        qjackctlSocketForm->resize(400, 300);
        const QIcon icon = QIcon(QString::fromUtf8(":/images/patchbay1.png"));
        qjackctlSocketForm->setWindowIcon(icon);
        vboxLayout = new QVBoxLayout(qjackctlSocketForm);
        vboxLayout->setSpacing(6);
        vboxLayout->setContentsMargins(9, 9, 9, 9);
        vboxLayout->setObjectName("vboxLayout");
        SocketTabWidget = new QTabWidget(qjackctlSocketForm);
        SocketTabWidget->setObjectName("SocketTabWidget");
        SocketTabWidget->setTabShape(QTabWidget::Rounded);
        tab = new QWidget();
        tab->setObjectName("tab");
        gridLayout = new QGridLayout(tab);
        gridLayout->setSpacing(4);
        gridLayout->setContentsMargins(8, 8, 8, 8);
        gridLayout->setObjectName("gridLayout");
        SocketNameTextLabel = new QLabel(tab);
        SocketNameTextLabel->setObjectName("SocketNameTextLabel");
        SocketNameTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        SocketNameTextLabel->setWordWrap(false);

        gridLayout->addWidget(SocketNameTextLabel, 0, 0, 1, 1);

        SocketNameLineEdit = new QLineEdit(tab);
        SocketNameLineEdit->setObjectName("SocketNameLineEdit");

        gridLayout->addWidget(SocketNameLineEdit, 0, 1, 1, 2);

        ClientNameComboBox = new QComboBox(tab);
        ClientNameComboBox->setObjectName("ClientNameComboBox");
        ClientNameComboBox->setEditable(true);

        gridLayout->addWidget(ClientNameComboBox, 2, 1, 1, 2);

        PlugAddPushButton = new QPushButton(tab);
        PlugAddPushButton->setObjectName("PlugAddPushButton");
        const QIcon icon1 = QIcon(QString::fromUtf8(":/images/add1.png"));
        PlugAddPushButton->setIcon(icon1);

        gridLayout->addWidget(PlugAddPushButton, 3, 2, 1, 1);

        PlugNameTextLabel = new QLabel(tab);
        PlugNameTextLabel->setObjectName("PlugNameTextLabel");
        PlugNameTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        PlugNameTextLabel->setWordWrap(false);

        gridLayout->addWidget(PlugNameTextLabel, 3, 0, 1, 1);

        PlugNameComboBox = new QComboBox(tab);
        PlugNameComboBox->setObjectName("PlugNameComboBox");
        PlugNameComboBox->setEditable(true);

        gridLayout->addWidget(PlugNameComboBox, 3, 1, 1, 1);

        PlugListView = new QTreeWidget(tab);
        PlugListView->setObjectName("PlugListView");
        PlugListView->setContextMenuPolicy(Qt::CustomContextMenu);
        PlugListView->setRootIsDecorated(false);
        PlugListView->setUniformRowHeights(true);
        PlugListView->setItemsExpandable(false);
        PlugListView->setAllColumnsShowFocus(true);

        gridLayout->addWidget(PlugListView, 4, 1, 5, 1);

        PlugEditPushButton = new QPushButton(tab);
        PlugEditPushButton->setObjectName("PlugEditPushButton");
        const QIcon icon2 = QIcon(QString::fromUtf8(":/images/edit1.png"));
        PlugEditPushButton->setIcon(icon2);

        gridLayout->addWidget(PlugEditPushButton, 4, 2, 1, 1);

        PlugRemovePushButton = new QPushButton(tab);
        PlugRemovePushButton->setObjectName("PlugRemovePushButton");
        const QIcon icon3 = QIcon(QString::fromUtf8(":/images/remove1.png"));
        PlugRemovePushButton->setIcon(icon3);

        gridLayout->addWidget(PlugRemovePushButton, 5, 2, 1, 1);

        ClientNameTextLabel = new QLabel(tab);
        ClientNameTextLabel->setObjectName("ClientNameTextLabel");
        ClientNameTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        ClientNameTextLabel->setWordWrap(false);

        gridLayout->addWidget(ClientNameTextLabel, 2, 0, 1, 1);

        PlugDownPushButton = new QPushButton(tab);
        PlugDownPushButton->setObjectName("PlugDownPushButton");
        const QIcon icon4 = QIcon(QString::fromUtf8(":/images/down1.png"));
        PlugDownPushButton->setIcon(icon4);

        gridLayout->addWidget(PlugDownPushButton, 8, 2, 1, 1);

        PlugUpPushButton = new QPushButton(tab);
        PlugUpPushButton->setObjectName("PlugUpPushButton");
        const QIcon icon5 = QIcon(QString::fromUtf8(":/images/up1.png"));
        PlugUpPushButton->setIcon(icon5);

        gridLayout->addWidget(PlugUpPushButton, 7, 2, 1, 1);

        spacerItem = new QSpacerItem(8, 8, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(spacerItem, 6, 2, 1, 1);

        ExclusiveCheckBox = new QCheckBox(tab);
        ExclusiveCheckBox->setObjectName("ExclusiveCheckBox");

        gridLayout->addWidget(ExclusiveCheckBox, 9, 1, 1, 1);

        SocketForwardTextLabel = new QLabel(tab);
        SocketForwardTextLabel->setObjectName("SocketForwardTextLabel");
        SocketForwardTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        SocketForwardTextLabel->setWordWrap(false);

        gridLayout->addWidget(SocketForwardTextLabel, 10, 0, 1, 1);

        SocketForwardComboBox = new QComboBox(tab);
        SocketForwardComboBox->setObjectName("SocketForwardComboBox");
        SocketForwardComboBox->setEditable(false);

        gridLayout->addWidget(SocketForwardComboBox, 10, 1, 1, 2);

        SocketTypeGroupBox = new QGroupBox(tab);
        SocketTypeGroupBox->setObjectName("SocketTypeGroupBox");
        hboxLayout = new QHBoxLayout(SocketTypeGroupBox);
        hboxLayout->setSpacing(4);
        hboxLayout->setContentsMargins(8, 8, 8, 8);
        hboxLayout->setObjectName("hboxLayout");
        AudioRadioButton = new QRadioButton(SocketTypeGroupBox);
        AudioRadioButton->setObjectName("AudioRadioButton");

        hboxLayout->addWidget(AudioRadioButton);

        MidiRadioButton = new QRadioButton(SocketTypeGroupBox);
        MidiRadioButton->setObjectName("MidiRadioButton");

        hboxLayout->addWidget(MidiRadioButton);

        AlsaRadioButton = new QRadioButton(SocketTypeGroupBox);
        AlsaRadioButton->setObjectName("AlsaRadioButton");

        hboxLayout->addWidget(AlsaRadioButton);


        gridLayout->addWidget(SocketTypeGroupBox, 1, 1, 1, 2);

        SocketTabWidget->addTab(tab, QString());

        vboxLayout->addWidget(SocketTabWidget);

        DialogButtonBox = new QDialogButtonBox(qjackctlSocketForm);
        DialogButtonBox->setObjectName("DialogButtonBox");
        DialogButtonBox->setOrientation(Qt::Horizontal);
        DialogButtonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        vboxLayout->addWidget(DialogButtonBox);

#if QT_CONFIG(shortcut)
        SocketNameTextLabel->setBuddy(SocketNameLineEdit);
        PlugNameTextLabel->setBuddy(PlugNameComboBox);
        ClientNameTextLabel->setBuddy(ClientNameComboBox);
        SocketForwardTextLabel->setBuddy(SocketForwardComboBox);
#endif // QT_CONFIG(shortcut)
        QWidget::setTabOrder(SocketTabWidget, SocketNameLineEdit);
        QWidget::setTabOrder(SocketNameLineEdit, AudioRadioButton);
        QWidget::setTabOrder(AudioRadioButton, MidiRadioButton);
        QWidget::setTabOrder(MidiRadioButton, AlsaRadioButton);
        QWidget::setTabOrder(AlsaRadioButton, ClientNameComboBox);
        QWidget::setTabOrder(ClientNameComboBox, PlugNameComboBox);
        QWidget::setTabOrder(PlugNameComboBox, PlugAddPushButton);
        QWidget::setTabOrder(PlugAddPushButton, PlugListView);
        QWidget::setTabOrder(PlugListView, PlugEditPushButton);
        QWidget::setTabOrder(PlugEditPushButton, PlugRemovePushButton);
        QWidget::setTabOrder(PlugRemovePushButton, PlugUpPushButton);
        QWidget::setTabOrder(PlugUpPushButton, PlugDownPushButton);
        QWidget::setTabOrder(PlugDownPushButton, ExclusiveCheckBox);
        QWidget::setTabOrder(ExclusiveCheckBox, SocketForwardComboBox);
        QWidget::setTabOrder(SocketForwardComboBox, DialogButtonBox);

        retranslateUi(qjackctlSocketForm);

        QMetaObject::connectSlotsByName(qjackctlSocketForm);
    } // setupUi

    void retranslateUi(QDialog *qjackctlSocketForm)
    {
        qjackctlSocketForm->setWindowTitle(QCoreApplication::translate("qjackctlSocketForm", "Socket", nullptr));
        SocketNameTextLabel->setText(QCoreApplication::translate("qjackctlSocketForm", "&Name (alias):", nullptr));
#if QT_CONFIG(tooltip)
        SocketNameLineEdit->setToolTip(QCoreApplication::translate("qjackctlSocketForm", "Socket name (an alias for client name)", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        ClientNameComboBox->setToolTip(QCoreApplication::translate("qjackctlSocketForm", "Client name (regular expression)", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        PlugAddPushButton->setToolTip(QCoreApplication::translate("qjackctlSocketForm", "Add plug to socket plug list", nullptr));
#endif // QT_CONFIG(tooltip)
        PlugAddPushButton->setText(QCoreApplication::translate("qjackctlSocketForm", "Add P&lug", nullptr));
        PlugNameTextLabel->setText(QCoreApplication::translate("qjackctlSocketForm", "&Plug:", nullptr));
#if QT_CONFIG(tooltip)
        PlugNameComboBox->setToolTip(QCoreApplication::translate("qjackctlSocketForm", "Port name (regular expression)", nullptr));
#endif // QT_CONFIG(tooltip)
        QTreeWidgetItem *___qtreewidgetitem = PlugListView->headerItem();
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("qjackctlSocketForm", "Socket Plugs / Ports", nullptr));
#if QT_CONFIG(tooltip)
        PlugListView->setToolTip(QCoreApplication::translate("qjackctlSocketForm", "Socket plug list", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        PlugEditPushButton->setToolTip(QCoreApplication::translate("qjackctlSocketForm", "Edit currently selected plug", nullptr));
#endif // QT_CONFIG(tooltip)
        PlugEditPushButton->setText(QCoreApplication::translate("qjackctlSocketForm", "&Edit", nullptr));
#if QT_CONFIG(tooltip)
        PlugRemovePushButton->setToolTip(QCoreApplication::translate("qjackctlSocketForm", "Remove currently selected plug from socket plug list", nullptr));
#endif // QT_CONFIG(tooltip)
        PlugRemovePushButton->setText(QCoreApplication::translate("qjackctlSocketForm", "&Remove", nullptr));
        ClientNameTextLabel->setText(QCoreApplication::translate("qjackctlSocketForm", "&Client:", nullptr));
#if QT_CONFIG(tooltip)
        PlugDownPushButton->setToolTip(QCoreApplication::translate("qjackctlSocketForm", "Move down currently selected plug in socket plug list", nullptr));
#endif // QT_CONFIG(tooltip)
        PlugDownPushButton->setText(QCoreApplication::translate("qjackctlSocketForm", "&Down", nullptr));
#if QT_CONFIG(tooltip)
        PlugUpPushButton->setToolTip(QCoreApplication::translate("qjackctlSocketForm", "Move up current selected plug in socket plug list", nullptr));
#endif // QT_CONFIG(tooltip)
        PlugUpPushButton->setText(QCoreApplication::translate("qjackctlSocketForm", "&Up", nullptr));
#if QT_CONFIG(tooltip)
        ExclusiveCheckBox->setToolTip(QCoreApplication::translate("qjackctlSocketForm", "Enforce only one exclusive cable connection", nullptr));
#endif // QT_CONFIG(tooltip)
        ExclusiveCheckBox->setText(QCoreApplication::translate("qjackctlSocketForm", "E&xclusive", nullptr));
        SocketForwardTextLabel->setText(QCoreApplication::translate("qjackctlSocketForm", "&Forward:", nullptr));
#if QT_CONFIG(tooltip)
        SocketForwardComboBox->setToolTip(QCoreApplication::translate("qjackctlSocketForm", "Forward (clone) all connections from this socket", nullptr));
#endif // QT_CONFIG(tooltip)
        SocketTypeGroupBox->setTitle(QCoreApplication::translate("qjackctlSocketForm", "Type", nullptr));
#if QT_CONFIG(tooltip)
        AudioRadioButton->setToolTip(QCoreApplication::translate("qjackctlSocketForm", "Audio socket type (JACK)", nullptr));
#endif // QT_CONFIG(tooltip)
        AudioRadioButton->setText(QCoreApplication::translate("qjackctlSocketForm", "&Audio", nullptr));
#if QT_CONFIG(tooltip)
        MidiRadioButton->setToolTip(QCoreApplication::translate("qjackctlSocketForm", "MIDI socket type (JACK)", nullptr));
#endif // QT_CONFIG(tooltip)
        MidiRadioButton->setText(QCoreApplication::translate("qjackctlSocketForm", "&MIDI", nullptr));
#if QT_CONFIG(tooltip)
        AlsaRadioButton->setToolTip(QCoreApplication::translate("qjackctlSocketForm", "MIDI socket type (ALSA)", nullptr));
#endif // QT_CONFIG(tooltip)
        AlsaRadioButton->setText(QCoreApplication::translate("qjackctlSocketForm", "AL&SA", nullptr));
        SocketTabWidget->setTabText(SocketTabWidget->indexOf(tab), QCoreApplication::translate("qjackctlSocketForm", "&Socket", nullptr));
    } // retranslateUi

};

namespace Ui {
    class qjackctlSocketForm: public Ui_qjackctlSocketForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QJACKCTLSOCKETFORM_H
