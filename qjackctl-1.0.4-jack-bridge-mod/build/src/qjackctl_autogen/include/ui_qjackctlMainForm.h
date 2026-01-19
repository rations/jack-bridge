/*
JACK Audio Connection Kit - Qt GUI Interface.

   Copyright (C) 2003-2025, rncbc aka Rui Nuno Capela. All rights reserved.

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
** Form generated from reading UI file 'qjackctlMainForm.ui'
**
** Created by: Qt User Interface Compiler version 6.4.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QJACKCTLMAINFORM_H
#define UI_QJACKCTLMAINFORM_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_qjackctlMainForm
{
public:
    QGridLayout *gridLayout;
    QToolButton *StartToolButton;
    QToolButton *StopToolButton;
    QFrame *StatusDisplayFrame;
    QGridLayout *gridLayout1;
    QLabel *ServerStateTextLabel;
    QLabel *ServerModeTextLabel;
    QLabel *DspLoadTextLabel;
    QLabel *SampleRateTextLabel;
    QLabel *XrunCountTextLabel;
    QLabel *TimeDisplayTextLabel;
    QLabel *TransportStateTextLabel;
    QLabel *TransportBpmTextLabel;
    QLabel *TransportTimeTextLabel;
    QToolButton *QuitToolButton;
    QToolButton *SessionToolButton;
    QToolButton *MessagesStatusToolButton;
    QToolButton *SetupToolButton;
    QToolButton *GraphToolButton;
    QToolButton *ConnectionsToolButton;
    QToolButton *PatchbayToolButton;
    QToolButton *RewindToolButton;
    QToolButton *BackwardToolButton;
    QToolButton *PlayToolButton;
    QToolButton *PauseToolButton;
    QToolButton *ForwardToolButton;
    QToolButton *AboutToolButton;

    void setupUi(QWidget *qjackctlMainForm)
    {
        if (qjackctlMainForm->objectName().isEmpty())
            qjackctlMainForm->setObjectName("qjackctlMainForm");
        qjackctlMainForm->resize(300, 100);
        const QIcon icon = QIcon(QString::fromUtf8(":/images/qjackctl.svg"));
        qjackctlMainForm->setWindowIcon(icon);
        gridLayout = new QGridLayout(qjackctlMainForm);
        gridLayout->setSpacing(4);
        gridLayout->setContentsMargins(4, 4, 4, 4);
        gridLayout->setObjectName("gridLayout");
        StartToolButton = new QToolButton(qjackctlMainForm);
        StartToolButton->setObjectName("StartToolButton");
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(StartToolButton->sizePolicy().hasHeightForWidth());
        StartToolButton->setSizePolicy(sizePolicy);
        StartToolButton->setMinimumSize(QSize(28, 28));
        StartToolButton->setFocusPolicy(Qt::TabFocus);
        const QIcon icon1 = QIcon(QString::fromUtf8(":/images/start1.png"));
        StartToolButton->setIcon(icon1);
        StartToolButton->setCheckable(false);
        StartToolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        gridLayout->addWidget(StartToolButton, 0, 0, 1, 1);

        StopToolButton = new QToolButton(qjackctlMainForm);
        StopToolButton->setObjectName("StopToolButton");
        sizePolicy.setHeightForWidth(StopToolButton->sizePolicy().hasHeightForWidth());
        StopToolButton->setSizePolicy(sizePolicy);
        StopToolButton->setMinimumSize(QSize(28, 28));
        StopToolButton->setFocusPolicy(Qt::TabFocus);
        const QIcon icon2 = QIcon(QString::fromUtf8(":/images/stop1.png"));
        StopToolButton->setIcon(icon2);
        StopToolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        gridLayout->addWidget(StopToolButton, 0, 1, 1, 1);

        StatusDisplayFrame = new QFrame(qjackctlMainForm);
        StatusDisplayFrame->setObjectName("StatusDisplayFrame");
        sizePolicy.setHeightForWidth(StatusDisplayFrame->sizePolicy().hasHeightForWidth());
        StatusDisplayFrame->setSizePolicy(sizePolicy);
        StatusDisplayFrame->setAutoFillBackground(true);
        StatusDisplayFrame->setFrameShape(QFrame::Panel);
        StatusDisplayFrame->setFrameShadow(QFrame::Sunken);
        gridLayout1 = new QGridLayout(StatusDisplayFrame);
        gridLayout1->setSpacing(0);
        gridLayout1->setContentsMargins(2, 2, 2, 2);
        gridLayout1->setObjectName("gridLayout1");
        ServerStateTextLabel = new QLabel(StatusDisplayFrame);
        ServerStateTextLabel->setObjectName("ServerStateTextLabel");
        ServerStateTextLabel->setMinimumSize(QSize(50, 0));
        QFont font;
        font.setPointSize(8);
        ServerStateTextLabel->setFont(font);
        ServerStateTextLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        ServerStateTextLabel->setWordWrap(false);
        ServerStateTextLabel->setIndent(2);

        gridLayout1->addWidget(ServerStateTextLabel, 0, 0, 1, 2);

        ServerModeTextLabel = new QLabel(StatusDisplayFrame);
        ServerModeTextLabel->setObjectName("ServerModeTextLabel");
        ServerModeTextLabel->setMinimumSize(QSize(20, 0));
        ServerModeTextLabel->setFont(font);
        ServerModeTextLabel->setAlignment(Qt::AlignCenter);
        ServerModeTextLabel->setWordWrap(false);

        gridLayout1->addWidget(ServerModeTextLabel, 0, 2, 1, 1);

        DspLoadTextLabel = new QLabel(StatusDisplayFrame);
        DspLoadTextLabel->setObjectName("DspLoadTextLabel");
        DspLoadTextLabel->setMinimumSize(QSize(40, 0));
        DspLoadTextLabel->setFont(font);
        DspLoadTextLabel->setAlignment(Qt::AlignCenter);
        DspLoadTextLabel->setWordWrap(false);

        gridLayout1->addWidget(DspLoadTextLabel, 0, 3, 1, 2);

        SampleRateTextLabel = new QLabel(StatusDisplayFrame);
        SampleRateTextLabel->setObjectName("SampleRateTextLabel");
        SampleRateTextLabel->setMinimumSize(QSize(50, 0));
        SampleRateTextLabel->setFont(font);
        SampleRateTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        SampleRateTextLabel->setWordWrap(false);

        gridLayout1->addWidget(SampleRateTextLabel, 0, 5, 1, 1);

        XrunCountTextLabel = new QLabel(StatusDisplayFrame);
        XrunCountTextLabel->setObjectName("XrunCountTextLabel");
        XrunCountTextLabel->setMinimumSize(QSize(30, 0));
        XrunCountTextLabel->setFont(font);
        XrunCountTextLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        XrunCountTextLabel->setWordWrap(false);
        XrunCountTextLabel->setIndent(2);

        gridLayout1->addWidget(XrunCountTextLabel, 1, 0, 1, 1);

        TimeDisplayTextLabel = new QLabel(StatusDisplayFrame);
        TimeDisplayTextLabel->setObjectName("TimeDisplayTextLabel");
        TimeDisplayTextLabel->setMinimumSize(QSize(130, 0));
        QFont font1;
        font1.setPointSize(14);
        font1.setBold(true);
        TimeDisplayTextLabel->setFont(font1);
        TimeDisplayTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        TimeDisplayTextLabel->setWordWrap(false);

        gridLayout1->addWidget(TimeDisplayTextLabel, 1, 1, 1, 5);

        TransportStateTextLabel = new QLabel(StatusDisplayFrame);
        TransportStateTextLabel->setObjectName("TransportStateTextLabel");
        TransportStateTextLabel->setMinimumSize(QSize(50, 0));
        TransportStateTextLabel->setFont(font);
        TransportStateTextLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        TransportStateTextLabel->setWordWrap(false);
        TransportStateTextLabel->setIndent(2);

        gridLayout1->addWidget(TransportStateTextLabel, 2, 0, 1, 2);

        TransportBpmTextLabel = new QLabel(StatusDisplayFrame);
        TransportBpmTextLabel->setObjectName("TransportBpmTextLabel");
        TransportBpmTextLabel->setMinimumSize(QSize(30, 0));
        TransportBpmTextLabel->setFont(font);
        TransportBpmTextLabel->setAlignment(Qt::AlignCenter);
        TransportBpmTextLabel->setWordWrap(false);

        gridLayout1->addWidget(TransportBpmTextLabel, 2, 2, 1, 2);

        TransportTimeTextLabel = new QLabel(StatusDisplayFrame);
        TransportTimeTextLabel->setObjectName("TransportTimeTextLabel");
        TransportTimeTextLabel->setMinimumSize(QSize(80, 0));
        QFont font2;
        font2.setPointSize(8);
        font2.setBold(true);
        TransportTimeTextLabel->setFont(font2);
        TransportTimeTextLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        TransportTimeTextLabel->setWordWrap(false);

        gridLayout1->addWidget(TransportTimeTextLabel, 2, 4, 1, 2);


        gridLayout->addWidget(StatusDisplayFrame, 0, 2, 2, 5);

        QuitToolButton = new QToolButton(qjackctlMainForm);
        QuitToolButton->setObjectName("QuitToolButton");
        sizePolicy.setHeightForWidth(QuitToolButton->sizePolicy().hasHeightForWidth());
        QuitToolButton->setSizePolicy(sizePolicy);
        QuitToolButton->setMinimumSize(QSize(28, 28));
        QuitToolButton->setFocusPolicy(Qt::TabFocus);
        const QIcon icon3 = QIcon(QString::fromUtf8(":/images/quit1.png"));
        QuitToolButton->setIcon(icon3);
        QuitToolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        gridLayout->addWidget(QuitToolButton, 0, 7, 1, 1);

        SessionToolButton = new QToolButton(qjackctlMainForm);
        SessionToolButton->setObjectName("SessionToolButton");
        sizePolicy.setHeightForWidth(SessionToolButton->sizePolicy().hasHeightForWidth());
        SessionToolButton->setSizePolicy(sizePolicy);
        SessionToolButton->setMinimumSize(QSize(28, 28));
        SessionToolButton->setFocusPolicy(Qt::TabFocus);
        const QIcon icon4 = QIcon(QString::fromUtf8(":/images/session1.png"));
        SessionToolButton->setIcon(icon4);
        SessionToolButton->setCheckable(true);
        SessionToolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        gridLayout->addWidget(SessionToolButton, 1, 1, 1, 1);

        MessagesStatusToolButton = new QToolButton(qjackctlMainForm);
        MessagesStatusToolButton->setObjectName("MessagesStatusToolButton");
        sizePolicy.setHeightForWidth(MessagesStatusToolButton->sizePolicy().hasHeightForWidth());
        MessagesStatusToolButton->setSizePolicy(sizePolicy);
        MessagesStatusToolButton->setMinimumSize(QSize(28, 28));
        MessagesStatusToolButton->setFocusPolicy(Qt::TabFocus);
        const QIcon icon5 = QIcon(QString::fromUtf8(":/images/messagesstatus1.png"));
        MessagesStatusToolButton->setIcon(icon5);
        MessagesStatusToolButton->setCheckable(true);
        MessagesStatusToolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        gridLayout->addWidget(MessagesStatusToolButton, 1, 0, 1, 1);

        SetupToolButton = new QToolButton(qjackctlMainForm);
        SetupToolButton->setObjectName("SetupToolButton");
        sizePolicy.setHeightForWidth(SetupToolButton->sizePolicy().hasHeightForWidth());
        SetupToolButton->setSizePolicy(sizePolicy);
        SetupToolButton->setMinimumSize(QSize(28, 28));
        SetupToolButton->setFocusPolicy(Qt::TabFocus);
        const QIcon icon6 = QIcon(QString::fromUtf8(":/images/setup1.png"));
        SetupToolButton->setIcon(icon6);
        SetupToolButton->setCheckable(true);
        SetupToolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        gridLayout->addWidget(SetupToolButton, 1, 7, 1, 1);

        GraphToolButton = new QToolButton(qjackctlMainForm);
        GraphToolButton->setObjectName("GraphToolButton");
        sizePolicy.setHeightForWidth(GraphToolButton->sizePolicy().hasHeightForWidth());
        GraphToolButton->setSizePolicy(sizePolicy);
        GraphToolButton->setMinimumSize(QSize(28, 28));
        GraphToolButton->setFocusPolicy(Qt::TabFocus);
        const QIcon icon7 = QIcon(QString::fromUtf8(":/images/graph1.png"));
        GraphToolButton->setIcon(icon7);
        GraphToolButton->setCheckable(true);
        GraphToolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        gridLayout->addWidget(GraphToolButton, 2, 0, 1, 1);

        ConnectionsToolButton = new QToolButton(qjackctlMainForm);
        ConnectionsToolButton->setObjectName("ConnectionsToolButton");
        sizePolicy.setHeightForWidth(ConnectionsToolButton->sizePolicy().hasHeightForWidth());
        ConnectionsToolButton->setSizePolicy(sizePolicy);
        ConnectionsToolButton->setMinimumSize(QSize(28, 28));
        ConnectionsToolButton->setFocusPolicy(Qt::TabFocus);
        const QIcon icon8 = QIcon(QString::fromUtf8(":/images/connections1.png"));
        ConnectionsToolButton->setIcon(icon8);
        ConnectionsToolButton->setCheckable(true);
        ConnectionsToolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        gridLayout->addWidget(ConnectionsToolButton, 2, 0, 1, 1);

        PatchbayToolButton = new QToolButton(qjackctlMainForm);
        PatchbayToolButton->setObjectName("PatchbayToolButton");
        sizePolicy.setHeightForWidth(PatchbayToolButton->sizePolicy().hasHeightForWidth());
        PatchbayToolButton->setSizePolicy(sizePolicy);
        PatchbayToolButton->setMinimumSize(QSize(28, 28));
        PatchbayToolButton->setFocusPolicy(Qt::TabFocus);
        const QIcon icon9 = QIcon(QString::fromUtf8(":/images/patchbay1.png"));
        PatchbayToolButton->setIcon(icon9);
        PatchbayToolButton->setCheckable(true);
        PatchbayToolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        gridLayout->addWidget(PatchbayToolButton, 2, 1, 1, 1);

        RewindToolButton = new QToolButton(qjackctlMainForm);
        RewindToolButton->setObjectName("RewindToolButton");
        sizePolicy.setHeightForWidth(RewindToolButton->sizePolicy().hasHeightForWidth());
        RewindToolButton->setSizePolicy(sizePolicy);
        RewindToolButton->setMinimumSize(QSize(28, 28));
        RewindToolButton->setFocusPolicy(Qt::TabFocus);
        const QIcon icon10 = QIcon(QString::fromUtf8(":/images/rewind1.png"));
        RewindToolButton->setIcon(icon10);

        gridLayout->addWidget(RewindToolButton, 2, 2, 1, 1);

        BackwardToolButton = new QToolButton(qjackctlMainForm);
        BackwardToolButton->setObjectName("BackwardToolButton");
        sizePolicy.setHeightForWidth(BackwardToolButton->sizePolicy().hasHeightForWidth());
        BackwardToolButton->setSizePolicy(sizePolicy);
        BackwardToolButton->setMinimumSize(QSize(28, 28));
        BackwardToolButton->setFocusPolicy(Qt::TabFocus);
        const QIcon icon11 = QIcon(QString::fromUtf8(":/images/backward1.png"));
        BackwardToolButton->setIcon(icon11);
        BackwardToolButton->setAutoRepeat(true);

        gridLayout->addWidget(BackwardToolButton, 2, 3, 1, 1);

        PlayToolButton = new QToolButton(qjackctlMainForm);
        PlayToolButton->setObjectName("PlayToolButton");
        sizePolicy.setHeightForWidth(PlayToolButton->sizePolicy().hasHeightForWidth());
        PlayToolButton->setSizePolicy(sizePolicy);
        PlayToolButton->setMinimumSize(QSize(28, 28));
        PlayToolButton->setFocusPolicy(Qt::TabFocus);
        const QIcon icon12 = QIcon(QString::fromUtf8(":/images/play1.png"));
        PlayToolButton->setIcon(icon12);
        PlayToolButton->setCheckable(true);

        gridLayout->addWidget(PlayToolButton, 2, 4, 1, 1);

        PauseToolButton = new QToolButton(qjackctlMainForm);
        PauseToolButton->setObjectName("PauseToolButton");
        sizePolicy.setHeightForWidth(PauseToolButton->sizePolicy().hasHeightForWidth());
        PauseToolButton->setSizePolicy(sizePolicy);
        PauseToolButton->setMinimumSize(QSize(28, 28));
        PauseToolButton->setFocusPolicy(Qt::TabFocus);
        const QIcon icon13 = QIcon(QString::fromUtf8(":/images/pause1.png"));
        PauseToolButton->setIcon(icon13);

        gridLayout->addWidget(PauseToolButton, 2, 5, 1, 1);

        ForwardToolButton = new QToolButton(qjackctlMainForm);
        ForwardToolButton->setObjectName("ForwardToolButton");
        sizePolicy.setHeightForWidth(ForwardToolButton->sizePolicy().hasHeightForWidth());
        ForwardToolButton->setSizePolicy(sizePolicy);
        ForwardToolButton->setMinimumSize(QSize(28, 28));
        ForwardToolButton->setFocusPolicy(Qt::TabFocus);
        const QIcon icon14 = QIcon(QString::fromUtf8(":/images/forward1.png"));
        ForwardToolButton->setIcon(icon14);
        ForwardToolButton->setAutoRepeat(true);

        gridLayout->addWidget(ForwardToolButton, 2, 6, 1, 1);

        AboutToolButton = new QToolButton(qjackctlMainForm);
        AboutToolButton->setObjectName("AboutToolButton");
        sizePolicy.setHeightForWidth(AboutToolButton->sizePolicy().hasHeightForWidth());
        AboutToolButton->setSizePolicy(sizePolicy);
        AboutToolButton->setMinimumSize(QSize(28, 28));
        AboutToolButton->setFocusPolicy(Qt::TabFocus);
        const QIcon icon15 = QIcon(QString::fromUtf8(":/images/about1.png"));
        AboutToolButton->setIcon(icon15);
        AboutToolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        gridLayout->addWidget(AboutToolButton, 2, 7, 1, 1);

        QWidget::setTabOrder(StartToolButton, StopToolButton);
        QWidget::setTabOrder(StopToolButton, QuitToolButton);
        QWidget::setTabOrder(QuitToolButton, MessagesStatusToolButton);
        QWidget::setTabOrder(MessagesStatusToolButton, SessionToolButton);
        QWidget::setTabOrder(SessionToolButton, SetupToolButton);
        QWidget::setTabOrder(SetupToolButton, GraphToolButton);
        QWidget::setTabOrder(GraphToolButton, ConnectionsToolButton);
        QWidget::setTabOrder(ConnectionsToolButton, PatchbayToolButton);
        QWidget::setTabOrder(PatchbayToolButton, RewindToolButton);
        QWidget::setTabOrder(RewindToolButton, BackwardToolButton);
        QWidget::setTabOrder(BackwardToolButton, PlayToolButton);
        QWidget::setTabOrder(PlayToolButton, PauseToolButton);
        QWidget::setTabOrder(PauseToolButton, ForwardToolButton);
        QWidget::setTabOrder(ForwardToolButton, AboutToolButton);

        retranslateUi(qjackctlMainForm);

        QMetaObject::connectSlotsByName(qjackctlMainForm);
    } // setupUi

    void retranslateUi(QWidget *qjackctlMainForm)
    {
#if QT_CONFIG(tooltip)
        StartToolButton->setToolTip(QCoreApplication::translate("qjackctlMainForm", "Start the JACK server", nullptr));
#endif // QT_CONFIG(tooltip)
        StartToolButton->setText(QCoreApplication::translate("qjackctlMainForm", "&Start", nullptr));
#if QT_CONFIG(tooltip)
        StopToolButton->setToolTip(QCoreApplication::translate("qjackctlMainForm", "Stop the JACK server", nullptr));
#endif // QT_CONFIG(tooltip)
        StopToolButton->setText(QCoreApplication::translate("qjackctlMainForm", "S&top", nullptr));
#if QT_CONFIG(tooltip)
        ServerStateTextLabel->setToolTip(QCoreApplication::translate("qjackctlMainForm", "JACK server state", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        ServerModeTextLabel->setToolTip(QCoreApplication::translate("qjackctlMainForm", "JACK server mode", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        DspLoadTextLabel->setToolTip(QCoreApplication::translate("qjackctlMainForm", "DSP Load", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        SampleRateTextLabel->setToolTip(QCoreApplication::translate("qjackctlMainForm", "Sample rate", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        XrunCountTextLabel->setToolTip(QCoreApplication::translate("qjackctlMainForm", "XRUN Count (notifications)", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        TimeDisplayTextLabel->setToolTip(QCoreApplication::translate("qjackctlMainForm", "Time display", nullptr));
#endif // QT_CONFIG(tooltip)
        TimeDisplayTextLabel->setText(QString());
#if QT_CONFIG(tooltip)
        TransportStateTextLabel->setToolTip(QCoreApplication::translate("qjackctlMainForm", "Transport state", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        TransportBpmTextLabel->setToolTip(QCoreApplication::translate("qjackctlMainForm", "Transport BPM", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        TransportTimeTextLabel->setToolTip(QCoreApplication::translate("qjackctlMainForm", "Transport time", nullptr));
#endif // QT_CONFIG(tooltip)
        TransportTimeTextLabel->setText(QString());
#if QT_CONFIG(tooltip)
        QuitToolButton->setToolTip(QCoreApplication::translate("qjackctlMainForm", "Quit processing and exit", nullptr));
#endif // QT_CONFIG(tooltip)
        QuitToolButton->setText(QCoreApplication::translate("qjackctlMainForm", "&Quit", nullptr));
#if QT_CONFIG(tooltip)
        SessionToolButton->setToolTip(QCoreApplication::translate("qjackctlMainForm", "Show/hide the session management window", nullptr));
#endif // QT_CONFIG(tooltip)
        SessionToolButton->setText(QCoreApplication::translate("qjackctlMainForm", "S&ession", nullptr));
#if QT_CONFIG(tooltip)
        MessagesStatusToolButton->setToolTip(QCoreApplication::translate("qjackctlMainForm", "Show/hide the messages log/status window", nullptr));
#endif // QT_CONFIG(tooltip)
        MessagesStatusToolButton->setText(QCoreApplication::translate("qjackctlMainForm", "&Messages", nullptr));
#if QT_CONFIG(tooltip)
        SetupToolButton->setToolTip(QCoreApplication::translate("qjackctlMainForm", "Show settings and options dialog", nullptr));
#endif // QT_CONFIG(tooltip)
        SetupToolButton->setText(QCoreApplication::translate("qjackctlMainForm", "Set&up...", nullptr));
#if QT_CONFIG(tooltip)
        GraphToolButton->setToolTip(QCoreApplication::translate("qjackctlMainForm", "Show/hide the graph window", nullptr));
#endif // QT_CONFIG(tooltip)
        GraphToolButton->setText(QCoreApplication::translate("qjackctlMainForm", "&Graph", nullptr));
#if QT_CONFIG(tooltip)
        ConnectionsToolButton->setToolTip(QCoreApplication::translate("qjackctlMainForm", "Show/hide the connections window", nullptr));
#endif // QT_CONFIG(tooltip)
        ConnectionsToolButton->setText(QCoreApplication::translate("qjackctlMainForm", "&Connect", nullptr));
#if QT_CONFIG(tooltip)
        PatchbayToolButton->setToolTip(QCoreApplication::translate("qjackctlMainForm", "Show/hide the patchbay editor window", nullptr));
#endif // QT_CONFIG(tooltip)
        PatchbayToolButton->setText(QCoreApplication::translate("qjackctlMainForm", "&Patchbay", nullptr));
#if QT_CONFIG(tooltip)
        RewindToolButton->setToolTip(QCoreApplication::translate("qjackctlMainForm", "Rewind transport", nullptr));
#endif // QT_CONFIG(tooltip)
        RewindToolButton->setText(QCoreApplication::translate("qjackctlMainForm", "&Rewind", nullptr));
#if QT_CONFIG(tooltip)
        BackwardToolButton->setToolTip(QCoreApplication::translate("qjackctlMainForm", "Backward transport", nullptr));
#endif // QT_CONFIG(tooltip)
        BackwardToolButton->setText(QCoreApplication::translate("qjackctlMainForm", "&Backward", nullptr));
#if QT_CONFIG(tooltip)
        PlayToolButton->setToolTip(QCoreApplication::translate("qjackctlMainForm", "Start transport rolling", nullptr));
#endif // QT_CONFIG(tooltip)
        PlayToolButton->setText(QCoreApplication::translate("qjackctlMainForm", "&Play", nullptr));
#if QT_CONFIG(tooltip)
        PauseToolButton->setToolTip(QCoreApplication::translate("qjackctlMainForm", "Stop transport rolling", nullptr));
#endif // QT_CONFIG(tooltip)
        PauseToolButton->setText(QCoreApplication::translate("qjackctlMainForm", "Pa&use", nullptr));
#if QT_CONFIG(tooltip)
        ForwardToolButton->setToolTip(QCoreApplication::translate("qjackctlMainForm", "Forward transport", nullptr));
#endif // QT_CONFIG(tooltip)
        ForwardToolButton->setText(QCoreApplication::translate("qjackctlMainForm", "&Forward", nullptr));
#if QT_CONFIG(tooltip)
        AboutToolButton->setToolTip(QCoreApplication::translate("qjackctlMainForm", "Show information about this application", nullptr));
#endif // QT_CONFIG(tooltip)
        AboutToolButton->setText(QCoreApplication::translate("qjackctlMainForm", "Ab&out...", nullptr));
        (void)qjackctlMainForm;
    } // retranslateUi

};

namespace Ui {
    class qjackctlMainForm: public Ui_qjackctlMainForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QJACKCTLMAINFORM_H
