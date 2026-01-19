/*
JACK Audio Connection Kit - Qt GUI Interface.

   Copyright (C) 2003-2024, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  S1ee the
   GNU General Public License for more details.


   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.


*/

/********************************************************************************
** Form generated from reading UI file 'qjackctlSetupForm.ui'
**
** Created by: Qt User Interface Compiler version 6.4.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QJACKCTLSETUPFORM_H
#define UI_QJACKCTLSETUPFORM_H

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
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "qjackctlInterfaceComboBox.h"

QT_BEGIN_NAMESPACE

class Ui_qjackctlSetupForm
{
public:
    QVBoxLayout *vboxLayout;
    QTabWidget *SetupTabWidget;
    QWidget *SettingsTabPage;
    QVBoxLayout *vboxLayout1;
    QHBoxLayout *hboxLayout;
    QLabel *PresetTextLabel;
    QComboBox *PresetComboBox;
    QPushButton *PresetClearPushButton;
    QPushButton *PresetSavePushButton;
    QPushButton *PresetDeletePushButton;
    QTabWidget *SettingsTabWidget;
    QWidget *ParametersTab;
    QVBoxLayout *vboxLayout2;
    QGridLayout *gridLayout;
    QLabel *DriverTextLabel;
    QComboBox *DriverComboBox;
    QSpacerItem *spacerItem;
    QLabel *InterfaceTextLabel;
    qjackctlInterfaceComboBox *InterfaceComboBox;
    QSpacerItem *spacerItem1;
    QLabel *MidiDriverTextLabel;
    QComboBox *MidiDriverComboBox;
    QCheckBox *RealtimeCheckBox;
    QLabel *SampleRateTextLabel;
    QComboBox *SampleRateComboBox;
    QSpacerItem *spacerItem2;
    QLabel *FramesTextLabel;
    QComboBox *FramesComboBox;
    QLabel *PeriodsTextLabel;
    QSpinBox *PeriodsSpinBox;
    QSpacerItem *spacerItem3;
    QCheckBox *SyncCheckBox;
    QSpacerItem *spacerItem4;
    QHBoxLayout *hboxLayout1;
    QCheckBox *VerboseCheckBox;
    QSpacerItem *spacerItem5;
    QLabel *LatencyTextLabel;
    QLabel *LatencyTextValue;
    QWidget *AdvancedTab;
    QGridLayout *gridLayout1;
    QHBoxLayout *hboxLayout2;
    QLabel *AdvancedIconLabel;
    QLabel *AdvancedTextLabel;
    QSpacerItem *spacerItem6;
    QHBoxLayout *hboxLayout3;
    QLabel *ServerPrefixTextLabel;
    QComboBox *ServerPrefixComboBox;
    QLabel *ServerNameTextLabel;
    QComboBox *ServerNameComboBox;
    QSpacerItem *spacerItem7;
    QVBoxLayout *vboxLayout3;
    QCheckBox *NoMemLockCheckBox;
    QCheckBox *UnlockMemCheckBox;
    QCheckBox *HWMeterCheckBox;
    QCheckBox *MonitorCheckBox;
    QCheckBox *SoftModeCheckBox;
    QCheckBox *ShortsCheckBox;
    QCheckBox *IgnoreHWCheckBox;
    QSpacerItem *spacerItem8;
    QGridLayout *gridLayout2;
    QLabel *PriorityTextLabel;
    QSpinBox *PrioritySpinBox;
    QLabel *WordLengthTextLabel;
    QComboBox *WordLengthComboBox;
    QLabel *WaitTextLabel;
    QComboBox *WaitComboBox;
    QLabel *ChanTextLabel;
    QSpinBox *ChanSpinBox;
    QLabel *PortMaxTextLabel;
    QComboBox *PortMaxComboBox;
    QLabel *TimeoutTextLabel;
    QComboBox *TimeoutComboBox;
    QLabel *ClockSourceTextLabel;
    QComboBox *ClockSourceComboBox;
    QSpacerItem *spacerItem9;
    QGridLayout *gridLayout3;
    QLabel *AudioTextLabel;
    QComboBox *AudioComboBox;
    QLabel *DitherTextLabel;
    QComboBox *DitherComboBox;
    QLabel *OutDeviceTextLabel;
    qjackctlInterfaceComboBox *OutDeviceComboBox;
    QLabel *InDeviceTextLabel;
    qjackctlInterfaceComboBox *InDeviceComboBox;
    QLabel *InOutChannelsTextLabel;
    QSpinBox *InChannelsSpinBox;
    QSpinBox *OutChannelsSpinBox;
    QLabel *InOutLatencyTextLabel;
    QSpinBox *InLatencySpinBox;
    QSpinBox *OutLatencySpinBox;
    QSpacerItem *spacerItem10;
    QSpacerItem *spacerItem11;
    QHBoxLayout *hboxLayout4;
    QSpacerItem *spacerItem12;
    QLabel *SelfConnectModeTextLabel;
    QComboBox *SelfConnectModeComboBox;
    QSpacerItem *spacerItem13;
    QSpacerItem *spacerItem14;
    QHBoxLayout *hboxLayout5;
    QLabel *ServerSuffixTextLabel;
    QComboBox *ServerSuffixComboBox;
    QLabel *StartDelayTextLabel;
    QSpinBox *StartDelaySpinBox;
    QWidget *OptionsTabPage;
    QVBoxLayout *vboxLayout4;
    QGroupBox *ScriptingGroupBox;
    QGridLayout *gridLayout4;
    QCheckBox *StartupScriptCheckBox;
    QCheckBox *PostStartupScriptCheckBox;
    QCheckBox *ShutdownScriptCheckBox;
    QComboBox *StartupScriptShellComboBox;
    QToolButton *StartupScriptSymbolToolButton;
    QToolButton *StartupScriptBrowseToolButton;
    QComboBox *PostStartupScriptShellComboBox;
    QToolButton *PostStartupScriptSymbolToolButton;
    QToolButton *PostStartupScriptBrowseToolButton;
    QToolButton *ShutdownScriptSymbolToolButton;
    QToolButton *ShutdownScriptBrowseToolButton;
    QComboBox *ShutdownScriptShellComboBox;
    QCheckBox *PostShutdownScriptCheckBox;
    QToolButton *PostShutdownScriptSymbolToolButton;
    QToolButton *PostShutdownScriptBrowseToolButton;
    QComboBox *PostShutdownScriptShellComboBox;
    QGroupBox *StatisticsGroupBox;
    QGridLayout *gridLayout5;
    QCheckBox *StdoutCaptureCheckBox;
    QLabel *XrunRegexTextLabel;
    QComboBox *XrunRegexComboBox;
    QGroupBox *ConnectionsGroupBox;
    QGridLayout *gridLayout6;
    QCheckBox *ActivePatchbayCheckBox;
    QComboBox *ActivePatchbayPathComboBox;
    QToolButton *ActivePatchbayPathToolButton;
    QCheckBox *ActivePatchbayResetCheckBox;
    QCheckBox *QueryDisconnectCheckBox;
    QSpacerItem *spacerItem15;
    QGroupBox *LoggingGroupBox;
    QGridLayout *gridLayout7;
    QCheckBox *MessagesLogCheckBox;
    QComboBox *MessagesLogPathComboBox;
    QToolButton *MessagesLogPathToolButton;
    QWidget *DisplayTabPage;
    QVBoxLayout *vboxLayout5;
    QGroupBox *TimeDisplayGroupBox;
    QGridLayout *gridLayout8;
    QVBoxLayout *vboxLayout6;
    QRadioButton *TransportTimeRadioButton;
    QRadioButton *TransportBBTRadioButton;
    QRadioButton *ElapsedResetRadioButton;
    QRadioButton *ElapsedXrunRadioButton;
    QSpacerItem *spacerItem16;
    QGridLayout *gridLayout9;
    QLabel *DisplayFont2TextLabel;
    QLabel *DisplayFont1TextLabel;
    QLabel *DisplayFont1Label;
    QPushButton *DisplayFont2PushButton;
    QPushButton *DisplayFont1PushButton;
    QLabel *DisplayFont2Label;
    QCheckBox *DisplayBlinkCheckBox;
    QGroupBox *DisplayCustomGroupBox;
    QGridLayout *gridLayout10;
    QLabel *CustomColorThemeTextLabel;
    QComboBox *CustomColorThemeComboBox;
    QToolButton *CustomColorThemeToolButton;
    QSpacerItem *spacerItem17;
    QLabel *CustomStyleThemeTextLabel;
    QComboBox *CustomStyleThemeComboBox;
    QSpacerItem *spacerItem18;
    QGroupBox *MessagesWindowGroupBox;
    QHBoxLayout *hboxLayout6;
    QLabel *MessagesFontTextLabel;
    QPushButton *MessagesFontPushButton;
    QSpacerItem *spacerItem19;
    QCheckBox *MessagesLimitCheckBox;
    QComboBox *MessagesLimitLinesComboBox;
    QSpacerItem *spacerItem20;
    QGroupBox *ConnectionsWindowGroupBox;
    QGridLayout *gridLayout11;
    QLabel *ConnectionsFontTextLabel;
    QPushButton *ConnectionsFontPushButton;
    QSpacerItem *spacerItem21;
    QLabel *ConnectionsIconSizeTextLabel;
    QComboBox *ConnectionsIconSizeComboBox;
    QSpacerItem *spacerItem22;
    QCheckBox *AliasesEnabledCheckBox;
    QLabel *JackClientPortAliasTextLabel;
    QComboBox *JackClientPortAliasComboBox;
    QCheckBox *AliasesEditingCheckBox;
    QCheckBox *JackClientPortMetadataCheckBox;
    QWidget *MiscTabPage;
    QGridLayout *gridLayout12;
    QGroupBox *OtherGroupBox;
    QGridLayout *gridLayout13;
    QVBoxLayout *vboxLayout7;
    QCheckBox *StartJackCheckBox;
    QCheckBox *QueryCloseCheckBox;
    QCheckBox *QueryShutdownCheckBox;
    QCheckBox *KeepOnTopCheckBox;
    QCheckBox *SystemTrayCheckBox;
    QCheckBox *SystemTrayQueryCloseCheckBox;
    QCheckBox *StartMinimizedCheckBox;
    QVBoxLayout *vboxLayout8;
    QCheckBox *ServerConfigCheckBox;
    QComboBox *ServerConfigNameComboBox;
    QCheckBox *AlsaSeqEnabledCheckBox;
    QCheckBox *DBusEnabledCheckBox;
    QCheckBox *JackDBusEnabledCheckBox;
    QCheckBox *StopJackCheckBox;
    QCheckBox *SingletonCheckBox;
    QSpacerItem *spacerItem23;
    QGroupBox *ButtonsGroupBox;
    QVBoxLayout *vboxLayout9;
    QVBoxLayout *vboxLayout10;
    QCheckBox *LeftButtonsCheckBox;
    QCheckBox *RightButtonsCheckBox;
    QCheckBox *TransportButtonsCheckBox;
    QCheckBox *TextLabelsCheckBox;
    QSpacerItem *spacerItem24;
    QCheckBox *GraphButtonCheckBox;
    QGroupBox *DefaultsGroupBox;
    QGridLayout *gridLayout14;
    QSpacerItem *spacerItem25;
    QLabel *BaseFontSizeTextLabel;
    QComboBox *BaseFontSizeComboBox;
    QDialogButtonBox *DialogButtonBox;

    void setupUi(QDialog *qjackctlSetupForm)
    {
        if (qjackctlSetupForm->objectName().isEmpty())
            qjackctlSetupForm->setObjectName("qjackctlSetupForm");
        qjackctlSetupForm->resize(640, 520);
        const QIcon icon = QIcon(QString::fromUtf8(":/images/setup1.png"));
        qjackctlSetupForm->setWindowIcon(icon);
        qjackctlSetupForm->setSizeGripEnabled(true);
        vboxLayout = new QVBoxLayout(qjackctlSetupForm);
        vboxLayout->setSpacing(4);
        vboxLayout->setContentsMargins(8, 8, 8, 8);
        vboxLayout->setObjectName("vboxLayout");
        SetupTabWidget = new QTabWidget(qjackctlSetupForm);
        SetupTabWidget->setObjectName("SetupTabWidget");
        SetupTabWidget->setAcceptDrops(false);
        SettingsTabPage = new QWidget();
        SettingsTabPage->setObjectName("SettingsTabPage");
        vboxLayout1 = new QVBoxLayout(SettingsTabPage);
        vboxLayout1->setSpacing(4);
        vboxLayout1->setContentsMargins(8, 8, 8, 8);
        vboxLayout1->setObjectName("vboxLayout1");
        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(4);
        hboxLayout->setContentsMargins(4, 4, 4, 4);
        hboxLayout->setObjectName("hboxLayout");
        PresetTextLabel = new QLabel(SettingsTabPage);
        PresetTextLabel->setObjectName("PresetTextLabel");
        PresetTextLabel->setWordWrap(false);

        hboxLayout->addWidget(PresetTextLabel);

        PresetComboBox = new QComboBox(SettingsTabPage);
        PresetComboBox->addItem(QString());
        PresetComboBox->setObjectName("PresetComboBox");
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(PresetComboBox->sizePolicy().hasHeightForWidth());
        PresetComboBox->setSizePolicy(sizePolicy);
        PresetComboBox->setEditable(true);

        hboxLayout->addWidget(PresetComboBox);

        PresetClearPushButton = new QPushButton(SettingsTabPage);
        PresetClearPushButton->setObjectName("PresetClearPushButton");
        const QIcon icon1 = QIcon(QString::fromUtf8(":/images/clear1.png"));
        PresetClearPushButton->setIcon(icon1);
        PresetClearPushButton->setAutoDefault(false);

        hboxLayout->addWidget(PresetClearPushButton);

        PresetSavePushButton = new QPushButton(SettingsTabPage);
        PresetSavePushButton->setObjectName("PresetSavePushButton");
        const QIcon icon2 = QIcon(QString::fromUtf8(":/images/save1.png"));
        PresetSavePushButton->setIcon(icon2);
        PresetSavePushButton->setAutoDefault(false);

        hboxLayout->addWidget(PresetSavePushButton);

        PresetDeletePushButton = new QPushButton(SettingsTabPage);
        PresetDeletePushButton->setObjectName("PresetDeletePushButton");
        const QIcon icon3 = QIcon(QString::fromUtf8(":/images/remove1.png"));
        PresetDeletePushButton->setIcon(icon3);
        PresetDeletePushButton->setAutoDefault(false);

        hboxLayout->addWidget(PresetDeletePushButton);


        vboxLayout1->addLayout(hboxLayout);

        SettingsTabWidget = new QTabWidget(SettingsTabPage);
        SettingsTabWidget->setObjectName("SettingsTabWidget");
        ParametersTab = new QWidget();
        ParametersTab->setObjectName("ParametersTab");
        vboxLayout2 = new QVBoxLayout(ParametersTab);
        vboxLayout2->setSpacing(4);
        vboxLayout2->setContentsMargins(8, 8, 8, 8);
        vboxLayout2->setObjectName("vboxLayout2");
        gridLayout = new QGridLayout();
        gridLayout->setSpacing(4);
        gridLayout->setObjectName("gridLayout");
        DriverTextLabel = new QLabel(ParametersTab);
        DriverTextLabel->setObjectName("DriverTextLabel");
        QFont font;
        font.setBold(false);
        DriverTextLabel->setFont(font);
        DriverTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        DriverTextLabel->setWordWrap(false);

        gridLayout->addWidget(DriverTextLabel, 0, 0, 1, 1);

        DriverComboBox = new QComboBox(ParametersTab);
        DriverComboBox->addItem(QString());
        DriverComboBox->addItem(QString());
        DriverComboBox->addItem(QString());
        DriverComboBox->addItem(QString());
        DriverComboBox->addItem(QString());
        DriverComboBox->addItem(QString());
        DriverComboBox->addItem(QString());
        DriverComboBox->addItem(QString());
        DriverComboBox->addItem(QString());
        DriverComboBox->setObjectName("DriverComboBox");
        DriverComboBox->setFont(font);
        DriverComboBox->setEditable(false);

        gridLayout->addWidget(DriverComboBox, 0, 1, 1, 1);

        spacerItem = new QSpacerItem(8, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem, 0, 2, 4, 1);

        InterfaceTextLabel = new QLabel(ParametersTab);
        InterfaceTextLabel->setObjectName("InterfaceTextLabel");
        InterfaceTextLabel->setFont(font);
        InterfaceTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        InterfaceTextLabel->setWordWrap(false);

        gridLayout->addWidget(InterfaceTextLabel, 0, 3, 1, 1);

        InterfaceComboBox = new qjackctlInterfaceComboBox(ParametersTab);
        InterfaceComboBox->addItem(QString());
        InterfaceComboBox->addItem(QString());
        InterfaceComboBox->addItem(QString());
        InterfaceComboBox->addItem(QString());
        InterfaceComboBox->addItem(QString());
        InterfaceComboBox->setObjectName("InterfaceComboBox");
        InterfaceComboBox->setMinimumSize(QSize(140, 0));
        InterfaceComboBox->setFont(font);
        InterfaceComboBox->setEditable(true);

        gridLayout->addWidget(InterfaceComboBox, 0, 4, 1, 2);

        spacerItem1 = new QSpacerItem(8, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem1, 0, 6, 4, 1);

        MidiDriverTextLabel = new QLabel(ParametersTab);
        MidiDriverTextLabel->setObjectName("MidiDriverTextLabel");
        MidiDriverTextLabel->setFont(font);
        MidiDriverTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        MidiDriverTextLabel->setWordWrap(false);

        gridLayout->addWidget(MidiDriverTextLabel, 0, 7, 1, 1);

        MidiDriverComboBox = new QComboBox(ParametersTab);
        MidiDriverComboBox->addItem(QString());
        MidiDriverComboBox->addItem(QString());
        MidiDriverComboBox->addItem(QString());
        MidiDriverComboBox->setObjectName("MidiDriverComboBox");
        MidiDriverComboBox->setMinimumSize(QSize(80, 0));
        MidiDriverComboBox->setFont(font);
        MidiDriverComboBox->setEditable(false);

        gridLayout->addWidget(MidiDriverComboBox, 0, 8, 1, 1);

        RealtimeCheckBox = new QCheckBox(ParametersTab);
        RealtimeCheckBox->setObjectName("RealtimeCheckBox");
        RealtimeCheckBox->setFont(font);

        gridLayout->addWidget(RealtimeCheckBox, 1, 1, 1, 1);

        SampleRateTextLabel = new QLabel(ParametersTab);
        SampleRateTextLabel->setObjectName("SampleRateTextLabel");
        SampleRateTextLabel->setFont(font);
        SampleRateTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        SampleRateTextLabel->setWordWrap(false);

        gridLayout->addWidget(SampleRateTextLabel, 1, 3, 1, 1);

        SampleRateComboBox = new QComboBox(ParametersTab);
        SampleRateComboBox->addItem(QString());
        SampleRateComboBox->addItem(QString());
        SampleRateComboBox->addItem(QString());
        SampleRateComboBox->addItem(QString());
        SampleRateComboBox->addItem(QString());
        SampleRateComboBox->addItem(QString());
        SampleRateComboBox->addItem(QString());
        SampleRateComboBox->setObjectName("SampleRateComboBox");
        SampleRateComboBox->setFont(font);
        SampleRateComboBox->setEditable(true);

        gridLayout->addWidget(SampleRateComboBox, 1, 4, 1, 1);

        spacerItem2 = new QSpacerItem(8, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem2, 1, 5, 3, 1);

        FramesTextLabel = new QLabel(ParametersTab);
        FramesTextLabel->setObjectName("FramesTextLabel");
        FramesTextLabel->setFont(font);
        FramesTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        FramesTextLabel->setWordWrap(false);

        gridLayout->addWidget(FramesTextLabel, 2, 3, 1, 1);

        FramesComboBox = new QComboBox(ParametersTab);
        FramesComboBox->addItem(QString());
        FramesComboBox->addItem(QString());
        FramesComboBox->addItem(QString());
        FramesComboBox->addItem(QString());
        FramesComboBox->addItem(QString());
        FramesComboBox->addItem(QString());
        FramesComboBox->addItem(QString());
        FramesComboBox->addItem(QString());
        FramesComboBox->addItem(QString());
        FramesComboBox->setObjectName("FramesComboBox");
        FramesComboBox->setFont(font);
        FramesComboBox->setEditable(true);

        gridLayout->addWidget(FramesComboBox, 2, 4, 1, 1);

        PeriodsTextLabel = new QLabel(ParametersTab);
        PeriodsTextLabel->setObjectName("PeriodsTextLabel");
        PeriodsTextLabel->setFont(font);
        PeriodsTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        PeriodsTextLabel->setWordWrap(false);

        gridLayout->addWidget(PeriodsTextLabel, 3, 3, 1, 1);

        PeriodsSpinBox = new QSpinBox(ParametersTab);
        PeriodsSpinBox->setObjectName("PeriodsSpinBox");
        PeriodsSpinBox->setFont(font);
        PeriodsSpinBox->setMinimum(1);
        PeriodsSpinBox->setMaximum(999);
        PeriodsSpinBox->setValue(1);

        gridLayout->addWidget(PeriodsSpinBox, 3, 4, 1, 1);


        vboxLayout2->addLayout(gridLayout);

        spacerItem3 = new QSpacerItem(20, 4, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout2->addItem(spacerItem3);

        SyncCheckBox = new QCheckBox(ParametersTab);
        SyncCheckBox->setObjectName("SyncCheckBox");
        SyncCheckBox->setFont(font);

        vboxLayout2->addWidget(SyncCheckBox);

        spacerItem4 = new QSpacerItem(20, 8, QSizePolicy::Expanding, QSizePolicy::Minimum);

        vboxLayout2->addItem(spacerItem4);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(4);
        hboxLayout1->setObjectName("hboxLayout1");
        VerboseCheckBox = new QCheckBox(ParametersTab);
        VerboseCheckBox->setObjectName("VerboseCheckBox");
        VerboseCheckBox->setFont(font);

        hboxLayout1->addWidget(VerboseCheckBox);

        spacerItem5 = new QSpacerItem(8, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacerItem5);

        LatencyTextLabel = new QLabel(ParametersTab);
        LatencyTextLabel->setObjectName("LatencyTextLabel");
        LatencyTextLabel->setFont(font);
        LatencyTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        LatencyTextLabel->setWordWrap(false);

        hboxLayout1->addWidget(LatencyTextLabel);

        LatencyTextValue = new QLabel(ParametersTab);
        LatencyTextValue->setObjectName("LatencyTextValue");
        LatencyTextValue->setMinimumSize(QSize(72, 0));
        LatencyTextValue->setFont(font);
        LatencyTextValue->setFrameShape(QFrame::StyledPanel);
        LatencyTextValue->setFrameShadow(QFrame::Sunken);
        LatencyTextValue->setAlignment(Qt::AlignCenter);
        LatencyTextValue->setWordWrap(false);

        hboxLayout1->addWidget(LatencyTextValue);


        vboxLayout2->addLayout(hboxLayout1);

        SettingsTabWidget->addTab(ParametersTab, QString());
        AdvancedTab = new QWidget();
        AdvancedTab->setObjectName("AdvancedTab");
        gridLayout1 = new QGridLayout(AdvancedTab);
        gridLayout1->setSpacing(4);
        gridLayout1->setContentsMargins(8, 8, 8, 8);
        gridLayout1->setObjectName("gridLayout1");
        hboxLayout2 = new QHBoxLayout();
        hboxLayout2->setSpacing(4);
        hboxLayout2->setContentsMargins(8, 8, 8, 8);
        hboxLayout2->setObjectName("hboxLayout2");
        AdvancedIconLabel = new QLabel(AdvancedTab);
        AdvancedIconLabel->setObjectName("AdvancedIconLabel");
        AdvancedIconLabel->setMaximumSize(QSize(16, 16));

        hboxLayout2->addWidget(AdvancedIconLabel);

        AdvancedTextLabel = new QLabel(AdvancedTab);
        AdvancedTextLabel->setObjectName("AdvancedTextLabel");

        hboxLayout2->addWidget(AdvancedTextLabel);

        spacerItem6 = new QSpacerItem(20, 8, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout2->addItem(spacerItem6);


        gridLayout1->addLayout(hboxLayout2, 0, 0, 1, 5);

        hboxLayout3 = new QHBoxLayout();
        hboxLayout3->setSpacing(4);
        hboxLayout3->setObjectName("hboxLayout3");
        ServerPrefixTextLabel = new QLabel(AdvancedTab);
        ServerPrefixTextLabel->setObjectName("ServerPrefixTextLabel");
        ServerPrefixTextLabel->setFont(font);
        ServerPrefixTextLabel->setWordWrap(false);

        hboxLayout3->addWidget(ServerPrefixTextLabel);

        ServerPrefixComboBox = new QComboBox(AdvancedTab);
        ServerPrefixComboBox->addItem(QString());
        ServerPrefixComboBox->addItem(QString());
        ServerPrefixComboBox->addItem(QString());
        ServerPrefixComboBox->setObjectName("ServerPrefixComboBox");
        sizePolicy.setHeightForWidth(ServerPrefixComboBox->sizePolicy().hasHeightForWidth());
        ServerPrefixComboBox->setSizePolicy(sizePolicy);
        ServerPrefixComboBox->setFont(font);
        ServerPrefixComboBox->setEditable(true);

        hboxLayout3->addWidget(ServerPrefixComboBox);

        ServerNameTextLabel = new QLabel(AdvancedTab);
        ServerNameTextLabel->setObjectName("ServerNameTextLabel");
        ServerNameTextLabel->setFont(font);
        ServerNameTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        ServerNameTextLabel->setWordWrap(false);

        hboxLayout3->addWidget(ServerNameTextLabel);

        ServerNameComboBox = new QComboBox(AdvancedTab);
        ServerNameComboBox->addItem(QString());
        ServerNameComboBox->setObjectName("ServerNameComboBox");
        ServerNameComboBox->setMinimumSize(QSize(120, 0));
        ServerNameComboBox->setFont(font);
        ServerNameComboBox->setEditable(true);

        hboxLayout3->addWidget(ServerNameComboBox);


        gridLayout1->addLayout(hboxLayout3, 1, 0, 1, 5);

        spacerItem7 = new QSpacerItem(20, 4, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout1->addItem(spacerItem7, 2, 0, 1, 5);

        vboxLayout3 = new QVBoxLayout();
        vboxLayout3->setSpacing(4);
        vboxLayout3->setObjectName("vboxLayout3");
        NoMemLockCheckBox = new QCheckBox(AdvancedTab);
        NoMemLockCheckBox->setObjectName("NoMemLockCheckBox");
        NoMemLockCheckBox->setFont(font);

        vboxLayout3->addWidget(NoMemLockCheckBox);

        UnlockMemCheckBox = new QCheckBox(AdvancedTab);
        UnlockMemCheckBox->setObjectName("UnlockMemCheckBox");
        UnlockMemCheckBox->setFont(font);

        vboxLayout3->addWidget(UnlockMemCheckBox);

        HWMeterCheckBox = new QCheckBox(AdvancedTab);
        HWMeterCheckBox->setObjectName("HWMeterCheckBox");
        HWMeterCheckBox->setFont(font);

        vboxLayout3->addWidget(HWMeterCheckBox);

        MonitorCheckBox = new QCheckBox(AdvancedTab);
        MonitorCheckBox->setObjectName("MonitorCheckBox");
        MonitorCheckBox->setFont(font);

        vboxLayout3->addWidget(MonitorCheckBox);

        SoftModeCheckBox = new QCheckBox(AdvancedTab);
        SoftModeCheckBox->setObjectName("SoftModeCheckBox");
        SoftModeCheckBox->setFont(font);

        vboxLayout3->addWidget(SoftModeCheckBox);

        ShortsCheckBox = new QCheckBox(AdvancedTab);
        ShortsCheckBox->setObjectName("ShortsCheckBox");
        ShortsCheckBox->setFont(font);

        vboxLayout3->addWidget(ShortsCheckBox);

        IgnoreHWCheckBox = new QCheckBox(AdvancedTab);
        IgnoreHWCheckBox->setObjectName("IgnoreHWCheckBox");
        IgnoreHWCheckBox->setFont(font);

        vboxLayout3->addWidget(IgnoreHWCheckBox);


        gridLayout1->addLayout(vboxLayout3, 3, 0, 2, 1);

        spacerItem8 = new QSpacerItem(8, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout1->addItem(spacerItem8, 3, 1, 2, 1);

        gridLayout2 = new QGridLayout();
        gridLayout2->setSpacing(4);
        gridLayout2->setObjectName("gridLayout2");
        PriorityTextLabel = new QLabel(AdvancedTab);
        PriorityTextLabel->setObjectName("PriorityTextLabel");
        PriorityTextLabel->setFont(font);
        PriorityTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        PriorityTextLabel->setWordWrap(false);

        gridLayout2->addWidget(PriorityTextLabel, 0, 0, 1, 1);

        PrioritySpinBox = new QSpinBox(AdvancedTab);
        PrioritySpinBox->setObjectName("PrioritySpinBox");
        PrioritySpinBox->setFont(font);
        PrioritySpinBox->setSingleStep(1);
        PrioritySpinBox->setMinimum(5);
        PrioritySpinBox->setMaximum(95);

        gridLayout2->addWidget(PrioritySpinBox, 0, 1, 1, 1);

        WordLengthTextLabel = new QLabel(AdvancedTab);
        WordLengthTextLabel->setObjectName("WordLengthTextLabel");
        WordLengthTextLabel->setFont(font);
        WordLengthTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        WordLengthTextLabel->setWordWrap(false);

        gridLayout2->addWidget(WordLengthTextLabel, 1, 0, 1, 1);

        WordLengthComboBox = new QComboBox(AdvancedTab);
        WordLengthComboBox->addItem(QString());
        WordLengthComboBox->addItem(QString());
        WordLengthComboBox->addItem(QString());
        WordLengthComboBox->setObjectName("WordLengthComboBox");
        WordLengthComboBox->setFont(font);
        WordLengthComboBox->setEditable(true);

        gridLayout2->addWidget(WordLengthComboBox, 1, 1, 1, 1);

        WaitTextLabel = new QLabel(AdvancedTab);
        WaitTextLabel->setObjectName("WaitTextLabel");
        WaitTextLabel->setFont(font);
        WaitTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        WaitTextLabel->setWordWrap(false);

        gridLayout2->addWidget(WaitTextLabel, 2, 0, 1, 1);

        WaitComboBox = new QComboBox(AdvancedTab);
        WaitComboBox->addItem(QString());
        WaitComboBox->setObjectName("WaitComboBox");
        QFont font1;
        font1.setFamilies({QString::fromUtf8("Sans Serif")});
        font1.setBold(false);
        WaitComboBox->setFont(font1);
        WaitComboBox->setEditable(true);

        gridLayout2->addWidget(WaitComboBox, 2, 1, 1, 1);

        ChanTextLabel = new QLabel(AdvancedTab);
        ChanTextLabel->setObjectName("ChanTextLabel");
        ChanTextLabel->setFont(font);
        ChanTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        ChanTextLabel->setWordWrap(false);
        ChanTextLabel->setMargin(0);

        gridLayout2->addWidget(ChanTextLabel, 3, 0, 1, 1);

        ChanSpinBox = new QSpinBox(AdvancedTab);
        ChanSpinBox->setObjectName("ChanSpinBox");
        ChanSpinBox->setFont(font);
        ChanSpinBox->setMaximum(999);

        gridLayout2->addWidget(ChanSpinBox, 3, 1, 1, 1);

        PortMaxTextLabel = new QLabel(AdvancedTab);
        PortMaxTextLabel->setObjectName("PortMaxTextLabel");
        PortMaxTextLabel->setFont(font);
        PortMaxTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        PortMaxTextLabel->setWordWrap(false);

        gridLayout2->addWidget(PortMaxTextLabel, 4, 0, 1, 1);

        PortMaxComboBox = new QComboBox(AdvancedTab);
        PortMaxComboBox->addItem(QString());
        PortMaxComboBox->addItem(QString());
        PortMaxComboBox->addItem(QString());
        PortMaxComboBox->addItem(QString());
        PortMaxComboBox->setObjectName("PortMaxComboBox");
        PortMaxComboBox->setMinimumSize(QSize(60, 0));
        PortMaxComboBox->setFont(font);
        PortMaxComboBox->setEditable(true);

        gridLayout2->addWidget(PortMaxComboBox, 4, 1, 1, 1);

        TimeoutTextLabel = new QLabel(AdvancedTab);
        TimeoutTextLabel->setObjectName("TimeoutTextLabel");
        QFont font2;
        font2.setBold(false);
        font2.setItalic(false);
        TimeoutTextLabel->setFont(font2);
        TimeoutTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        TimeoutTextLabel->setWordWrap(false);

        gridLayout2->addWidget(TimeoutTextLabel, 5, 0, 1, 1);

        TimeoutComboBox = new QComboBox(AdvancedTab);
        TimeoutComboBox->addItem(QString());
        TimeoutComboBox->addItem(QString());
        TimeoutComboBox->addItem(QString());
        TimeoutComboBox->addItem(QString());
        TimeoutComboBox->addItem(QString());
        TimeoutComboBox->setObjectName("TimeoutComboBox");
        TimeoutComboBox->setFont(font);
        TimeoutComboBox->setEditable(true);

        gridLayout2->addWidget(TimeoutComboBox, 5, 1, 1, 1);

        ClockSourceTextLabel = new QLabel(AdvancedTab);
        ClockSourceTextLabel->setObjectName("ClockSourceTextLabel");
        ClockSourceTextLabel->setFont(font);
        ClockSourceTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        ClockSourceTextLabel->setWordWrap(false);

        gridLayout2->addWidget(ClockSourceTextLabel, 6, 0, 1, 1);

        ClockSourceComboBox = new QComboBox(AdvancedTab);
        ClockSourceComboBox->setObjectName("ClockSourceComboBox");
        ClockSourceComboBox->setFont(font);
        ClockSourceComboBox->setEditable(false);

        gridLayout2->addWidget(ClockSourceComboBox, 6, 1, 1, 1);


        gridLayout1->addLayout(gridLayout2, 3, 2, 1, 1);

        spacerItem9 = new QSpacerItem(8, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout1->addItem(spacerItem9, 3, 3, 1, 1);

        gridLayout3 = new QGridLayout();
        gridLayout3->setSpacing(4);
        gridLayout3->setObjectName("gridLayout3");
        AudioTextLabel = new QLabel(AdvancedTab);
        AudioTextLabel->setObjectName("AudioTextLabel");
        AudioTextLabel->setFont(font);
        AudioTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        AudioTextLabel->setWordWrap(false);

        gridLayout3->addWidget(AudioTextLabel, 0, 0, 1, 1);

        AudioComboBox = new QComboBox(AdvancedTab);
        AudioComboBox->addItem(QString());
        AudioComboBox->addItem(QString());
        AudioComboBox->addItem(QString());
        AudioComboBox->setObjectName("AudioComboBox");
        QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(AudioComboBox->sizePolicy().hasHeightForWidth());
        AudioComboBox->setSizePolicy(sizePolicy1);
        AudioComboBox->setFont(font);

        gridLayout3->addWidget(AudioComboBox, 0, 1, 1, 2);

        DitherTextLabel = new QLabel(AdvancedTab);
        DitherTextLabel->setObjectName("DitherTextLabel");
        DitherTextLabel->setFont(font);
        DitherTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        DitherTextLabel->setWordWrap(false);

        gridLayout3->addWidget(DitherTextLabel, 1, 0, 1, 1);

        DitherComboBox = new QComboBox(AdvancedTab);
        DitherComboBox->addItem(QString());
        DitherComboBox->addItem(QString());
        DitherComboBox->addItem(QString());
        DitherComboBox->addItem(QString());
        DitherComboBox->setObjectName("DitherComboBox");
        sizePolicy1.setHeightForWidth(DitherComboBox->sizePolicy().hasHeightForWidth());
        DitherComboBox->setSizePolicy(sizePolicy1);
        DitherComboBox->setFont(font);

        gridLayout3->addWidget(DitherComboBox, 1, 1, 1, 2);

        OutDeviceTextLabel = new QLabel(AdvancedTab);
        OutDeviceTextLabel->setObjectName("OutDeviceTextLabel");
        OutDeviceTextLabel->setFont(font);
        OutDeviceTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        OutDeviceTextLabel->setWordWrap(false);

        gridLayout3->addWidget(OutDeviceTextLabel, 2, 0, 1, 1);

        OutDeviceComboBox = new qjackctlInterfaceComboBox(AdvancedTab);
        OutDeviceComboBox->addItem(QString());
        OutDeviceComboBox->addItem(QString());
        OutDeviceComboBox->addItem(QString());
        OutDeviceComboBox->addItem(QString());
        OutDeviceComboBox->addItem(QString());
        OutDeviceComboBox->setObjectName("OutDeviceComboBox");
        OutDeviceComboBox->setFont(font);
        OutDeviceComboBox->setEditable(true);

        gridLayout3->addWidget(OutDeviceComboBox, 2, 1, 1, 2);

        InDeviceTextLabel = new QLabel(AdvancedTab);
        InDeviceTextLabel->setObjectName("InDeviceTextLabel");
        InDeviceTextLabel->setFont(font);
        InDeviceTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        InDeviceTextLabel->setWordWrap(false);

        gridLayout3->addWidget(InDeviceTextLabel, 3, 0, 1, 1);

        InDeviceComboBox = new qjackctlInterfaceComboBox(AdvancedTab);
        InDeviceComboBox->addItem(QString());
        InDeviceComboBox->addItem(QString());
        InDeviceComboBox->addItem(QString());
        InDeviceComboBox->addItem(QString());
        InDeviceComboBox->addItem(QString());
        InDeviceComboBox->setObjectName("InDeviceComboBox");
        InDeviceComboBox->setFont(font);
        InDeviceComboBox->setEditable(true);

        gridLayout3->addWidget(InDeviceComboBox, 3, 1, 1, 2);

        InOutChannelsTextLabel = new QLabel(AdvancedTab);
        InOutChannelsTextLabel->setObjectName("InOutChannelsTextLabel");
        InOutChannelsTextLabel->setFont(font);
        InOutChannelsTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        InOutChannelsTextLabel->setWordWrap(false);

        gridLayout3->addWidget(InOutChannelsTextLabel, 4, 0, 1, 1);

        InChannelsSpinBox = new QSpinBox(AdvancedTab);
        InChannelsSpinBox->setObjectName("InChannelsSpinBox");
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(InChannelsSpinBox->sizePolicy().hasHeightForWidth());
        InChannelsSpinBox->setSizePolicy(sizePolicy2);
        InChannelsSpinBox->setMinimumSize(QSize(80, 0));
        InChannelsSpinBox->setFont(font);
        InChannelsSpinBox->setMaximum(999);

        gridLayout3->addWidget(InChannelsSpinBox, 4, 1, 1, 1);

        OutChannelsSpinBox = new QSpinBox(AdvancedTab);
        OutChannelsSpinBox->setObjectName("OutChannelsSpinBox");
        sizePolicy2.setHeightForWidth(OutChannelsSpinBox->sizePolicy().hasHeightForWidth());
        OutChannelsSpinBox->setSizePolicy(sizePolicy2);
        OutChannelsSpinBox->setMinimumSize(QSize(80, 0));
        OutChannelsSpinBox->setFont(font);
        OutChannelsSpinBox->setMaximum(999);

        gridLayout3->addWidget(OutChannelsSpinBox, 4, 2, 1, 1);

        InOutLatencyTextLabel = new QLabel(AdvancedTab);
        InOutLatencyTextLabel->setObjectName("InOutLatencyTextLabel");
        InOutLatencyTextLabel->setFont(font);
        InOutLatencyTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        InOutLatencyTextLabel->setWordWrap(false);

        gridLayout3->addWidget(InOutLatencyTextLabel, 5, 0, 1, 1);

        InLatencySpinBox = new QSpinBox(AdvancedTab);
        InLatencySpinBox->setObjectName("InLatencySpinBox");
        sizePolicy2.setHeightForWidth(InLatencySpinBox->sizePolicy().hasHeightForWidth());
        InLatencySpinBox->setSizePolicy(sizePolicy2);
        InLatencySpinBox->setMinimumSize(QSize(80, 0));
        InLatencySpinBox->setFont(font);
        InLatencySpinBox->setMaximum(9999999);

        gridLayout3->addWidget(InLatencySpinBox, 5, 1, 1, 1);

        OutLatencySpinBox = new QSpinBox(AdvancedTab);
        OutLatencySpinBox->setObjectName("OutLatencySpinBox");
        sizePolicy2.setHeightForWidth(OutLatencySpinBox->sizePolicy().hasHeightForWidth());
        OutLatencySpinBox->setSizePolicy(sizePolicy2);
        OutLatencySpinBox->setMinimumSize(QSize(80, 0));
        OutLatencySpinBox->setFont(font);
        OutLatencySpinBox->setMaximum(9999999);

        gridLayout3->addWidget(OutLatencySpinBox, 5, 2, 1, 1);

        spacerItem10 = new QSpacerItem(20, 4, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout3->addItem(spacerItem10, 6, 0, 1, 3);


        gridLayout1->addLayout(gridLayout3, 3, 4, 1, 1);

        spacerItem11 = new QSpacerItem(20, 4, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout1->addItem(spacerItem11, 4, 0, 1, 5);

        hboxLayout4 = new QHBoxLayout();
        hboxLayout4->setSpacing(4);
        hboxLayout4->setObjectName("hboxLayout4");
        spacerItem12 = new QSpacerItem(20, 8, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout4->addItem(spacerItem12);

        SelfConnectModeTextLabel = new QLabel(AdvancedTab);
        SelfConnectModeTextLabel->setObjectName("SelfConnectModeTextLabel");
        SelfConnectModeTextLabel->setFont(font);

        hboxLayout4->addWidget(SelfConnectModeTextLabel);

        SelfConnectModeComboBox = new QComboBox(AdvancedTab);
        SelfConnectModeComboBox->setObjectName("SelfConnectModeComboBox");
        SelfConnectModeComboBox->setFont(font);
        SelfConnectModeComboBox->setEditable(false);

        hboxLayout4->addWidget(SelfConnectModeComboBox);

        spacerItem13 = new QSpacerItem(20, 8, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout4->addItem(spacerItem13);


        gridLayout1->addLayout(hboxLayout4, 5, 0, 1, 5);

        spacerItem14 = new QSpacerItem(20, 4, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout1->addItem(spacerItem14, 6, 0, 1, 5);

        hboxLayout5 = new QHBoxLayout();
        hboxLayout5->setSpacing(4);
        hboxLayout5->setObjectName("hboxLayout5");
        ServerSuffixTextLabel = new QLabel(AdvancedTab);
        ServerSuffixTextLabel->setObjectName("ServerSuffixTextLabel");
        ServerSuffixTextLabel->setFont(font);
        ServerSuffixTextLabel->setWordWrap(false);

        hboxLayout5->addWidget(ServerSuffixTextLabel);

        ServerSuffixComboBox = new QComboBox(AdvancedTab);
        ServerSuffixComboBox->setObjectName("ServerSuffixComboBox");
        sizePolicy.setHeightForWidth(ServerSuffixComboBox->sizePolicy().hasHeightForWidth());
        ServerSuffixComboBox->setSizePolicy(sizePolicy);
        ServerSuffixComboBox->setFont(font);
        ServerSuffixComboBox->setEditable(true);

        hboxLayout5->addWidget(ServerSuffixComboBox);

        StartDelayTextLabel = new QLabel(AdvancedTab);
        StartDelayTextLabel->setObjectName("StartDelayTextLabel");
        StartDelayTextLabel->setFont(font);
        StartDelayTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        StartDelayTextLabel->setWordWrap(false);

        hboxLayout5->addWidget(StartDelayTextLabel);

        StartDelaySpinBox = new QSpinBox(AdvancedTab);
        StartDelaySpinBox->setObjectName("StartDelaySpinBox");
        StartDelaySpinBox->setFont(font);
        StartDelaySpinBox->setMinimum(0);
        StartDelaySpinBox->setMaximum(999);

        hboxLayout5->addWidget(StartDelaySpinBox);


        gridLayout1->addLayout(hboxLayout5, 7, 0, 1, 5);

        SettingsTabWidget->addTab(AdvancedTab, QString());

        vboxLayout1->addWidget(SettingsTabWidget);

        SetupTabWidget->addTab(SettingsTabPage, QString());
        OptionsTabPage = new QWidget();
        OptionsTabPage->setObjectName("OptionsTabPage");
        vboxLayout4 = new QVBoxLayout(OptionsTabPage);
        vboxLayout4->setSpacing(4);
        vboxLayout4->setContentsMargins(8, 8, 8, 8);
        vboxLayout4->setObjectName("vboxLayout4");
        ScriptingGroupBox = new QGroupBox(OptionsTabPage);
        ScriptingGroupBox->setObjectName("ScriptingGroupBox");
        QFont font3;
        font3.setBold(true);
        ScriptingGroupBox->setFont(font3);
        ScriptingGroupBox->setFlat(true);
        gridLayout4 = new QGridLayout(ScriptingGroupBox);
        gridLayout4->setSpacing(4);
        gridLayout4->setContentsMargins(8, 8, 8, 8);
        gridLayout4->setObjectName("gridLayout4");
        StartupScriptCheckBox = new QCheckBox(ScriptingGroupBox);
        StartupScriptCheckBox->setObjectName("StartupScriptCheckBox");
        StartupScriptCheckBox->setFont(font);

        gridLayout4->addWidget(StartupScriptCheckBox, 0, 0, 1, 1);

        PostStartupScriptCheckBox = new QCheckBox(ScriptingGroupBox);
        PostStartupScriptCheckBox->setObjectName("PostStartupScriptCheckBox");
        PostStartupScriptCheckBox->setFont(font);

        gridLayout4->addWidget(PostStartupScriptCheckBox, 1, 0, 1, 1);

        ShutdownScriptCheckBox = new QCheckBox(ScriptingGroupBox);
        ShutdownScriptCheckBox->setObjectName("ShutdownScriptCheckBox");
        ShutdownScriptCheckBox->setFont(font);

        gridLayout4->addWidget(ShutdownScriptCheckBox, 2, 0, 1, 1);

        StartupScriptShellComboBox = new QComboBox(ScriptingGroupBox);
        StartupScriptShellComboBox->setObjectName("StartupScriptShellComboBox");
        sizePolicy.setHeightForWidth(StartupScriptShellComboBox->sizePolicy().hasHeightForWidth());
        StartupScriptShellComboBox->setSizePolicy(sizePolicy);
        StartupScriptShellComboBox->setFont(font);
        StartupScriptShellComboBox->setEditable(true);

        gridLayout4->addWidget(StartupScriptShellComboBox, 0, 1, 1, 1);

        StartupScriptSymbolToolButton = new QToolButton(ScriptingGroupBox);
        StartupScriptSymbolToolButton->setObjectName("StartupScriptSymbolToolButton");
        StartupScriptSymbolToolButton->setMinimumSize(QSize(22, 22));
        StartupScriptSymbolToolButton->setMaximumSize(QSize(24, 24));
        StartupScriptSymbolToolButton->setFont(font);
        StartupScriptSymbolToolButton->setFocusPolicy(Qt::TabFocus);

        gridLayout4->addWidget(StartupScriptSymbolToolButton, 0, 2, 1, 1);

        StartupScriptBrowseToolButton = new QToolButton(ScriptingGroupBox);
        StartupScriptBrowseToolButton->setObjectName("StartupScriptBrowseToolButton");
        StartupScriptBrowseToolButton->setMinimumSize(QSize(22, 22));
        StartupScriptBrowseToolButton->setMaximumSize(QSize(24, 24));
        StartupScriptBrowseToolButton->setFont(font);
        StartupScriptBrowseToolButton->setFocusPolicy(Qt::TabFocus);

        gridLayout4->addWidget(StartupScriptBrowseToolButton, 0, 3, 1, 1);

        PostStartupScriptShellComboBox = new QComboBox(ScriptingGroupBox);
        PostStartupScriptShellComboBox->setObjectName("PostStartupScriptShellComboBox");
        sizePolicy.setHeightForWidth(PostStartupScriptShellComboBox->sizePolicy().hasHeightForWidth());
        PostStartupScriptShellComboBox->setSizePolicy(sizePolicy);
        PostStartupScriptShellComboBox->setFont(font);
        PostStartupScriptShellComboBox->setEditable(true);

        gridLayout4->addWidget(PostStartupScriptShellComboBox, 1, 1, 1, 1);

        PostStartupScriptSymbolToolButton = new QToolButton(ScriptingGroupBox);
        PostStartupScriptSymbolToolButton->setObjectName("PostStartupScriptSymbolToolButton");
        PostStartupScriptSymbolToolButton->setMinimumSize(QSize(22, 22));
        PostStartupScriptSymbolToolButton->setMaximumSize(QSize(24, 24));
        PostStartupScriptSymbolToolButton->setFont(font);
        PostStartupScriptSymbolToolButton->setFocusPolicy(Qt::TabFocus);

        gridLayout4->addWidget(PostStartupScriptSymbolToolButton, 1, 2, 1, 1);

        PostStartupScriptBrowseToolButton = new QToolButton(ScriptingGroupBox);
        PostStartupScriptBrowseToolButton->setObjectName("PostStartupScriptBrowseToolButton");
        PostStartupScriptBrowseToolButton->setMinimumSize(QSize(22, 22));
        PostStartupScriptBrowseToolButton->setMaximumSize(QSize(24, 24));
        PostStartupScriptBrowseToolButton->setFont(font);
        PostStartupScriptBrowseToolButton->setFocusPolicy(Qt::TabFocus);

        gridLayout4->addWidget(PostStartupScriptBrowseToolButton, 1, 3, 1, 1);

        ShutdownScriptSymbolToolButton = new QToolButton(ScriptingGroupBox);
        ShutdownScriptSymbolToolButton->setObjectName("ShutdownScriptSymbolToolButton");
        ShutdownScriptSymbolToolButton->setMinimumSize(QSize(22, 22));
        ShutdownScriptSymbolToolButton->setMaximumSize(QSize(24, 24));
        ShutdownScriptSymbolToolButton->setFont(font);
        ShutdownScriptSymbolToolButton->setFocusPolicy(Qt::TabFocus);

        gridLayout4->addWidget(ShutdownScriptSymbolToolButton, 2, 2, 1, 1);

        ShutdownScriptBrowseToolButton = new QToolButton(ScriptingGroupBox);
        ShutdownScriptBrowseToolButton->setObjectName("ShutdownScriptBrowseToolButton");
        ShutdownScriptBrowseToolButton->setMinimumSize(QSize(22, 22));
        ShutdownScriptBrowseToolButton->setMaximumSize(QSize(24, 24));
        ShutdownScriptBrowseToolButton->setFont(font);
        ShutdownScriptBrowseToolButton->setFocusPolicy(Qt::TabFocus);

        gridLayout4->addWidget(ShutdownScriptBrowseToolButton, 2, 3, 1, 1);

        ShutdownScriptShellComboBox = new QComboBox(ScriptingGroupBox);
        ShutdownScriptShellComboBox->setObjectName("ShutdownScriptShellComboBox");
        sizePolicy.setHeightForWidth(ShutdownScriptShellComboBox->sizePolicy().hasHeightForWidth());
        ShutdownScriptShellComboBox->setSizePolicy(sizePolicy);
        ShutdownScriptShellComboBox->setFont(font);
        ShutdownScriptShellComboBox->setEditable(true);

        gridLayout4->addWidget(ShutdownScriptShellComboBox, 2, 1, 1, 1);

        PostShutdownScriptCheckBox = new QCheckBox(ScriptingGroupBox);
        PostShutdownScriptCheckBox->setObjectName("PostShutdownScriptCheckBox");
        PostShutdownScriptCheckBox->setFont(font);

        gridLayout4->addWidget(PostShutdownScriptCheckBox, 3, 0, 1, 1);

        PostShutdownScriptSymbolToolButton = new QToolButton(ScriptingGroupBox);
        PostShutdownScriptSymbolToolButton->setObjectName("PostShutdownScriptSymbolToolButton");
        PostShutdownScriptSymbolToolButton->setMinimumSize(QSize(22, 22));
        PostShutdownScriptSymbolToolButton->setMaximumSize(QSize(24, 24));
        PostShutdownScriptSymbolToolButton->setFont(font);
        PostShutdownScriptSymbolToolButton->setFocusPolicy(Qt::TabFocus);

        gridLayout4->addWidget(PostShutdownScriptSymbolToolButton, 3, 2, 1, 1);

        PostShutdownScriptBrowseToolButton = new QToolButton(ScriptingGroupBox);
        PostShutdownScriptBrowseToolButton->setObjectName("PostShutdownScriptBrowseToolButton");
        PostShutdownScriptBrowseToolButton->setMinimumSize(QSize(22, 22));
        PostShutdownScriptBrowseToolButton->setMaximumSize(QSize(24, 24));
        PostShutdownScriptBrowseToolButton->setFont(font);
        PostShutdownScriptBrowseToolButton->setFocusPolicy(Qt::TabFocus);

        gridLayout4->addWidget(PostShutdownScriptBrowseToolButton, 3, 3, 1, 1);

        PostShutdownScriptShellComboBox = new QComboBox(ScriptingGroupBox);
        PostShutdownScriptShellComboBox->setObjectName("PostShutdownScriptShellComboBox");
        sizePolicy.setHeightForWidth(PostShutdownScriptShellComboBox->sizePolicy().hasHeightForWidth());
        PostShutdownScriptShellComboBox->setSizePolicy(sizePolicy);
        PostShutdownScriptShellComboBox->setFont(font);
        PostShutdownScriptShellComboBox->setEditable(true);

        gridLayout4->addWidget(PostShutdownScriptShellComboBox, 3, 1, 1, 1);


        vboxLayout4->addWidget(ScriptingGroupBox);

        StatisticsGroupBox = new QGroupBox(OptionsTabPage);
        StatisticsGroupBox->setObjectName("StatisticsGroupBox");
        StatisticsGroupBox->setFont(font3);
        StatisticsGroupBox->setFlat(true);
        gridLayout5 = new QGridLayout(StatisticsGroupBox);
        gridLayout5->setSpacing(4);
        gridLayout5->setContentsMargins(8, 8, 8, 8);
        gridLayout5->setObjectName("gridLayout5");
        StdoutCaptureCheckBox = new QCheckBox(StatisticsGroupBox);
        StdoutCaptureCheckBox->setObjectName("StdoutCaptureCheckBox");
        StdoutCaptureCheckBox->setFont(font);

        gridLayout5->addWidget(StdoutCaptureCheckBox, 0, 0, 1, 3);

        XrunRegexTextLabel = new QLabel(StatisticsGroupBox);
        XrunRegexTextLabel->setObjectName("XrunRegexTextLabel");
        XrunRegexTextLabel->setFont(font);
        XrunRegexTextLabel->setWordWrap(false);

        gridLayout5->addWidget(XrunRegexTextLabel, 1, 1, 1, 1);

        XrunRegexComboBox = new QComboBox(StatisticsGroupBox);
        XrunRegexComboBox->addItem(QString());
        XrunRegexComboBox->setObjectName("XrunRegexComboBox");
        sizePolicy.setHeightForWidth(XrunRegexComboBox->sizePolicy().hasHeightForWidth());
        XrunRegexComboBox->setSizePolicy(sizePolicy);
        XrunRegexComboBox->setFont(font);
        XrunRegexComboBox->setEditable(true);

        gridLayout5->addWidget(XrunRegexComboBox, 1, 2, 1, 1);


        vboxLayout4->addWidget(StatisticsGroupBox);

        ConnectionsGroupBox = new QGroupBox(OptionsTabPage);
        ConnectionsGroupBox->setObjectName("ConnectionsGroupBox");
        ConnectionsGroupBox->setFont(font3);
        ConnectionsGroupBox->setFlat(true);
        gridLayout6 = new QGridLayout(ConnectionsGroupBox);
        gridLayout6->setSpacing(4);
        gridLayout6->setContentsMargins(8, 8, 8, 8);
        gridLayout6->setObjectName("gridLayout6");
        ActivePatchbayCheckBox = new QCheckBox(ConnectionsGroupBox);
        ActivePatchbayCheckBox->setObjectName("ActivePatchbayCheckBox");
        ActivePatchbayCheckBox->setFont(font);

        gridLayout6->addWidget(ActivePatchbayCheckBox, 0, 0, 1, 1);

        ActivePatchbayPathComboBox = new QComboBox(ConnectionsGroupBox);
        ActivePatchbayPathComboBox->setObjectName("ActivePatchbayPathComboBox");
        sizePolicy.setHeightForWidth(ActivePatchbayPathComboBox->sizePolicy().hasHeightForWidth());
        ActivePatchbayPathComboBox->setSizePolicy(sizePolicy);
        ActivePatchbayPathComboBox->setFont(font);
        ActivePatchbayPathComboBox->setEditable(true);

        gridLayout6->addWidget(ActivePatchbayPathComboBox, 0, 1, 1, 2);

        ActivePatchbayPathToolButton = new QToolButton(ConnectionsGroupBox);
        ActivePatchbayPathToolButton->setObjectName("ActivePatchbayPathToolButton");
        ActivePatchbayPathToolButton->setMinimumSize(QSize(22, 22));
        ActivePatchbayPathToolButton->setMaximumSize(QSize(24, 24));
        ActivePatchbayPathToolButton->setFont(font);
        ActivePatchbayPathToolButton->setFocusPolicy(Qt::TabFocus);

        gridLayout6->addWidget(ActivePatchbayPathToolButton, 0, 3, 1, 1);

        ActivePatchbayResetCheckBox = new QCheckBox(ConnectionsGroupBox);
        ActivePatchbayResetCheckBox->setObjectName("ActivePatchbayResetCheckBox");
        ActivePatchbayResetCheckBox->setFont(font);

        gridLayout6->addWidget(ActivePatchbayResetCheckBox, 1, 0, 1, 2);

        QueryDisconnectCheckBox = new QCheckBox(ConnectionsGroupBox);
        QueryDisconnectCheckBox->setObjectName("QueryDisconnectCheckBox");
        QueryDisconnectCheckBox->setFont(font);

        gridLayout6->addWidget(QueryDisconnectCheckBox, 1, 2, 1, 2);


        vboxLayout4->addWidget(ConnectionsGroupBox);

        spacerItem15 = new QSpacerItem(20, 4, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout4->addItem(spacerItem15);

        LoggingGroupBox = new QGroupBox(OptionsTabPage);
        LoggingGroupBox->setObjectName("LoggingGroupBox");
        LoggingGroupBox->setFont(font3);
        LoggingGroupBox->setFlat(true);
        gridLayout7 = new QGridLayout(LoggingGroupBox);
        gridLayout7->setSpacing(4);
        gridLayout7->setContentsMargins(8, 8, 8, 8);
        gridLayout7->setObjectName("gridLayout7");
        MessagesLogCheckBox = new QCheckBox(LoggingGroupBox);
        MessagesLogCheckBox->setObjectName("MessagesLogCheckBox");
        MessagesLogCheckBox->setFont(font);

        gridLayout7->addWidget(MessagesLogCheckBox, 0, 0, 1, 1);

        MessagesLogPathComboBox = new QComboBox(LoggingGroupBox);
        MessagesLogPathComboBox->setObjectName("MessagesLogPathComboBox");
        sizePolicy.setHeightForWidth(MessagesLogPathComboBox->sizePolicy().hasHeightForWidth());
        MessagesLogPathComboBox->setSizePolicy(sizePolicy);
        MessagesLogPathComboBox->setFont(font);
        MessagesLogPathComboBox->setEditable(true);

        gridLayout7->addWidget(MessagesLogPathComboBox, 0, 1, 1, 1);

        MessagesLogPathToolButton = new QToolButton(LoggingGroupBox);
        MessagesLogPathToolButton->setObjectName("MessagesLogPathToolButton");
        MessagesLogPathToolButton->setMinimumSize(QSize(22, 22));
        MessagesLogPathToolButton->setMaximumSize(QSize(24, 24));
        MessagesLogPathToolButton->setFont(font);
        MessagesLogPathToolButton->setFocusPolicy(Qt::TabFocus);

        gridLayout7->addWidget(MessagesLogPathToolButton, 0, 2, 1, 1);


        vboxLayout4->addWidget(LoggingGroupBox);

        SetupTabWidget->addTab(OptionsTabPage, QString());
        DisplayTabPage = new QWidget();
        DisplayTabPage->setObjectName("DisplayTabPage");
        vboxLayout5 = new QVBoxLayout(DisplayTabPage);
        vboxLayout5->setSpacing(4);
        vboxLayout5->setContentsMargins(8, 8, 8, 8);
        vboxLayout5->setObjectName("vboxLayout5");
        TimeDisplayGroupBox = new QGroupBox(DisplayTabPage);
        TimeDisplayGroupBox->setObjectName("TimeDisplayGroupBox");
        TimeDisplayGroupBox->setFont(font3);
        TimeDisplayGroupBox->setFlat(true);
        gridLayout8 = new QGridLayout(TimeDisplayGroupBox);
        gridLayout8->setSpacing(4);
        gridLayout8->setContentsMargins(8, 8, 8, 8);
        gridLayout8->setObjectName("gridLayout8");
        vboxLayout6 = new QVBoxLayout();
        vboxLayout6->setSpacing(4);
        vboxLayout6->setContentsMargins(4, 4, 4, 4);
        vboxLayout6->setObjectName("vboxLayout6");
        TransportTimeRadioButton = new QRadioButton(TimeDisplayGroupBox);
        TransportTimeRadioButton->setObjectName("TransportTimeRadioButton");
        TransportTimeRadioButton->setFont(font);

        vboxLayout6->addWidget(TransportTimeRadioButton);

        TransportBBTRadioButton = new QRadioButton(TimeDisplayGroupBox);
        TransportBBTRadioButton->setObjectName("TransportBBTRadioButton");
        TransportBBTRadioButton->setFont(font);

        vboxLayout6->addWidget(TransportBBTRadioButton);

        ElapsedResetRadioButton = new QRadioButton(TimeDisplayGroupBox);
        ElapsedResetRadioButton->setObjectName("ElapsedResetRadioButton");
        ElapsedResetRadioButton->setFont(font);

        vboxLayout6->addWidget(ElapsedResetRadioButton);

        ElapsedXrunRadioButton = new QRadioButton(TimeDisplayGroupBox);
        ElapsedXrunRadioButton->setObjectName("ElapsedXrunRadioButton");
        ElapsedXrunRadioButton->setFont(font);

        vboxLayout6->addWidget(ElapsedXrunRadioButton);


        gridLayout8->addLayout(vboxLayout6, 0, 0, 1, 1);

        spacerItem16 = new QSpacerItem(8, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout8->addItem(spacerItem16, 0, 1, 2, 1);

        gridLayout9 = new QGridLayout();
        gridLayout9->setSpacing(4);
        gridLayout9->setContentsMargins(4, 4, 4, 4);
        gridLayout9->setObjectName("gridLayout9");
        DisplayFont2TextLabel = new QLabel(TimeDisplayGroupBox);
        DisplayFont2TextLabel->setObjectName("DisplayFont2TextLabel");
        DisplayFont2TextLabel->setMinimumSize(QSize(180, 0));
        DisplayFont2TextLabel->setMaximumSize(QSize(260, 32767));
        DisplayFont2TextLabel->setFont(font);
        DisplayFont2TextLabel->setAutoFillBackground(true);
        DisplayFont2TextLabel->setFrameShape(QFrame::StyledPanel);
        DisplayFont2TextLabel->setFrameShadow(QFrame::Sunken);
        DisplayFont2TextLabel->setAlignment(Qt::AlignCenter);
        DisplayFont2TextLabel->setWordWrap(false);

        gridLayout9->addWidget(DisplayFont2TextLabel, 3, 0, 1, 1);

        DisplayFont1TextLabel = new QLabel(TimeDisplayGroupBox);
        DisplayFont1TextLabel->setObjectName("DisplayFont1TextLabel");
        DisplayFont1TextLabel->setMinimumSize(QSize(180, 0));
        DisplayFont1TextLabel->setMaximumSize(QSize(260, 32767));
        DisplayFont1TextLabel->setFont(font);
        DisplayFont1TextLabel->setAutoFillBackground(true);
        DisplayFont1TextLabel->setFrameShape(QFrame::StyledPanel);
        DisplayFont1TextLabel->setFrameShadow(QFrame::Sunken);
        DisplayFont1TextLabel->setAlignment(Qt::AlignCenter);
        DisplayFont1TextLabel->setWordWrap(false);

        gridLayout9->addWidget(DisplayFont1TextLabel, 1, 0, 1, 1);

        DisplayFont1Label = new QLabel(TimeDisplayGroupBox);
        DisplayFont1Label->setObjectName("DisplayFont1Label");
        DisplayFont1Label->setFont(font);
        DisplayFont1Label->setWordWrap(false);

        gridLayout9->addWidget(DisplayFont1Label, 0, 0, 1, 2);

        DisplayFont2PushButton = new QPushButton(TimeDisplayGroupBox);
        DisplayFont2PushButton->setObjectName("DisplayFont2PushButton");
        DisplayFont2PushButton->setFont(font);
        DisplayFont2PushButton->setAutoDefault(false);

        gridLayout9->addWidget(DisplayFont2PushButton, 3, 1, 1, 1);

        DisplayFont1PushButton = new QPushButton(TimeDisplayGroupBox);
        DisplayFont1PushButton->setObjectName("DisplayFont1PushButton");
        DisplayFont1PushButton->setFont(font);
        DisplayFont1PushButton->setAutoDefault(false);

        gridLayout9->addWidget(DisplayFont1PushButton, 1, 1, 1, 1);

        DisplayFont2Label = new QLabel(TimeDisplayGroupBox);
        DisplayFont2Label->setObjectName("DisplayFont2Label");
        DisplayFont2Label->setFont(font);
        DisplayFont2Label->setWordWrap(false);

        gridLayout9->addWidget(DisplayFont2Label, 2, 0, 1, 2);

        DisplayBlinkCheckBox = new QCheckBox(TimeDisplayGroupBox);
        DisplayBlinkCheckBox->setObjectName("DisplayBlinkCheckBox");
        DisplayBlinkCheckBox->setFont(font);

        gridLayout9->addWidget(DisplayBlinkCheckBox, 4, 0, 1, 2);


        gridLayout8->addLayout(gridLayout9, 0, 2, 2, 1);


        vboxLayout5->addWidget(TimeDisplayGroupBox);

        DisplayCustomGroupBox = new QGroupBox(DisplayTabPage);
        DisplayCustomGroupBox->setObjectName("DisplayCustomGroupBox");
        DisplayCustomGroupBox->setFont(font3);
        DisplayCustomGroupBox->setFlat(true);
        gridLayout10 = new QGridLayout(DisplayCustomGroupBox);
        gridLayout10->setSpacing(4);
        gridLayout10->setContentsMargins(8, 8, 8, 8);
        gridLayout10->setObjectName("gridLayout10");
        CustomColorThemeTextLabel = new QLabel(DisplayCustomGroupBox);
        CustomColorThemeTextLabel->setObjectName("CustomColorThemeTextLabel");
        CustomColorThemeTextLabel->setFont(font);
        CustomColorThemeTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout10->addWidget(CustomColorThemeTextLabel, 0, 0, 1, 1);

        CustomColorThemeComboBox = new QComboBox(DisplayCustomGroupBox);
        CustomColorThemeComboBox->addItem(QString());
        CustomColorThemeComboBox->addItem(QString());
        CustomColorThemeComboBox->addItem(QString());
        CustomColorThemeComboBox->setObjectName("CustomColorThemeComboBox");
        CustomColorThemeComboBox->setFont(font);
        CustomColorThemeComboBox->setEditable(false);

        gridLayout10->addWidget(CustomColorThemeComboBox, 0, 1, 1, 1);

        CustomColorThemeToolButton = new QToolButton(DisplayCustomGroupBox);
        CustomColorThemeToolButton->setObjectName("CustomColorThemeToolButton");
        CustomColorThemeToolButton->setMinimumSize(QSize(22, 22));
        CustomColorThemeToolButton->setMaximumSize(QSize(24, 24));
        CustomColorThemeToolButton->setFont(font);
        CustomColorThemeToolButton->setFocusPolicy(Qt::TabFocus);

        gridLayout10->addWidget(CustomColorThemeToolButton, 0, 3, 1, 1);

        spacerItem17 = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout10->addItem(spacerItem17, 0, 4, 1, 1);

        CustomStyleThemeTextLabel = new QLabel(DisplayCustomGroupBox);
        CustomStyleThemeTextLabel->setObjectName("CustomStyleThemeTextLabel");
        CustomStyleThemeTextLabel->setFont(font);

        gridLayout10->addWidget(CustomStyleThemeTextLabel, 0, 5, 1, 1);

        CustomStyleThemeComboBox = new QComboBox(DisplayCustomGroupBox);
        CustomStyleThemeComboBox->addItem(QString());
        CustomStyleThemeComboBox->setObjectName("CustomStyleThemeComboBox");
        CustomStyleThemeComboBox->setFont(font);
        CustomStyleThemeComboBox->setEditable(false);

        gridLayout10->addWidget(CustomStyleThemeComboBox, 0, 6, 1, 1);


        vboxLayout5->addWidget(DisplayCustomGroupBox);

        spacerItem18 = new QSpacerItem(20, 4, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout5->addItem(spacerItem18);

        MessagesWindowGroupBox = new QGroupBox(DisplayTabPage);
        MessagesWindowGroupBox->setObjectName("MessagesWindowGroupBox");
        MessagesWindowGroupBox->setFont(font3);
        MessagesWindowGroupBox->setFlat(true);
        hboxLayout6 = new QHBoxLayout(MessagesWindowGroupBox);
        hboxLayout6->setSpacing(4);
        hboxLayout6->setContentsMargins(8, 8, 8, 8);
        hboxLayout6->setObjectName("hboxLayout6");
        MessagesFontTextLabel = new QLabel(MessagesWindowGroupBox);
        MessagesFontTextLabel->setObjectName("MessagesFontTextLabel");
        MessagesFontTextLabel->setMinimumSize(QSize(180, 0));
        MessagesFontTextLabel->setMaximumSize(QSize(260, 16777215));
        MessagesFontTextLabel->setFont(font);
        MessagesFontTextLabel->setAutoFillBackground(true);
        MessagesFontTextLabel->setFrameShape(QFrame::StyledPanel);
        MessagesFontTextLabel->setFrameShadow(QFrame::Sunken);
        MessagesFontTextLabel->setAlignment(Qt::AlignCenter);
        MessagesFontTextLabel->setWordWrap(false);

        hboxLayout6->addWidget(MessagesFontTextLabel);

        MessagesFontPushButton = new QPushButton(MessagesWindowGroupBox);
        MessagesFontPushButton->setObjectName("MessagesFontPushButton");
        MessagesFontPushButton->setFont(font);
        MessagesFontPushButton->setAutoDefault(false);

        hboxLayout6->addWidget(MessagesFontPushButton);

        spacerItem19 = new QSpacerItem(8, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout6->addItem(spacerItem19);

        MessagesLimitCheckBox = new QCheckBox(MessagesWindowGroupBox);
        MessagesLimitCheckBox->setObjectName("MessagesLimitCheckBox");
        MessagesLimitCheckBox->setFont(font);

        hboxLayout6->addWidget(MessagesLimitCheckBox);

        MessagesLimitLinesComboBox = new QComboBox(MessagesWindowGroupBox);
        MessagesLimitLinesComboBox->addItem(QString());
        MessagesLimitLinesComboBox->addItem(QString());
        MessagesLimitLinesComboBox->addItem(QString());
        MessagesLimitLinesComboBox->addItem(QString());
        MessagesLimitLinesComboBox->addItem(QString());
        MessagesLimitLinesComboBox->addItem(QString());
        MessagesLimitLinesComboBox->setObjectName("MessagesLimitLinesComboBox");
        MessagesLimitLinesComboBox->setFont(font);
        MessagesLimitLinesComboBox->setEditable(true);

        hboxLayout6->addWidget(MessagesLimitLinesComboBox);


        vboxLayout5->addWidget(MessagesWindowGroupBox);

        spacerItem20 = new QSpacerItem(20, 4, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout5->addItem(spacerItem20);

        ConnectionsWindowGroupBox = new QGroupBox(DisplayTabPage);
        ConnectionsWindowGroupBox->setObjectName("ConnectionsWindowGroupBox");
        ConnectionsWindowGroupBox->setFont(font3);
        ConnectionsWindowGroupBox->setFlat(true);
        gridLayout11 = new QGridLayout(ConnectionsWindowGroupBox);
        gridLayout11->setSpacing(4);
        gridLayout11->setContentsMargins(8, 8, 8, 8);
        gridLayout11->setObjectName("gridLayout11");
        ConnectionsFontTextLabel = new QLabel(ConnectionsWindowGroupBox);
        ConnectionsFontTextLabel->setObjectName("ConnectionsFontTextLabel");
        ConnectionsFontTextLabel->setMinimumSize(QSize(180, 0));
        ConnectionsFontTextLabel->setMaximumSize(QSize(260, 16777215));
        ConnectionsFontTextLabel->setFont(font);
        ConnectionsFontTextLabel->setAutoFillBackground(true);
        ConnectionsFontTextLabel->setFrameShape(QFrame::StyledPanel);
        ConnectionsFontTextLabel->setFrameShadow(QFrame::Sunken);
        ConnectionsFontTextLabel->setAlignment(Qt::AlignCenter);
        ConnectionsFontTextLabel->setWordWrap(false);

        gridLayout11->addWidget(ConnectionsFontTextLabel, 0, 0, 1, 1);

        ConnectionsFontPushButton = new QPushButton(ConnectionsWindowGroupBox);
        ConnectionsFontPushButton->setObjectName("ConnectionsFontPushButton");
        ConnectionsFontPushButton->setFont(font);
        ConnectionsFontPushButton->setAutoDefault(false);

        gridLayout11->addWidget(ConnectionsFontPushButton, 0, 1, 1, 1);

        spacerItem21 = new QSpacerItem(8, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout11->addItem(spacerItem21, 0, 2, 1, 2);

        ConnectionsIconSizeTextLabel = new QLabel(ConnectionsWindowGroupBox);
        ConnectionsIconSizeTextLabel->setObjectName("ConnectionsIconSizeTextLabel");
        ConnectionsIconSizeTextLabel->setFont(font);
        ConnectionsIconSizeTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        ConnectionsIconSizeTextLabel->setWordWrap(false);

        gridLayout11->addWidget(ConnectionsIconSizeTextLabel, 0, 4, 1, 1);

        ConnectionsIconSizeComboBox = new QComboBox(ConnectionsWindowGroupBox);
        ConnectionsIconSizeComboBox->addItem(QString());
        ConnectionsIconSizeComboBox->addItem(QString());
        ConnectionsIconSizeComboBox->addItem(QString());
        ConnectionsIconSizeComboBox->setObjectName("ConnectionsIconSizeComboBox");
        ConnectionsIconSizeComboBox->setFont(font);
        ConnectionsIconSizeComboBox->setEditable(false);

        gridLayout11->addWidget(ConnectionsIconSizeComboBox, 0, 5, 1, 1);

        spacerItem22 = new QSpacerItem(20, 4, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout11->addItem(spacerItem22, 1, 0, 1, 6);

        AliasesEnabledCheckBox = new QCheckBox(ConnectionsWindowGroupBox);
        AliasesEnabledCheckBox->setObjectName("AliasesEnabledCheckBox");
        AliasesEnabledCheckBox->setFont(font);

        gridLayout11->addWidget(AliasesEnabledCheckBox, 2, 0, 1, 3);

        JackClientPortAliasTextLabel = new QLabel(ConnectionsWindowGroupBox);
        JackClientPortAliasTextLabel->setObjectName("JackClientPortAliasTextLabel");
        JackClientPortAliasTextLabel->setFont(font);
        JackClientPortAliasTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        JackClientPortAliasTextLabel->setWordWrap(false);

        gridLayout11->addWidget(JackClientPortAliasTextLabel, 2, 3, 1, 2);

        JackClientPortAliasComboBox = new QComboBox(ConnectionsWindowGroupBox);
        JackClientPortAliasComboBox->addItem(QString());
        JackClientPortAliasComboBox->addItem(QString());
        JackClientPortAliasComboBox->addItem(QString());
        JackClientPortAliasComboBox->setObjectName("JackClientPortAliasComboBox");
        JackClientPortAliasComboBox->setFont(font);
        JackClientPortAliasComboBox->setEditable(false);

        gridLayout11->addWidget(JackClientPortAliasComboBox, 2, 5, 1, 1);

        AliasesEditingCheckBox = new QCheckBox(ConnectionsWindowGroupBox);
        AliasesEditingCheckBox->setObjectName("AliasesEditingCheckBox");
        AliasesEditingCheckBox->setFont(font);

        gridLayout11->addWidget(AliasesEditingCheckBox, 3, 0, 1, 3);

        JackClientPortMetadataCheckBox = new QCheckBox(ConnectionsWindowGroupBox);
        JackClientPortMetadataCheckBox->setObjectName("JackClientPortMetadataCheckBox");
        JackClientPortMetadataCheckBox->setFont(font);

        gridLayout11->addWidget(JackClientPortMetadataCheckBox, 3, 3, 1, 3);


        vboxLayout5->addWidget(ConnectionsWindowGroupBox);

        SetupTabWidget->addTab(DisplayTabPage, QString());
        MiscTabPage = new QWidget();
        MiscTabPage->setObjectName("MiscTabPage");
        gridLayout12 = new QGridLayout(MiscTabPage);
        gridLayout12->setSpacing(4);
        gridLayout12->setContentsMargins(4, 4, 4, 4);
        gridLayout12->setObjectName("gridLayout12");
        OtherGroupBox = new QGroupBox(MiscTabPage);
        OtherGroupBox->setObjectName("OtherGroupBox");
        OtherGroupBox->setFont(font3);
        OtherGroupBox->setFlat(true);
        gridLayout13 = new QGridLayout(OtherGroupBox);
        gridLayout13->setSpacing(4);
        gridLayout13->setContentsMargins(4, 4, 4, 4);
        gridLayout13->setObjectName("gridLayout13");
        vboxLayout7 = new QVBoxLayout();
        vboxLayout7->setSpacing(4);
        vboxLayout7->setContentsMargins(0, 0, 0, 0);
        vboxLayout7->setObjectName("vboxLayout7");
        StartJackCheckBox = new QCheckBox(OtherGroupBox);
        StartJackCheckBox->setObjectName("StartJackCheckBox");
        StartJackCheckBox->setFont(font);

        vboxLayout7->addWidget(StartJackCheckBox);

        QueryCloseCheckBox = new QCheckBox(OtherGroupBox);
        QueryCloseCheckBox->setObjectName("QueryCloseCheckBox");
        QueryCloseCheckBox->setFont(font);

        vboxLayout7->addWidget(QueryCloseCheckBox);

        QueryShutdownCheckBox = new QCheckBox(OtherGroupBox);
        QueryShutdownCheckBox->setObjectName("QueryShutdownCheckBox");
        QueryShutdownCheckBox->setFont(font);

        vboxLayout7->addWidget(QueryShutdownCheckBox);

        KeepOnTopCheckBox = new QCheckBox(OtherGroupBox);
        KeepOnTopCheckBox->setObjectName("KeepOnTopCheckBox");
        KeepOnTopCheckBox->setFont(font2);

        vboxLayout7->addWidget(KeepOnTopCheckBox);

        SystemTrayCheckBox = new QCheckBox(OtherGroupBox);
        SystemTrayCheckBox->setObjectName("SystemTrayCheckBox");
        SystemTrayCheckBox->setFont(font);

        vboxLayout7->addWidget(SystemTrayCheckBox);

        SystemTrayQueryCloseCheckBox = new QCheckBox(OtherGroupBox);
        SystemTrayQueryCloseCheckBox->setObjectName("SystemTrayQueryCloseCheckBox");
        SystemTrayQueryCloseCheckBox->setFont(font);

        vboxLayout7->addWidget(SystemTrayQueryCloseCheckBox);

        StartMinimizedCheckBox = new QCheckBox(OtherGroupBox);
        StartMinimizedCheckBox->setObjectName("StartMinimizedCheckBox");
        StartMinimizedCheckBox->setFont(font);

        vboxLayout7->addWidget(StartMinimizedCheckBox);


        gridLayout13->addLayout(vboxLayout7, 0, 0, 1, 1);

        vboxLayout8 = new QVBoxLayout();
        vboxLayout8->setSpacing(4);
        vboxLayout8->setContentsMargins(0, 0, 0, 0);
        vboxLayout8->setObjectName("vboxLayout8");
        ServerConfigCheckBox = new QCheckBox(OtherGroupBox);
        ServerConfigCheckBox->setObjectName("ServerConfigCheckBox");
        ServerConfigCheckBox->setFont(font);

        vboxLayout8->addWidget(ServerConfigCheckBox);

        ServerConfigNameComboBox = new QComboBox(OtherGroupBox);
        ServerConfigNameComboBox->addItem(QString());
        ServerConfigNameComboBox->setObjectName("ServerConfigNameComboBox");
        ServerConfigNameComboBox->setFont(font);
        ServerConfigNameComboBox->setEditable(true);

        vboxLayout8->addWidget(ServerConfigNameComboBox);

        AlsaSeqEnabledCheckBox = new QCheckBox(OtherGroupBox);
        AlsaSeqEnabledCheckBox->setObjectName("AlsaSeqEnabledCheckBox");
        AlsaSeqEnabledCheckBox->setFont(font);

        vboxLayout8->addWidget(AlsaSeqEnabledCheckBox);

        DBusEnabledCheckBox = new QCheckBox(OtherGroupBox);
        DBusEnabledCheckBox->setObjectName("DBusEnabledCheckBox");
        DBusEnabledCheckBox->setFont(font);

        vboxLayout8->addWidget(DBusEnabledCheckBox);

        JackDBusEnabledCheckBox = new QCheckBox(OtherGroupBox);
        JackDBusEnabledCheckBox->setObjectName("JackDBusEnabledCheckBox");
        JackDBusEnabledCheckBox->setFont(font);

        vboxLayout8->addWidget(JackDBusEnabledCheckBox);

        StopJackCheckBox = new QCheckBox(OtherGroupBox);
        StopJackCheckBox->setObjectName("StopJackCheckBox");
        StopJackCheckBox->setFont(font);

        vboxLayout8->addWidget(StopJackCheckBox);

        SingletonCheckBox = new QCheckBox(OtherGroupBox);
        SingletonCheckBox->setObjectName("SingletonCheckBox");
        SingletonCheckBox->setFont(font);

        vboxLayout8->addWidget(SingletonCheckBox);


        gridLayout13->addLayout(vboxLayout8, 0, 1, 1, 1);

        spacerItem23 = new QSpacerItem(20, 4, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout13->addItem(spacerItem23, 1, 0, 1, 2);


        gridLayout12->addWidget(OtherGroupBox, 0, 0, 1, 2);

        ButtonsGroupBox = new QGroupBox(MiscTabPage);
        ButtonsGroupBox->setObjectName("ButtonsGroupBox");
        ButtonsGroupBox->setFont(font3);
        ButtonsGroupBox->setFlat(true);
        vboxLayout9 = new QVBoxLayout(ButtonsGroupBox);
        vboxLayout9->setSpacing(4);
        vboxLayout9->setContentsMargins(8, 8, 8, 8);
        vboxLayout9->setObjectName("vboxLayout9");
        vboxLayout10 = new QVBoxLayout();
        vboxLayout10->setSpacing(4);
        vboxLayout10->setContentsMargins(0, 0, 0, 0);
        vboxLayout10->setObjectName("vboxLayout10");
        LeftButtonsCheckBox = new QCheckBox(ButtonsGroupBox);
        LeftButtonsCheckBox->setObjectName("LeftButtonsCheckBox");
        LeftButtonsCheckBox->setFont(font);

        vboxLayout10->addWidget(LeftButtonsCheckBox);

        RightButtonsCheckBox = new QCheckBox(ButtonsGroupBox);
        RightButtonsCheckBox->setObjectName("RightButtonsCheckBox");
        RightButtonsCheckBox->setFont(font);

        vboxLayout10->addWidget(RightButtonsCheckBox);

        TransportButtonsCheckBox = new QCheckBox(ButtonsGroupBox);
        TransportButtonsCheckBox->setObjectName("TransportButtonsCheckBox");
        TransportButtonsCheckBox->setFont(font);

        vboxLayout10->addWidget(TransportButtonsCheckBox);

        TextLabelsCheckBox = new QCheckBox(ButtonsGroupBox);
        TextLabelsCheckBox->setObjectName("TextLabelsCheckBox");
        TextLabelsCheckBox->setFont(font);

        vboxLayout10->addWidget(TextLabelsCheckBox);

        spacerItem24 = new QSpacerItem(20, 4, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout10->addItem(spacerItem24);

        GraphButtonCheckBox = new QCheckBox(ButtonsGroupBox);
        GraphButtonCheckBox->setObjectName("GraphButtonCheckBox");
        GraphButtonCheckBox->setFont(font);

        vboxLayout10->addWidget(GraphButtonCheckBox);


        vboxLayout9->addLayout(vboxLayout10);


        gridLayout12->addWidget(ButtonsGroupBox, 1, 0, 1, 1);

        DefaultsGroupBox = new QGroupBox(MiscTabPage);
        DefaultsGroupBox->setObjectName("DefaultsGroupBox");
        DefaultsGroupBox->setFont(font3);
        DefaultsGroupBox->setFlat(true);
        gridLayout14 = new QGridLayout(DefaultsGroupBox);
        gridLayout14->setSpacing(4);
        gridLayout14->setContentsMargins(4, 4, 4, 4);
        gridLayout14->setObjectName("gridLayout14");
        spacerItem25 = new QSpacerItem(20, 4, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout14->addItem(spacerItem25, 1, 0, 1, 3);

        BaseFontSizeTextLabel = new QLabel(DefaultsGroupBox);
        BaseFontSizeTextLabel->setObjectName("BaseFontSizeTextLabel");
        BaseFontSizeTextLabel->setFont(font);
        BaseFontSizeTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout14->addWidget(BaseFontSizeTextLabel, 0, 1, 1, 1);

        BaseFontSizeComboBox = new QComboBox(DefaultsGroupBox);
        BaseFontSizeComboBox->addItem(QString());
        BaseFontSizeComboBox->addItem(QString());
        BaseFontSizeComboBox->addItem(QString());
        BaseFontSizeComboBox->addItem(QString());
        BaseFontSizeComboBox->addItem(QString());
        BaseFontSizeComboBox->addItem(QString());
        BaseFontSizeComboBox->addItem(QString());
        BaseFontSizeComboBox->addItem(QString());
        BaseFontSizeComboBox->setObjectName("BaseFontSizeComboBox");
        BaseFontSizeComboBox->setFont(font);
        BaseFontSizeComboBox->setEditable(true);

        gridLayout14->addWidget(BaseFontSizeComboBox, 0, 2, 1, 1);


        gridLayout12->addWidget(DefaultsGroupBox, 1, 1, 1, 1);

        SetupTabWidget->addTab(MiscTabPage, QString());

        vboxLayout->addWidget(SetupTabWidget);

        DialogButtonBox = new QDialogButtonBox(qjackctlSetupForm);
        DialogButtonBox->setObjectName("DialogButtonBox");
        DialogButtonBox->setOrientation(Qt::Horizontal);
        DialogButtonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Apply|QDialogButtonBox::Ok);

        vboxLayout->addWidget(DialogButtonBox);

#if QT_CONFIG(shortcut)
        PresetTextLabel->setBuddy(PresetComboBox);
        DriverTextLabel->setBuddy(DriverComboBox);
        InterfaceTextLabel->setBuddy(InterfaceComboBox);
        MidiDriverTextLabel->setBuddy(MidiDriverComboBox);
        SampleRateTextLabel->setBuddy(SampleRateComboBox);
        FramesTextLabel->setBuddy(FramesComboBox);
        PeriodsTextLabel->setBuddy(PeriodsSpinBox);
        ServerPrefixTextLabel->setBuddy(ServerPrefixComboBox);
        ServerNameTextLabel->setBuddy(ServerNameComboBox);
        PriorityTextLabel->setBuddy(PrioritySpinBox);
        WordLengthTextLabel->setBuddy(WordLengthComboBox);
        WaitTextLabel->setBuddy(WaitComboBox);
        ChanTextLabel->setBuddy(ChanSpinBox);
        PortMaxTextLabel->setBuddy(PortMaxComboBox);
        TimeoutTextLabel->setBuddy(TimeoutComboBox);
        ClockSourceTextLabel->setBuddy(ClockSourceComboBox);
        AudioTextLabel->setBuddy(AudioComboBox);
        DitherTextLabel->setBuddy(DitherComboBox);
        OutDeviceTextLabel->setBuddy(OutDeviceComboBox);
        InDeviceTextLabel->setBuddy(InDeviceComboBox);
        InOutChannelsTextLabel->setBuddy(InChannelsSpinBox);
        InOutLatencyTextLabel->setBuddy(InLatencySpinBox);
        SelfConnectModeTextLabel->setBuddy(SelfConnectModeComboBox);
        ServerSuffixTextLabel->setBuddy(ServerSuffixComboBox);
        StartDelayTextLabel->setBuddy(StartDelaySpinBox);
        XrunRegexTextLabel->setBuddy(XrunRegexComboBox);
        CustomColorThemeTextLabel->setBuddy(CustomColorThemeComboBox);
        CustomStyleThemeTextLabel->setBuddy(CustomStyleThemeComboBox);
        ConnectionsIconSizeTextLabel->setBuddy(ConnectionsIconSizeComboBox);
        JackClientPortAliasTextLabel->setBuddy(JackClientPortAliasComboBox);
        BaseFontSizeTextLabel->setBuddy(BaseFontSizeComboBox);
#endif // QT_CONFIG(shortcut)
        QWidget::setTabOrder(SetupTabWidget, PresetComboBox);
        QWidget::setTabOrder(PresetComboBox, PresetClearPushButton);
        QWidget::setTabOrder(PresetClearPushButton, PresetSavePushButton);
        QWidget::setTabOrder(PresetSavePushButton, PresetDeletePushButton);
        QWidget::setTabOrder(PresetDeletePushButton, SettingsTabWidget);
        QWidget::setTabOrder(SettingsTabWidget, DriverComboBox);
        QWidget::setTabOrder(DriverComboBox, RealtimeCheckBox);
        QWidget::setTabOrder(RealtimeCheckBox, InterfaceComboBox);
        QWidget::setTabOrder(InterfaceComboBox, SampleRateComboBox);
        QWidget::setTabOrder(SampleRateComboBox, FramesComboBox);
        QWidget::setTabOrder(FramesComboBox, PeriodsSpinBox);
        QWidget::setTabOrder(PeriodsSpinBox, MidiDriverComboBox);
        QWidget::setTabOrder(MidiDriverComboBox, SyncCheckBox);
        QWidget::setTabOrder(SyncCheckBox, VerboseCheckBox);
        QWidget::setTabOrder(VerboseCheckBox, ServerPrefixComboBox);
        QWidget::setTabOrder(ServerPrefixComboBox, ServerNameComboBox);
        QWidget::setTabOrder(ServerNameComboBox, NoMemLockCheckBox);
        QWidget::setTabOrder(NoMemLockCheckBox, UnlockMemCheckBox);
        QWidget::setTabOrder(UnlockMemCheckBox, HWMeterCheckBox);
        QWidget::setTabOrder(HWMeterCheckBox, MonitorCheckBox);
        QWidget::setTabOrder(MonitorCheckBox, SoftModeCheckBox);
        QWidget::setTabOrder(SoftModeCheckBox, ShortsCheckBox);
        QWidget::setTabOrder(ShortsCheckBox, IgnoreHWCheckBox);
        QWidget::setTabOrder(IgnoreHWCheckBox, PrioritySpinBox);
        QWidget::setTabOrder(PrioritySpinBox, WordLengthComboBox);
        QWidget::setTabOrder(WordLengthComboBox, WaitComboBox);
        QWidget::setTabOrder(WaitComboBox, ChanSpinBox);
        QWidget::setTabOrder(ChanSpinBox, PortMaxComboBox);
        QWidget::setTabOrder(PortMaxComboBox, TimeoutComboBox);
        QWidget::setTabOrder(TimeoutComboBox, AudioComboBox);
        QWidget::setTabOrder(AudioComboBox, ClockSourceComboBox);
        QWidget::setTabOrder(ClockSourceComboBox, DitherComboBox);
        QWidget::setTabOrder(DitherComboBox, OutDeviceComboBox);
        QWidget::setTabOrder(OutDeviceComboBox, InDeviceComboBox);
        QWidget::setTabOrder(InDeviceComboBox, InChannelsSpinBox);
        QWidget::setTabOrder(InChannelsSpinBox, OutChannelsSpinBox);
        QWidget::setTabOrder(OutChannelsSpinBox, InLatencySpinBox);
        QWidget::setTabOrder(InLatencySpinBox, OutLatencySpinBox);
        QWidget::setTabOrder(OutLatencySpinBox, SelfConnectModeComboBox);
        QWidget::setTabOrder(SelfConnectModeComboBox, ServerSuffixComboBox);
        QWidget::setTabOrder(ServerSuffixComboBox, StartDelaySpinBox);
        QWidget::setTabOrder(StartDelaySpinBox, StartupScriptCheckBox);
        QWidget::setTabOrder(StartupScriptCheckBox, StartupScriptShellComboBox);
        QWidget::setTabOrder(StartupScriptShellComboBox, StartupScriptSymbolToolButton);
        QWidget::setTabOrder(StartupScriptSymbolToolButton, StartupScriptBrowseToolButton);
        QWidget::setTabOrder(StartupScriptBrowseToolButton, PostStartupScriptCheckBox);
        QWidget::setTabOrder(PostStartupScriptCheckBox, PostStartupScriptShellComboBox);
        QWidget::setTabOrder(PostStartupScriptShellComboBox, PostStartupScriptSymbolToolButton);
        QWidget::setTabOrder(PostStartupScriptSymbolToolButton, PostStartupScriptBrowseToolButton);
        QWidget::setTabOrder(PostStartupScriptBrowseToolButton, ShutdownScriptCheckBox);
        QWidget::setTabOrder(ShutdownScriptCheckBox, ShutdownScriptShellComboBox);
        QWidget::setTabOrder(ShutdownScriptShellComboBox, ShutdownScriptSymbolToolButton);
        QWidget::setTabOrder(ShutdownScriptSymbolToolButton, ShutdownScriptBrowseToolButton);
        QWidget::setTabOrder(ShutdownScriptBrowseToolButton, PostShutdownScriptCheckBox);
        QWidget::setTabOrder(PostShutdownScriptCheckBox, PostShutdownScriptShellComboBox);
        QWidget::setTabOrder(PostShutdownScriptShellComboBox, PostShutdownScriptSymbolToolButton);
        QWidget::setTabOrder(PostShutdownScriptSymbolToolButton, PostShutdownScriptBrowseToolButton);
        QWidget::setTabOrder(PostShutdownScriptBrowseToolButton, StdoutCaptureCheckBox);
        QWidget::setTabOrder(StdoutCaptureCheckBox, XrunRegexComboBox);
        QWidget::setTabOrder(XrunRegexComboBox, ActivePatchbayCheckBox);
        QWidget::setTabOrder(ActivePatchbayCheckBox, ActivePatchbayPathComboBox);
        QWidget::setTabOrder(ActivePatchbayPathComboBox, ActivePatchbayPathToolButton);
        QWidget::setTabOrder(ActivePatchbayPathToolButton, ActivePatchbayResetCheckBox);
        QWidget::setTabOrder(ActivePatchbayResetCheckBox, QueryDisconnectCheckBox);
        QWidget::setTabOrder(QueryDisconnectCheckBox, MessagesLogCheckBox);
        QWidget::setTabOrder(MessagesLogCheckBox, MessagesLogPathComboBox);
        QWidget::setTabOrder(MessagesLogPathComboBox, MessagesLogPathToolButton);
        QWidget::setTabOrder(MessagesLogPathToolButton, TransportTimeRadioButton);
        QWidget::setTabOrder(TransportTimeRadioButton, TransportBBTRadioButton);
        QWidget::setTabOrder(TransportBBTRadioButton, ElapsedResetRadioButton);
        QWidget::setTabOrder(ElapsedResetRadioButton, ElapsedXrunRadioButton);
        QWidget::setTabOrder(ElapsedXrunRadioButton, DisplayFont1PushButton);
        QWidget::setTabOrder(DisplayFont1PushButton, DisplayFont2PushButton);
        QWidget::setTabOrder(DisplayFont2PushButton, DisplayBlinkCheckBox);
        QWidget::setTabOrder(DisplayBlinkCheckBox, CustomColorThemeComboBox);
        QWidget::setTabOrder(CustomColorThemeComboBox, CustomColorThemeToolButton);
        QWidget::setTabOrder(CustomColorThemeToolButton, CustomStyleThemeComboBox);
        QWidget::setTabOrder(CustomStyleThemeComboBox, MessagesFontPushButton);
        QWidget::setTabOrder(MessagesFontPushButton, MessagesLimitCheckBox);
        QWidget::setTabOrder(MessagesLimitCheckBox, MessagesLimitLinesComboBox);
        QWidget::setTabOrder(MessagesLimitLinesComboBox, ConnectionsFontPushButton);
        QWidget::setTabOrder(ConnectionsFontPushButton, ConnectionsIconSizeComboBox);
        QWidget::setTabOrder(ConnectionsIconSizeComboBox, AliasesEnabledCheckBox);
        QWidget::setTabOrder(AliasesEnabledCheckBox, AliasesEditingCheckBox);
        QWidget::setTabOrder(AliasesEditingCheckBox, JackClientPortAliasComboBox);
        QWidget::setTabOrder(JackClientPortAliasComboBox, JackClientPortMetadataCheckBox);
        QWidget::setTabOrder(JackClientPortMetadataCheckBox, StartJackCheckBox);
        QWidget::setTabOrder(StartJackCheckBox, QueryCloseCheckBox);
        QWidget::setTabOrder(QueryCloseCheckBox, QueryShutdownCheckBox);
        QWidget::setTabOrder(QueryShutdownCheckBox, KeepOnTopCheckBox);
        QWidget::setTabOrder(KeepOnTopCheckBox, SystemTrayCheckBox);
        QWidget::setTabOrder(SystemTrayCheckBox, SystemTrayQueryCloseCheckBox);
        QWidget::setTabOrder(SystemTrayQueryCloseCheckBox, StartMinimizedCheckBox);
        QWidget::setTabOrder(StartMinimizedCheckBox, ServerConfigCheckBox);
        QWidget::setTabOrder(ServerConfigCheckBox, ServerConfigNameComboBox);
        QWidget::setTabOrder(ServerConfigNameComboBox, AlsaSeqEnabledCheckBox);
        QWidget::setTabOrder(AlsaSeqEnabledCheckBox, DBusEnabledCheckBox);
        QWidget::setTabOrder(DBusEnabledCheckBox, JackDBusEnabledCheckBox);
        QWidget::setTabOrder(JackDBusEnabledCheckBox, StopJackCheckBox);
        QWidget::setTabOrder(StopJackCheckBox, SingletonCheckBox);
        QWidget::setTabOrder(SingletonCheckBox, LeftButtonsCheckBox);
        QWidget::setTabOrder(LeftButtonsCheckBox, RightButtonsCheckBox);
        QWidget::setTabOrder(RightButtonsCheckBox, TransportButtonsCheckBox);
        QWidget::setTabOrder(TransportButtonsCheckBox, TextLabelsCheckBox);
        QWidget::setTabOrder(TextLabelsCheckBox, BaseFontSizeComboBox);
        QWidget::setTabOrder(BaseFontSizeComboBox, DialogButtonBox);

        retranslateUi(qjackctlSetupForm);

        SetupTabWidget->setCurrentIndex(0);
        SettingsTabWidget->setCurrentIndex(0);
        PortMaxComboBox->setCurrentIndex(0);
        TimeoutComboBox->setCurrentIndex(0);
        ClockSourceComboBox->setCurrentIndex(0);
        SelfConnectModeComboBox->setCurrentIndex(0);
        MessagesLimitLinesComboBox->setCurrentIndex(3);
        ConnectionsIconSizeComboBox->setCurrentIndex(0);
        JackClientPortAliasComboBox->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(qjackctlSetupForm);
    } // setupUi

    void retranslateUi(QDialog *qjackctlSetupForm)
    {
        qjackctlSetupForm->setWindowTitle(QCoreApplication::translate("qjackctlSetupForm", "Setup", nullptr));
        PresetTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "Preset &Name:", nullptr));
        PresetComboBox->setItemText(0, QCoreApplication::translate("qjackctlSetupForm", "(default)", nullptr));

#if QT_CONFIG(tooltip)
        PresetComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Settings preset name", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        PresetClearPushButton->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Clear settings of current preset name", nullptr));
#endif // QT_CONFIG(tooltip)
        PresetClearPushButton->setText(QCoreApplication::translate("qjackctlSetupForm", "Clea&r", nullptr));
#if QT_CONFIG(tooltip)
        PresetSavePushButton->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Save settings as current preset name", nullptr));
#endif // QT_CONFIG(tooltip)
        PresetSavePushButton->setText(QCoreApplication::translate("qjackctlSetupForm", "&Save", nullptr));
#if QT_CONFIG(tooltip)
        PresetDeletePushButton->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Delete current settings preset", nullptr));
#endif // QT_CONFIG(tooltip)
        PresetDeletePushButton->setText(QCoreApplication::translate("qjackctlSetupForm", "&Delete", nullptr));
        DriverTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "Driv&er:", nullptr));
        DriverComboBox->setItemText(0, QCoreApplication::translate("qjackctlSetupForm", "dummy", nullptr));
        DriverComboBox->setItemText(1, QCoreApplication::translate("qjackctlSetupForm", "sun", nullptr));
        DriverComboBox->setItemText(2, QCoreApplication::translate("qjackctlSetupForm", "oss", nullptr));
        DriverComboBox->setItemText(3, QCoreApplication::translate("qjackctlSetupForm", "alsa", nullptr));
        DriverComboBox->setItemText(4, QCoreApplication::translate("qjackctlSetupForm", "portaudio", nullptr));
        DriverComboBox->setItemText(5, QCoreApplication::translate("qjackctlSetupForm", "coreaudio", nullptr));
        DriverComboBox->setItemText(6, QCoreApplication::translate("qjackctlSetupForm", "firewire", nullptr));
        DriverComboBox->setItemText(7, QCoreApplication::translate("qjackctlSetupForm", "net", nullptr));
        DriverComboBox->setItemText(8, QCoreApplication::translate("qjackctlSetupForm", "netone", nullptr));

#if QT_CONFIG(tooltip)
        DriverComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "The audio backend driver interface to use", nullptr));
#endif // QT_CONFIG(tooltip)
        InterfaceTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "&Interface:", nullptr));
        InterfaceComboBox->setItemText(0, QCoreApplication::translate("qjackctlSetupForm", "(default)", nullptr));
        InterfaceComboBox->setItemText(1, QCoreApplication::translate("qjackctlSetupForm", "hw:0", nullptr));
        InterfaceComboBox->setItemText(2, QCoreApplication::translate("qjackctlSetupForm", "plughw:0", nullptr));
        InterfaceComboBox->setItemText(3, QCoreApplication::translate("qjackctlSetupForm", "/dev/audio", nullptr));
        InterfaceComboBox->setItemText(4, QCoreApplication::translate("qjackctlSetupForm", "/dev/dsp", nullptr));

#if QT_CONFIG(tooltip)
        InterfaceComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "The PCM device name to use", nullptr));
#endif // QT_CONFIG(tooltip)
        MidiDriverTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "MIDI Driv&er:", nullptr));
        MidiDriverComboBox->setItemText(0, QCoreApplication::translate("qjackctlSetupForm", "none", nullptr));
        MidiDriverComboBox->setItemText(1, QCoreApplication::translate("qjackctlSetupForm", "raw", nullptr));
        MidiDriverComboBox->setItemText(2, QCoreApplication::translate("qjackctlSetupForm", "seq", nullptr));

#if QT_CONFIG(tooltip)
        MidiDriverComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "The ALSA MIDI backend driver to use", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        RealtimeCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Use realtime scheduling", nullptr));
#endif // QT_CONFIG(tooltip)
        RealtimeCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "&Realtime", nullptr));
        SampleRateTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "Sample &Rate:", nullptr));
        SampleRateComboBox->setItemText(0, QCoreApplication::translate("qjackctlSetupForm", "22050", nullptr));
        SampleRateComboBox->setItemText(1, QCoreApplication::translate("qjackctlSetupForm", "32000", nullptr));
        SampleRateComboBox->setItemText(2, QCoreApplication::translate("qjackctlSetupForm", "44100", nullptr));
        SampleRateComboBox->setItemText(3, QCoreApplication::translate("qjackctlSetupForm", "48000", nullptr));
        SampleRateComboBox->setItemText(4, QCoreApplication::translate("qjackctlSetupForm", "88200", nullptr));
        SampleRateComboBox->setItemText(5, QCoreApplication::translate("qjackctlSetupForm", "96000", nullptr));
        SampleRateComboBox->setItemText(6, QCoreApplication::translate("qjackctlSetupForm", "192000", nullptr));

#if QT_CONFIG(tooltip)
        SampleRateComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Sample rate in frames per second", nullptr));
#endif // QT_CONFIG(tooltip)
        FramesTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "&Frames/Period:", nullptr));
        FramesComboBox->setItemText(0, QCoreApplication::translate("qjackctlSetupForm", "16", nullptr));
        FramesComboBox->setItemText(1, QCoreApplication::translate("qjackctlSetupForm", "32", nullptr));
        FramesComboBox->setItemText(2, QCoreApplication::translate("qjackctlSetupForm", "64", nullptr));
        FramesComboBox->setItemText(3, QCoreApplication::translate("qjackctlSetupForm", "128", nullptr));
        FramesComboBox->setItemText(4, QCoreApplication::translate("qjackctlSetupForm", "256", nullptr));
        FramesComboBox->setItemText(5, QCoreApplication::translate("qjackctlSetupForm", "512", nullptr));
        FramesComboBox->setItemText(6, QCoreApplication::translate("qjackctlSetupForm", "1024", nullptr));
        FramesComboBox->setItemText(7, QCoreApplication::translate("qjackctlSetupForm", "2048", nullptr));
        FramesComboBox->setItemText(8, QCoreApplication::translate("qjackctlSetupForm", "4096", nullptr));

#if QT_CONFIG(tooltip)
        FramesComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Frames per period between process() calls", nullptr));
#endif // QT_CONFIG(tooltip)
        PeriodsTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "Periods/&Buffer:", nullptr));
#if QT_CONFIG(tooltip)
        PeriodsSpinBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Number of periods in the hardware buffer", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        SyncCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to use server synchronous mode", nullptr));
#endif // QT_CONFIG(tooltip)
        SyncCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "&Use server synchronous mode", nullptr));
#if QT_CONFIG(tooltip)
        VerboseCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to give verbose output on messages", nullptr));
#endif // QT_CONFIG(tooltip)
        VerboseCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "&Verbose messages", nullptr));
        LatencyTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "Latency:", nullptr));
#if QT_CONFIG(tooltip)
        LatencyTextValue->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Output latency in milliseconds, calculated based on the period, rate and buffer settings", nullptr));
#endif // QT_CONFIG(tooltip)
        LatencyTextValue->setText(QCoreApplication::translate("qjackctlSetupForm", "0 msecs", nullptr));
        SettingsTabWidget->setTabText(SettingsTabWidget->indexOf(ParametersTab), QCoreApplication::translate("qjackctlSetupForm", "Parameters", nullptr));
        AdvancedTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "Please do not touch these settings unless you know what you are doing.", nullptr));
        ServerPrefixTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "Server &Prefix:", nullptr));
        ServerPrefixComboBox->setItemText(0, QCoreApplication::translate("qjackctlSetupForm", "jackd", nullptr));
        ServerPrefixComboBox->setItemText(1, QCoreApplication::translate("qjackctlSetupForm", "jackdmp", nullptr));
        ServerPrefixComboBox->setItemText(2, QCoreApplication::translate("qjackctlSetupForm", "jackstart", nullptr));

#if QT_CONFIG(tooltip)
        ServerPrefixComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Server path (command line prefix)", nullptr));
#endif // QT_CONFIG(tooltip)
        ServerNameTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "&Name:", nullptr));
        ServerNameComboBox->setItemText(0, QCoreApplication::translate("qjackctlSetupForm", "(default)", nullptr));

#if QT_CONFIG(tooltip)
        ServerNameComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "The JACK Audio Connection Kit sound server name", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        NoMemLockCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Do not attempt to lock memory, even if in realtime mode", nullptr));
#endif // QT_CONFIG(tooltip)
        NoMemLockCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "No Memory Loc&k", nullptr));
#if QT_CONFIG(tooltip)
        UnlockMemCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Unlock memory of common toolkit libraries (GTK+, QT, FLTK, Wine)", nullptr));
#endif // QT_CONFIG(tooltip)
        UnlockMemCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "&Unlock Memory", nullptr));
#if QT_CONFIG(tooltip)
        HWMeterCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Enable hardware metering on cards that support it", nullptr));
#endif // QT_CONFIG(tooltip)
        HWMeterCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "H/&W Meter", nullptr));
#if QT_CONFIG(tooltip)
        MonitorCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Provide output monitor ports", nullptr));
#endif // QT_CONFIG(tooltip)
        MonitorCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "&Monitor", nullptr));
#if QT_CONFIG(tooltip)
        SoftModeCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Ignore xruns reported by the backend driver", nullptr));
#endif // QT_CONFIG(tooltip)
        SoftModeCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "So&ft Mode", nullptr));
#if QT_CONFIG(tooltip)
        ShortsCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Force 16bit mode instead of failing over 32bit (default)", nullptr));
#endif // QT_CONFIG(tooltip)
        ShortsCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "Force &16bit", nullptr));
#if QT_CONFIG(tooltip)
        IgnoreHWCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Ignore hardware period/buffer size", nullptr));
#endif // QT_CONFIG(tooltip)
        IgnoreHWCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "&Ignore H/W", nullptr));
        PriorityTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "Priorit&y:", nullptr));
#if QT_CONFIG(tooltip)
        PrioritySpinBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Scheduler priority when running realtime", nullptr));
#endif // QT_CONFIG(tooltip)
        WordLengthTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "&Word Length:", nullptr));
        WordLengthComboBox->setItemText(0, QCoreApplication::translate("qjackctlSetupForm", "16", nullptr));
        WordLengthComboBox->setItemText(1, QCoreApplication::translate("qjackctlSetupForm", "32", nullptr));
        WordLengthComboBox->setItemText(2, QCoreApplication::translate("qjackctlSetupForm", "64", nullptr));

#if QT_CONFIG(tooltip)
        WordLengthComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Word length", nullptr));
#endif // QT_CONFIG(tooltip)
        WaitTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "&Wait (usec):", nullptr));
        WaitComboBox->setItemText(0, QCoreApplication::translate("qjackctlSetupForm", "21333", nullptr));

#if QT_CONFIG(tooltip)
        WaitComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Number of microseconds to wait between engine processes (dummy)", nullptr));
#endif // QT_CONFIG(tooltip)
        ChanTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "&Channels:", nullptr));
#if QT_CONFIG(tooltip)
        ChanSpinBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Maximum number of audio channels to allocate", nullptr));
#endif // QT_CONFIG(tooltip)
        PortMaxTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "Port Ma&ximum:", nullptr));
        PortMaxComboBox->setItemText(0, QCoreApplication::translate("qjackctlSetupForm", "128", nullptr));
        PortMaxComboBox->setItemText(1, QCoreApplication::translate("qjackctlSetupForm", "256", nullptr));
        PortMaxComboBox->setItemText(2, QCoreApplication::translate("qjackctlSetupForm", "512", nullptr));
        PortMaxComboBox->setItemText(3, QCoreApplication::translate("qjackctlSetupForm", "1024", nullptr));

#if QT_CONFIG(tooltip)
        PortMaxComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Maximum number of ports the JACK server can manage", nullptr));
#endif // QT_CONFIG(tooltip)
        TimeoutTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "&Timeout (msec):", nullptr));
        TimeoutComboBox->setItemText(0, QCoreApplication::translate("qjackctlSetupForm", "200", nullptr));
        TimeoutComboBox->setItemText(1, QCoreApplication::translate("qjackctlSetupForm", "500", nullptr));
        TimeoutComboBox->setItemText(2, QCoreApplication::translate("qjackctlSetupForm", "1000", nullptr));
        TimeoutComboBox->setItemText(3, QCoreApplication::translate("qjackctlSetupForm", "2000", nullptr));
        TimeoutComboBox->setItemText(4, QCoreApplication::translate("qjackctlSetupForm", "5000", nullptr));

#if QT_CONFIG(tooltip)
        TimeoutComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Set client timeout limit in milliseconds", nullptr));
#endif // QT_CONFIG(tooltip)
        ClockSourceTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "Cloc&k source:", nullptr));
#if QT_CONFIG(tooltip)
        ClockSourceComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Clock source", nullptr));
#endif // QT_CONFIG(tooltip)
        AudioTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "&Audio:", nullptr));
        AudioComboBox->setItemText(0, QCoreApplication::translate("qjackctlSetupForm", "Duplex", nullptr));
        AudioComboBox->setItemText(1, QCoreApplication::translate("qjackctlSetupForm", "Capture Only", nullptr));
        AudioComboBox->setItemText(2, QCoreApplication::translate("qjackctlSetupForm", "Playback Only", nullptr));

#if QT_CONFIG(tooltip)
        AudioComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Provide either audio capture, playback or both", nullptr));
#endif // QT_CONFIG(tooltip)
        DitherTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "Dit&her:", nullptr));
        DitherComboBox->setItemText(0, QCoreApplication::translate("qjackctlSetupForm", "None", nullptr));
        DitherComboBox->setItemText(1, QCoreApplication::translate("qjackctlSetupForm", "Rectangular", nullptr));
        DitherComboBox->setItemText(2, QCoreApplication::translate("qjackctlSetupForm", "Shaped", nullptr));
        DitherComboBox->setItemText(3, QCoreApplication::translate("qjackctlSetupForm", "Triangular", nullptr));

#if QT_CONFIG(tooltip)
        DitherComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Set dither mode", nullptr));
#endif // QT_CONFIG(tooltip)
        OutDeviceTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "&Output Device:", nullptr));
        OutDeviceComboBox->setItemText(0, QCoreApplication::translate("qjackctlSetupForm", "(default)", nullptr));
        OutDeviceComboBox->setItemText(1, QCoreApplication::translate("qjackctlSetupForm", "hw:0", nullptr));
        OutDeviceComboBox->setItemText(2, QCoreApplication::translate("qjackctlSetupForm", "plughw:0", nullptr));
        OutDeviceComboBox->setItemText(3, QCoreApplication::translate("qjackctlSetupForm", "/dev/audio", nullptr));
        OutDeviceComboBox->setItemText(4, QCoreApplication::translate("qjackctlSetupForm", "/dev/dsp", nullptr));

#if QT_CONFIG(tooltip)
        OutDeviceComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Alternate output device for playback", nullptr));
#endif // QT_CONFIG(tooltip)
        InDeviceTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "&Input Device:", nullptr));
        InDeviceComboBox->setItemText(0, QCoreApplication::translate("qjackctlSetupForm", "(default)", nullptr));
        InDeviceComboBox->setItemText(1, QCoreApplication::translate("qjackctlSetupForm", "hw:0", nullptr));
        InDeviceComboBox->setItemText(2, QCoreApplication::translate("qjackctlSetupForm", "plughw:0", nullptr));
        InDeviceComboBox->setItemText(3, QCoreApplication::translate("qjackctlSetupForm", "/dev/audio", nullptr));
        InDeviceComboBox->setItemText(4, QCoreApplication::translate("qjackctlSetupForm", "/dev/dsp", nullptr));

#if QT_CONFIG(tooltip)
        InDeviceComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Alternate input device for capture", nullptr));
#endif // QT_CONFIG(tooltip)
        InOutChannelsTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "&Channels I/O:", nullptr));
#if QT_CONFIG(tooltip)
        InChannelsSpinBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Maximum input audio hardware channels to allocate", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        OutChannelsSpinBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Maximum output audio hardware channels to allocate", nullptr));
#endif // QT_CONFIG(tooltip)
        InOutLatencyTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "&Latency I/O:", nullptr));
#if QT_CONFIG(tooltip)
        InLatencySpinBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "External input latency (frames)", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        OutLatencySpinBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "External output latency (frames)", nullptr));
#endif // QT_CONFIG(tooltip)
        SelfConnectModeTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "S&elf connect mode:", nullptr));
#if QT_CONFIG(tooltip)
        SelfConnectModeComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to restrict client self-connections", nullptr));
#endif // QT_CONFIG(tooltip)
        ServerSuffixTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "Server Suffi&x:", nullptr));
#if QT_CONFIG(tooltip)
        ServerSuffixComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Extra driver options (command line suffix)", nullptr));
#endif // QT_CONFIG(tooltip)
        StartDelayTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "Start De&lay:", nullptr));
#if QT_CONFIG(tooltip)
        StartDelaySpinBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Time in seconds that client is delayed after server startup", nullptr));
#endif // QT_CONFIG(tooltip)
        StartDelaySpinBox->setSuffix(QCoreApplication::translate("qjackctlSetupForm", " secs", nullptr));
        SettingsTabWidget->setTabText(SettingsTabWidget->indexOf(AdvancedTab), QCoreApplication::translate("qjackctlSetupForm", "Advanced", nullptr));
        SetupTabWidget->setTabText(SetupTabWidget->indexOf(SettingsTabPage), QCoreApplication::translate("qjackctlSetupForm", "Settings", nullptr));
        ScriptingGroupBox->setTitle(QCoreApplication::translate("qjackctlSetupForm", "Scripting", nullptr));
#if QT_CONFIG(tooltip)
        StartupScriptCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to execute a custom shell script before starting up the JACK audio server.", nullptr));
#endif // QT_CONFIG(tooltip)
        StartupScriptCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "Execute script on Start&up:", nullptr));
#if QT_CONFIG(tooltip)
        PostStartupScriptCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to execute a custom shell script after starting up the JACK audio server.", nullptr));
#endif // QT_CONFIG(tooltip)
        PostStartupScriptCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "Execute script after &Startup:", nullptr));
#if QT_CONFIG(tooltip)
        ShutdownScriptCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to execute a custom shell script before shuting down the JACK audio server.", nullptr));
#endif // QT_CONFIG(tooltip)
        ShutdownScriptCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "Execute script on Shut&down:", nullptr));
#if QT_CONFIG(tooltip)
        StartupScriptShellComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Command line to be executed before starting up the JACK audio server", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        StartupScriptSymbolToolButton->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Scripting argument meta-symbols", nullptr));
#endif // QT_CONFIG(tooltip)
        StartupScriptSymbolToolButton->setText(QCoreApplication::translate("qjackctlSetupForm", ">", nullptr));
#if QT_CONFIG(tooltip)
        StartupScriptBrowseToolButton->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Browse for script to be executed before starting up the JACK audio server", nullptr));
#endif // QT_CONFIG(tooltip)
        StartupScriptBrowseToolButton->setText(QCoreApplication::translate("qjackctlSetupForm", "...", nullptr));
#if QT_CONFIG(tooltip)
        PostStartupScriptShellComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Command line to be executed after starting up the JACK audio server", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        PostStartupScriptSymbolToolButton->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Scripting argument meta-symbols", nullptr));
#endif // QT_CONFIG(tooltip)
        PostStartupScriptSymbolToolButton->setText(QCoreApplication::translate("qjackctlSetupForm", ">", nullptr));
#if QT_CONFIG(tooltip)
        PostStartupScriptBrowseToolButton->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Browse for script to be executed after starting up the JACK audio server", nullptr));
#endif // QT_CONFIG(tooltip)
        PostStartupScriptBrowseToolButton->setText(QCoreApplication::translate("qjackctlSetupForm", "...", nullptr));
#if QT_CONFIG(tooltip)
        ShutdownScriptSymbolToolButton->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Scripting argument meta-symbols", nullptr));
#endif // QT_CONFIG(tooltip)
        ShutdownScriptSymbolToolButton->setText(QCoreApplication::translate("qjackctlSetupForm", ">", nullptr));
#if QT_CONFIG(tooltip)
        ShutdownScriptBrowseToolButton->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Browse for script to be executed before shutting down the JACK audio server", nullptr));
#endif // QT_CONFIG(tooltip)
        ShutdownScriptBrowseToolButton->setText(QCoreApplication::translate("qjackctlSetupForm", "...", nullptr));
#if QT_CONFIG(tooltip)
        ShutdownScriptShellComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Command line to be executed before shutting down the JACK audio server", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        PostShutdownScriptCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to execute a custom shell script after shuting down the JACK audio server.", nullptr));
#endif // QT_CONFIG(tooltip)
        PostShutdownScriptCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "Execute script after Shu&tdown:", nullptr));
#if QT_CONFIG(tooltip)
        PostShutdownScriptSymbolToolButton->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Scripting argument meta-symbols", nullptr));
#endif // QT_CONFIG(tooltip)
        PostShutdownScriptSymbolToolButton->setText(QCoreApplication::translate("qjackctlSetupForm", ">", nullptr));
#if QT_CONFIG(tooltip)
        PostShutdownScriptBrowseToolButton->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Browse for script to be executed after shutting down the JACK audio server", nullptr));
#endif // QT_CONFIG(tooltip)
        PostShutdownScriptBrowseToolButton->setText(QCoreApplication::translate("qjackctlSetupForm", "...", nullptr));
#if QT_CONFIG(tooltip)
        PostShutdownScriptShellComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Command line to be executed after shutting down the JACK audio server", nullptr));
#endif // QT_CONFIG(tooltip)
        StatisticsGroupBox->setTitle(QCoreApplication::translate("qjackctlSetupForm", "Statistics", nullptr));
#if QT_CONFIG(tooltip)
        StdoutCaptureCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to capture standard output (stdout/stderr) into messages window", nullptr));
#endif // QT_CONFIG(tooltip)
        StdoutCaptureCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "&Capture standard output", nullptr));
        XrunRegexTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "&XRUN detection regex:", nullptr));
        XrunRegexComboBox->setItemText(0, QCoreApplication::translate("qjackctlSetupForm", "xrun of at least ([0-9|\\.]+) msecs", nullptr));

#if QT_CONFIG(tooltip)
        XrunRegexComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Regular expression used to detect XRUNs on server output messages", nullptr));
#endif // QT_CONFIG(tooltip)
        ConnectionsGroupBox->setTitle(QCoreApplication::translate("qjackctlSetupForm", "Connections", nullptr));
#if QT_CONFIG(tooltip)
        ActivePatchbayCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to activate a patchbay definition for connection persistence profile.", nullptr));
#endif // QT_CONFIG(tooltip)
        ActivePatchbayCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "Activate &Patchbay persistence:", nullptr));
#if QT_CONFIG(tooltip)
        ActivePatchbayPathComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Patchbay definition file to be activated as connection persistence profile", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        ActivePatchbayPathToolButton->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Browse for a patchbay definition file to be activated", nullptr));
#endif // QT_CONFIG(tooltip)
        ActivePatchbayPathToolButton->setText(QCoreApplication::translate("qjackctlSetupForm", "...", nullptr));
#if QT_CONFIG(tooltip)
        ActivePatchbayResetCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to reset all connections when a patchbay definition is activated.", nullptr));
#endif // QT_CONFIG(tooltip)
        ActivePatchbayResetCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "&Reset all connections on patchbay activation", nullptr));
#if QT_CONFIG(tooltip)
        QueryDisconnectCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to issue a warning on active patchbay port disconnections.", nullptr));
#endif // QT_CONFIG(tooltip)
        QueryDisconnectCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "&Warn on active patchbay disconnections", nullptr));
        LoggingGroupBox->setTitle(QCoreApplication::translate("qjackctlSetupForm", "Logging", nullptr));
#if QT_CONFIG(tooltip)
        MessagesLogCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to activate a messages logging to file.", nullptr));
#endif // QT_CONFIG(tooltip)
        MessagesLogCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "&Messages log file:", nullptr));
#if QT_CONFIG(tooltip)
        MessagesLogPathComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Messages log file", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        MessagesLogPathToolButton->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Browse for the messages log file location", nullptr));
#endif // QT_CONFIG(tooltip)
        MessagesLogPathToolButton->setText(QCoreApplication::translate("qjackctlSetupForm", "...", nullptr));
        SetupTabWidget->setTabText(SetupTabWidget->indexOf(OptionsTabPage), QCoreApplication::translate("qjackctlSetupForm", "Options", nullptr));
        TimeDisplayGroupBox->setTitle(QCoreApplication::translate("qjackctlSetupForm", "Time Display", nullptr));
        TransportTimeRadioButton->setText(QCoreApplication::translate("qjackctlSetupForm", "Transport &Time Code", nullptr));
        TransportBBTRadioButton->setText(QCoreApplication::translate("qjackctlSetupForm", "Transport &BBT (bar:beat.ticks)", nullptr));
        ElapsedResetRadioButton->setText(QCoreApplication::translate("qjackctlSetupForm", "Elapsed time since last &Reset", nullptr));
        ElapsedXrunRadioButton->setText(QCoreApplication::translate("qjackctlSetupForm", "Elapsed time since last &XRUN", nullptr));
#if QT_CONFIG(tooltip)
        DisplayFont2TextLabel->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Sample front panel normal display font", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        DisplayFont1TextLabel->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Sample big time display font", nullptr));
#endif // QT_CONFIG(tooltip)
        DisplayFont1Label->setText(QCoreApplication::translate("qjackctlSetupForm", "Big Time display:", nullptr));
#if QT_CONFIG(tooltip)
        DisplayFont2PushButton->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Select font for front panel normal display", nullptr));
#endif // QT_CONFIG(tooltip)
        DisplayFont2PushButton->setText(QCoreApplication::translate("qjackctlSetupForm", "&Font...", nullptr));
#if QT_CONFIG(tooltip)
        DisplayFont1PushButton->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Select font for big time display", nullptr));
#endif // QT_CONFIG(tooltip)
        DisplayFont1PushButton->setText(QCoreApplication::translate("qjackctlSetupForm", "&Font...", nullptr));
        DisplayFont2Label->setText(QCoreApplication::translate("qjackctlSetupForm", "Normal display:", nullptr));
#if QT_CONFIG(tooltip)
        DisplayBlinkCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to enable blinking (flashing) of the server mode (RT) indicator", nullptr));
#endif // QT_CONFIG(tooltip)
        DisplayBlinkCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "Blin&k server mode indicator", nullptr));
        DisplayCustomGroupBox->setTitle(QCoreApplication::translate("qjackctlSetupForm", "Custom", nullptr));
        CustomColorThemeTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "&Color palette theme:", nullptr));
        CustomColorThemeComboBox->setItemText(0, QCoreApplication::translate("qjackctlSetupForm", "(default)", nullptr));
        CustomColorThemeComboBox->setItemText(1, QCoreApplication::translate("qjackctlSetupForm", "Wonton Soup", nullptr));
        CustomColorThemeComboBox->setItemText(2, QCoreApplication::translate("qjackctlSetupForm", "KXStudio", nullptr));

#if QT_CONFIG(tooltip)
        CustomColorThemeComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Custom color palette theme", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        CustomColorThemeToolButton->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Manage custom color palette themes", nullptr));
#endif // QT_CONFIG(tooltip)
        CustomColorThemeToolButton->setText(QCoreApplication::translate("qjackctlSetupForm", "...", nullptr));
        CustomStyleThemeTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "&Widget style theme:", nullptr));
        CustomStyleThemeComboBox->setItemText(0, QCoreApplication::translate("qjackctlSetupForm", "(default)", nullptr));

#if QT_CONFIG(tooltip)
        CustomStyleThemeComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Custom widget style theme", nullptr));
#endif // QT_CONFIG(tooltip)
        MessagesWindowGroupBox->setTitle(QCoreApplication::translate("qjackctlSetupForm", "Messages Window", nullptr));
#if QT_CONFIG(tooltip)
        MessagesFontTextLabel->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Sample messages text font display", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        MessagesFontPushButton->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Select font for the messages text display", nullptr));
#endif // QT_CONFIG(tooltip)
        MessagesFontPushButton->setText(QCoreApplication::translate("qjackctlSetupForm", "&Font...", nullptr));
#if QT_CONFIG(tooltip)
        MessagesLimitCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to keep a maximum number of lines in the messages window", nullptr));
#endif // QT_CONFIG(tooltip)
        MessagesLimitCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "&Messages limit:", nullptr));
        MessagesLimitLinesComboBox->setItemText(0, QCoreApplication::translate("qjackctlSetupForm", "100", nullptr));
        MessagesLimitLinesComboBox->setItemText(1, QCoreApplication::translate("qjackctlSetupForm", "250", nullptr));
        MessagesLimitLinesComboBox->setItemText(2, QCoreApplication::translate("qjackctlSetupForm", "500", nullptr));
        MessagesLimitLinesComboBox->setItemText(3, QCoreApplication::translate("qjackctlSetupForm", "1000", nullptr));
        MessagesLimitLinesComboBox->setItemText(4, QCoreApplication::translate("qjackctlSetupForm", "2500", nullptr));
        MessagesLimitLinesComboBox->setItemText(5, QCoreApplication::translate("qjackctlSetupForm", "5000", nullptr));

#if QT_CONFIG(tooltip)
        MessagesLimitLinesComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "The maximum number of message lines to keep in view", nullptr));
#endif // QT_CONFIG(tooltip)
        ConnectionsWindowGroupBox->setTitle(QCoreApplication::translate("qjackctlSetupForm", "Connections Window", nullptr));
#if QT_CONFIG(tooltip)
        ConnectionsFontTextLabel->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Sample connections view font", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        ConnectionsFontPushButton->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Select font for the connections view", nullptr));
#endif // QT_CONFIG(tooltip)
        ConnectionsFontPushButton->setText(QCoreApplication::translate("qjackctlSetupForm", "&Font...", nullptr));
        ConnectionsIconSizeTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "&Icon size:", nullptr));
        ConnectionsIconSizeComboBox->setItemText(0, QCoreApplication::translate("qjackctlSetupForm", "16 x 16", nullptr));
        ConnectionsIconSizeComboBox->setItemText(1, QCoreApplication::translate("qjackctlSetupForm", "32 x 32", nullptr));
        ConnectionsIconSizeComboBox->setItemText(2, QCoreApplication::translate("qjackctlSetupForm", "64 x 64", nullptr));

#if QT_CONFIG(tooltip)
        ConnectionsIconSizeComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "The icon size for each item of the connections view", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        AliasesEnabledCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to enable client/port name aliases on the connections window", nullptr));
#endif // QT_CONFIG(tooltip)
        AliasesEnabledCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "E&nable client/port aliases", nullptr));
        JackClientPortAliasTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "&JACK client/port aliases:", nullptr));
        JackClientPortAliasComboBox->setItemText(0, QCoreApplication::translate("qjackctlSetupForm", "Default", nullptr));
        JackClientPortAliasComboBox->setItemText(1, QCoreApplication::translate("qjackctlSetupForm", "First", nullptr));
        JackClientPortAliasComboBox->setItemText(2, QCoreApplication::translate("qjackctlSetupForm", "Second", nullptr));

#if QT_CONFIG(tooltip)
        JackClientPortAliasComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "JACK client/port aliases display mode", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        AliasesEditingCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to enable in-place client/port name editing (rename)", nullptr));
#endif // QT_CONFIG(tooltip)
        AliasesEditingCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "Ena&ble client/port aliases editing (rename)", nullptr));
#if QT_CONFIG(tooltip)
        JackClientPortMetadataCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "JACK client/port pretty-name (metadata) display mode", nullptr));
#endif // QT_CONFIG(tooltip)
        JackClientPortMetadataCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "Enable JA&CK client/port pretty-names (metadata)", nullptr));
        SetupTabWidget->setTabText(SetupTabWidget->indexOf(DisplayTabPage), QCoreApplication::translate("qjackctlSetupForm", "Display", nullptr));
        OtherGroupBox->setTitle(QCoreApplication::translate("qjackctlSetupForm", "Other", nullptr));
#if QT_CONFIG(tooltip)
        StartJackCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to start JACK audio server immediately on application startup", nullptr));
#endif // QT_CONFIG(tooltip)
        StartJackCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "&Start JACK audio server on application startup", nullptr));
#if QT_CONFIG(tooltip)
        QueryCloseCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to ask for confirmation on application exit", nullptr));
#endif // QT_CONFIG(tooltip)
        QueryCloseCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "&Confirm application close", nullptr));
#if QT_CONFIG(tooltip)
        QueryShutdownCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to ask for confirmation on JACK audio server shutdown and/or restart", nullptr));
#endif // QT_CONFIG(tooltip)
        QueryShutdownCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "Confirm server sh&utdown and/or restart", nullptr));
#if QT_CONFIG(tooltip)
        KeepOnTopCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to keep all child windows on top of the main window", nullptr));
#endif // QT_CONFIG(tooltip)
        KeepOnTopCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "&Keep child windows always on top", nullptr));
#if QT_CONFIG(tooltip)
        SystemTrayCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to enable the system tray icon", nullptr));
#endif // QT_CONFIG(tooltip)
        SystemTrayCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "&Enable system tray icon", nullptr));
#if QT_CONFIG(tooltip)
        SystemTrayQueryCloseCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to show system tray message on main window close", nullptr));
#endif // QT_CONFIG(tooltip)
        SystemTrayQueryCloseCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "Sho&w system tray message on close", nullptr));
#if QT_CONFIG(tooltip)
        StartMinimizedCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to start minimized to system tray", nullptr));
#endif // QT_CONFIG(tooltip)
        StartMinimizedCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "Start minimi&zed to system tray", nullptr));
#if QT_CONFIG(tooltip)
        ServerConfigCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to save the JACK server command-line configuration into a local file (auto-start)", nullptr));
#endif // QT_CONFIG(tooltip)
        ServerConfigCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "S&ave JACK audio server configuration to:", nullptr));
        ServerConfigNameComboBox->setItemText(0, QCoreApplication::translate("qjackctlSetupForm", ".jackdrc", nullptr));

#if QT_CONFIG(tooltip)
        ServerConfigNameComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "The server configuration local file name (auto-start)", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        AlsaSeqEnabledCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to enable ALSA Sequencer (MIDI) support on startup", nullptr));
#endif // QT_CONFIG(tooltip)
        AlsaSeqEnabledCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "E&nable ALSA Sequencer support", nullptr));
#if QT_CONFIG(tooltip)
        DBusEnabledCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to enable D-Bus interface", nullptr));
#endif // QT_CONFIG(tooltip)
        DBusEnabledCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "&Enable D-Bus interface", nullptr));
#if QT_CONFIG(tooltip)
        JackDBusEnabledCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to enable JACK D-Bus interface", nullptr));
#endif // QT_CONFIG(tooltip)
        JackDBusEnabledCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "&Enable JACK D-Bus interface", nullptr));
#if QT_CONFIG(tooltip)
        StopJackCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to stop JACK audio server on application exit", nullptr));
#endif // QT_CONFIG(tooltip)
        StopJackCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "S&top JACK audio server on application exit", nullptr));
#if QT_CONFIG(tooltip)
        SingletonCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to restrict to one single application instance (X11)", nullptr));
#endif // QT_CONFIG(tooltip)
        SingletonCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "Single application &instance", nullptr));
        ButtonsGroupBox->setTitle(QCoreApplication::translate("qjackctlSetupForm", "Buttons", nullptr));
#if QT_CONFIG(tooltip)
        LeftButtonsCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to hide the left button group on the main window", nullptr));
#endif // QT_CONFIG(tooltip)
        LeftButtonsCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "Hide main window &Left buttons", nullptr));
#if QT_CONFIG(tooltip)
        RightButtonsCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to hide the right button group on the main window", nullptr));
#endif // QT_CONFIG(tooltip)
        RightButtonsCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "Hide main window &Right buttons", nullptr));
#if QT_CONFIG(tooltip)
        TransportButtonsCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to hide the transport button group on the main window", nullptr));
#endif // QT_CONFIG(tooltip)
        TransportButtonsCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "Hide main window &Transport buttons", nullptr));
#if QT_CONFIG(tooltip)
        TextLabelsCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to hide the text labels on the main window buttons", nullptr));
#endif // QT_CONFIG(tooltip)
        TextLabelsCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "Hide main window &button text labels", nullptr));
#if QT_CONFIG(tooltip)
        GraphButtonCheckBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Whether to replace Connections with Graph button on the main window", nullptr));
#endif // QT_CONFIG(tooltip)
        GraphButtonCheckBox->setText(QCoreApplication::translate("qjackctlSetupForm", "Replace Connections with &Graph button", nullptr));
        DefaultsGroupBox->setTitle(QCoreApplication::translate("qjackctlSetupForm", "Defaults", nullptr));
        BaseFontSizeTextLabel->setText(QCoreApplication::translate("qjackctlSetupForm", "&Base font size:", nullptr));
        BaseFontSizeComboBox->setItemText(0, QCoreApplication::translate("qjackctlSetupForm", "(default)", nullptr));
        BaseFontSizeComboBox->setItemText(1, QCoreApplication::translate("qjackctlSetupForm", "6", nullptr));
        BaseFontSizeComboBox->setItemText(2, QCoreApplication::translate("qjackctlSetupForm", "7", nullptr));
        BaseFontSizeComboBox->setItemText(3, QCoreApplication::translate("qjackctlSetupForm", "8", nullptr));
        BaseFontSizeComboBox->setItemText(4, QCoreApplication::translate("qjackctlSetupForm", "9", nullptr));
        BaseFontSizeComboBox->setItemText(5, QCoreApplication::translate("qjackctlSetupForm", "10", nullptr));
        BaseFontSizeComboBox->setItemText(6, QCoreApplication::translate("qjackctlSetupForm", "11", nullptr));
        BaseFontSizeComboBox->setItemText(7, QCoreApplication::translate("qjackctlSetupForm", "12", nullptr));

#if QT_CONFIG(tooltip)
        BaseFontSizeComboBox->setToolTip(QCoreApplication::translate("qjackctlSetupForm", "Base application font size (pt.)", nullptr));
#endif // QT_CONFIG(tooltip)
        SetupTabWidget->setTabText(SetupTabWidget->indexOf(MiscTabPage), QCoreApplication::translate("qjackctlSetupForm", "Misc", nullptr));
    } // retranslateUi

};

namespace Ui {
    class qjackctlSetupForm: public Ui_qjackctlSetupForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QJACKCTLSETUPFORM_H
