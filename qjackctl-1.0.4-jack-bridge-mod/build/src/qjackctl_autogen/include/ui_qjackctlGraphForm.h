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
** Form generated from reading UI file 'qjackctlGraphForm.ui'
**
** Created by: Qt User Interface Compiler version 6.4.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QJACKCTLGRAPHFORM_H
#define UI_QJACKCTLGRAPHFORM_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "qjackctlGraph.h"

QT_BEGIN_NAMESPACE

class Ui_qjackctlGraphForm
{
public:
    QAction *graphConnectAction;
    QAction *graphDisconnectAction;
    QAction *graphCloseAction;
    QAction *editSelectAllAction;
    QAction *editSelectNoneAction;
    QAction *editSelectInvertAction;
    QAction *editRenameItemAction;
    QAction *editSearchItemAction;
    QAction *viewMenubarAction;
    QAction *viewToolbarAction;
    QAction *viewStatusbarAction;
    QAction *viewThumbviewTopLeftAction;
    QAction *viewThumbviewTopRightAction;
    QAction *viewThumbviewBottomLeftAction;
    QAction *viewThumbviewBottomRightAction;
    QAction *viewThumbviewNoneAction;
    QAction *viewTextBesideIconsAction;
    QAction *viewCenterAction;
    QAction *viewRefreshAction;
    QAction *viewZoomInAction;
    QAction *viewZoomOutAction;
    QAction *viewZoomFitAction;
    QAction *viewZoomResetAction;
    QAction *viewZoomRangeAction;
    QAction *viewColorsJackAudioAction;
    QAction *viewColorsJackMidiAction;
    QAction *viewColorsAlsaMidiAction;
    QAction *viewColorsJackCvAction;
    QAction *viewColorsJackOscAction;
    QAction *viewColorsResetAction;
    QAction *viewSortPortNameAction;
    QAction *viewSortPortTitleAction;
    QAction *viewSortPortIndexAction;
    QAction *viewSortAscendingAction;
    QAction *viewSortDescendingAction;
    QAction *viewRepelOverlappingNodesAction;
    QAction *viewConnectThroughNodesAction;
    QAction *helpAboutAction;
    QAction *helpAboutQtAction;
    QWidget *MainCentralWidget;
    QVBoxLayout *vboxLayout;
    qjackctlGraphCanvas *graphCanvas;
    QStatusBar *StatusBar;
    QMenuBar *MenuBar;
    QMenu *graphMenu;
    QMenu *editMenu;
    QMenu *viewMenu;
    QMenu *viewThumbviewMenu;
    QMenu *viewZoomMenu;
    QMenu *viewColorsMenu;
    QMenu *viewSortMenu;
    QMenu *helpMenu;
    QToolBar *ToolBar;

    void setupUi(QMainWindow *qjackctlGraphForm)
    {
        if (qjackctlGraphForm->objectName().isEmpty())
            qjackctlGraphForm->setObjectName("qjackctlGraphForm");
        qjackctlGraphForm->resize(800, 600);
        const QIcon icon = QIcon(QString::fromUtf8(":/images/graph1.png"));
        qjackctlGraphForm->setWindowIcon(icon);
        graphConnectAction = new QAction(qjackctlGraphForm);
        graphConnectAction->setObjectName("graphConnectAction");
        const QIcon icon1 = QIcon(QString::fromUtf8(":/images/graphConnect.png"));
        graphConnectAction->setIcon(icon1);
        graphDisconnectAction = new QAction(qjackctlGraphForm);
        graphDisconnectAction->setObjectName("graphDisconnectAction");
        const QIcon icon2 = QIcon(QString::fromUtf8(":/images/graphDisconnect.png"));
        graphDisconnectAction->setIcon(icon2);
        graphCloseAction = new QAction(qjackctlGraphForm);
        graphCloseAction->setObjectName("graphCloseAction");
        editSelectAllAction = new QAction(qjackctlGraphForm);
        editSelectAllAction->setObjectName("editSelectAllAction");
        editSelectNoneAction = new QAction(qjackctlGraphForm);
        editSelectNoneAction->setObjectName("editSelectNoneAction");
        editSelectInvertAction = new QAction(qjackctlGraphForm);
        editSelectInvertAction->setObjectName("editSelectInvertAction");
        editRenameItemAction = new QAction(qjackctlGraphForm);
        editRenameItemAction->setObjectName("editRenameItemAction");
        const QIcon icon3 = QIcon(QString::fromUtf8(":/images/graphRename.png"));
        editRenameItemAction->setIcon(icon3);
        editSearchItemAction = new QAction(qjackctlGraphForm);
        editSearchItemAction->setObjectName("editSearchItemAction");
        const QIcon icon4 = QIcon(QString::fromUtf8(":/images/graphFind.png"));
        editSearchItemAction->setIcon(icon4);
        viewMenubarAction = new QAction(qjackctlGraphForm);
        viewMenubarAction->setObjectName("viewMenubarAction");
        viewMenubarAction->setCheckable(true);
        viewToolbarAction = new QAction(qjackctlGraphForm);
        viewToolbarAction->setObjectName("viewToolbarAction");
        viewToolbarAction->setCheckable(true);
        viewStatusbarAction = new QAction(qjackctlGraphForm);
        viewStatusbarAction->setObjectName("viewStatusbarAction");
        viewStatusbarAction->setCheckable(true);
        viewThumbviewTopLeftAction = new QAction(qjackctlGraphForm);
        viewThumbviewTopLeftAction->setObjectName("viewThumbviewTopLeftAction");
        viewThumbviewTopLeftAction->setCheckable(true);
        viewThumbviewTopRightAction = new QAction(qjackctlGraphForm);
        viewThumbviewTopRightAction->setObjectName("viewThumbviewTopRightAction");
        viewThumbviewTopRightAction->setCheckable(true);
        viewThumbviewBottomLeftAction = new QAction(qjackctlGraphForm);
        viewThumbviewBottomLeftAction->setObjectName("viewThumbviewBottomLeftAction");
        viewThumbviewBottomLeftAction->setCheckable(true);
        viewThumbviewBottomRightAction = new QAction(qjackctlGraphForm);
        viewThumbviewBottomRightAction->setObjectName("viewThumbviewBottomRightAction");
        viewThumbviewBottomRightAction->setCheckable(true);
        viewThumbviewNoneAction = new QAction(qjackctlGraphForm);
        viewThumbviewNoneAction->setObjectName("viewThumbviewNoneAction");
        viewThumbviewNoneAction->setCheckable(true);
        viewTextBesideIconsAction = new QAction(qjackctlGraphForm);
        viewTextBesideIconsAction->setObjectName("viewTextBesideIconsAction");
        viewTextBesideIconsAction->setCheckable(true);
        viewCenterAction = new QAction(qjackctlGraphForm);
        viewCenterAction->setObjectName("viewCenterAction");
        const QIcon icon5 = QIcon(QString::fromUtf8(":/images/graphCenter.png"));
        viewCenterAction->setIcon(icon5);
        viewRefreshAction = new QAction(qjackctlGraphForm);
        viewRefreshAction->setObjectName("viewRefreshAction");
        viewZoomInAction = new QAction(qjackctlGraphForm);
        viewZoomInAction->setObjectName("viewZoomInAction");
        const QIcon icon6 = QIcon(QString::fromUtf8(":/images/graphZoomIn.png"));
        viewZoomInAction->setIcon(icon6);
        viewZoomOutAction = new QAction(qjackctlGraphForm);
        viewZoomOutAction->setObjectName("viewZoomOutAction");
        const QIcon icon7 = QIcon(QString::fromUtf8(":/images/graphZoomOut.png"));
        viewZoomOutAction->setIcon(icon7);
        viewZoomFitAction = new QAction(qjackctlGraphForm);
        viewZoomFitAction->setObjectName("viewZoomFitAction");
        const QIcon icon8 = QIcon(QString::fromUtf8(":/images/graphZoomFit.png"));
        viewZoomFitAction->setIcon(icon8);
        viewZoomResetAction = new QAction(qjackctlGraphForm);
        viewZoomResetAction->setObjectName("viewZoomResetAction");
        const QIcon icon9 = QIcon(QString::fromUtf8(":/images/graphZoomReset.png"));
        viewZoomResetAction->setIcon(icon9);
        viewZoomRangeAction = new QAction(qjackctlGraphForm);
        viewZoomRangeAction->setObjectName("viewZoomRangeAction");
        viewZoomRangeAction->setCheckable(true);
        const QIcon icon10 = QIcon(QString::fromUtf8(":/images/graphZoomRange.png"));
        viewZoomRangeAction->setIcon(icon10);
        viewColorsJackAudioAction = new QAction(qjackctlGraphForm);
        viewColorsJackAudioAction->setObjectName("viewColorsJackAudioAction");
        viewColorsJackMidiAction = new QAction(qjackctlGraphForm);
        viewColorsJackMidiAction->setObjectName("viewColorsJackMidiAction");
        viewColorsAlsaMidiAction = new QAction(qjackctlGraphForm);
        viewColorsAlsaMidiAction->setObjectName("viewColorsAlsaMidiAction");
        viewColorsJackCvAction = new QAction(qjackctlGraphForm);
        viewColorsJackCvAction->setObjectName("viewColorsJackCvAction");
        viewColorsJackOscAction = new QAction(qjackctlGraphForm);
        viewColorsJackOscAction->setObjectName("viewColorsJackOscAction");
        viewColorsResetAction = new QAction(qjackctlGraphForm);
        viewColorsResetAction->setObjectName("viewColorsResetAction");
        viewSortPortNameAction = new QAction(qjackctlGraphForm);
        viewSortPortNameAction->setObjectName("viewSortPortNameAction");
        viewSortPortNameAction->setCheckable(true);
        viewSortPortTitleAction = new QAction(qjackctlGraphForm);
        viewSortPortTitleAction->setObjectName("viewSortPortTitleAction");
        viewSortPortTitleAction->setCheckable(true);
        viewSortPortIndexAction = new QAction(qjackctlGraphForm);
        viewSortPortIndexAction->setObjectName("viewSortPortIndexAction");
        viewSortPortIndexAction->setCheckable(true);
        viewSortAscendingAction = new QAction(qjackctlGraphForm);
        viewSortAscendingAction->setObjectName("viewSortAscendingAction");
        viewSortAscendingAction->setCheckable(true);
        viewSortDescendingAction = new QAction(qjackctlGraphForm);
        viewSortDescendingAction->setObjectName("viewSortDescendingAction");
        viewSortDescendingAction->setCheckable(true);
        viewRepelOverlappingNodesAction = new QAction(qjackctlGraphForm);
        viewRepelOverlappingNodesAction->setObjectName("viewRepelOverlappingNodesAction");
        viewRepelOverlappingNodesAction->setCheckable(true);
        viewConnectThroughNodesAction = new QAction(qjackctlGraphForm);
        viewConnectThroughNodesAction->setObjectName("viewConnectThroughNodesAction");
        viewConnectThroughNodesAction->setCheckable(true);
        helpAboutAction = new QAction(qjackctlGraphForm);
        helpAboutAction->setObjectName("helpAboutAction");
        helpAboutQtAction = new QAction(qjackctlGraphForm);
        helpAboutQtAction->setObjectName("helpAboutQtAction");
        MainCentralWidget = new QWidget(qjackctlGraphForm);
        MainCentralWidget->setObjectName("MainCentralWidget");
        vboxLayout = new QVBoxLayout(MainCentralWidget);
        vboxLayout->setSpacing(0);
        vboxLayout->setContentsMargins(0, 0, 0, 0);
        vboxLayout->setObjectName("vboxLayout");
        graphCanvas = new qjackctlGraphCanvas(MainCentralWidget);
        graphCanvas->setObjectName("graphCanvas");

        vboxLayout->addWidget(graphCanvas);

        qjackctlGraphForm->setCentralWidget(MainCentralWidget);
        StatusBar = new QStatusBar(qjackctlGraphForm);
        StatusBar->setObjectName("StatusBar");
        qjackctlGraphForm->setStatusBar(StatusBar);
        MenuBar = new QMenuBar(qjackctlGraphForm);
        MenuBar->setObjectName("MenuBar");
        MenuBar->setGeometry(QRect(0, 0, 800, 20));
        graphMenu = new QMenu(MenuBar);
        graphMenu->setObjectName("graphMenu");
        editMenu = new QMenu(MenuBar);
        editMenu->setObjectName("editMenu");
        viewMenu = new QMenu(MenuBar);
        viewMenu->setObjectName("viewMenu");
        viewThumbviewMenu = new QMenu(viewMenu);
        viewThumbviewMenu->setObjectName("viewThumbviewMenu");
        const QIcon icon11 = QIcon(QString::fromUtf8(":/images/graphThumbview.png"));
        viewThumbviewMenu->setIcon(icon11);
        viewZoomMenu = new QMenu(viewMenu);
        viewZoomMenu->setObjectName("viewZoomMenu");
        const QIcon icon12 = QIcon(QString::fromUtf8(":/images/graphZoomTool.png"));
        viewZoomMenu->setIcon(icon12);
        viewColorsMenu = new QMenu(viewMenu);
        viewColorsMenu->setObjectName("viewColorsMenu");
        const QIcon icon13 = QIcon(QString::fromUtf8(":/images/graphColors.png"));
        viewColorsMenu->setIcon(icon13);
        viewSortMenu = new QMenu(viewMenu);
        viewSortMenu->setObjectName("viewSortMenu");
        helpMenu = new QMenu(MenuBar);
        helpMenu->setObjectName("helpMenu");
        qjackctlGraphForm->setMenuBar(MenuBar);
        ToolBar = new QToolBar(qjackctlGraphForm);
        ToolBar->setObjectName("ToolBar");
        ToolBar->setAllowedAreas(Qt::AllToolBarAreas);
        ToolBar->setOrientation(Qt::Horizontal);
        ToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        ToolBar->setFloatable(true);
        qjackctlGraphForm->addToolBar(Qt::TopToolBarArea, ToolBar);

        MenuBar->addAction(graphMenu->menuAction());
        MenuBar->addAction(editMenu->menuAction());
        MenuBar->addAction(viewMenu->menuAction());
        MenuBar->addSeparator();
        MenuBar->addAction(helpMenu->menuAction());
        graphMenu->addAction(graphConnectAction);
        graphMenu->addAction(graphDisconnectAction);
        graphMenu->addSeparator();
        graphMenu->addAction(graphCloseAction);
        editMenu->addAction(editSelectAllAction);
        editMenu->addAction(editSelectNoneAction);
        editMenu->addAction(editSelectInvertAction);
        editMenu->addSeparator();
        editMenu->addAction(editSearchItemAction);
        editMenu->addAction(editRenameItemAction);
        viewMenu->addAction(viewMenubarAction);
        viewMenu->addAction(viewToolbarAction);
        viewMenu->addAction(viewStatusbarAction);
        viewMenu->addSeparator();
        viewMenu->addAction(viewThumbviewMenu->menuAction());
        viewMenu->addSeparator();
        viewMenu->addAction(viewTextBesideIconsAction);
        viewMenu->addAction(viewColorsMenu->menuAction());
        viewMenu->addAction(viewSortMenu->menuAction());
        viewMenu->addSeparator();
        viewMenu->addAction(viewCenterAction);
        viewMenu->addAction(viewZoomMenu->menuAction());
        viewMenu->addAction(viewZoomRangeAction);
        viewMenu->addSeparator();
        viewMenu->addAction(viewRepelOverlappingNodesAction);
        viewMenu->addAction(viewConnectThroughNodesAction);
        viewMenu->addSeparator();
        viewMenu->addAction(viewRefreshAction);
        viewThumbviewMenu->addAction(viewThumbviewTopLeftAction);
        viewThumbviewMenu->addAction(viewThumbviewTopRightAction);
        viewThumbviewMenu->addAction(viewThumbviewBottomLeftAction);
        viewThumbviewMenu->addAction(viewThumbviewBottomRightAction);
        viewThumbviewMenu->addSeparator();
        viewThumbviewMenu->addAction(viewThumbviewNoneAction);
        viewZoomMenu->addAction(viewZoomInAction);
        viewZoomMenu->addAction(viewZoomOutAction);
        viewZoomMenu->addSeparator();
        viewZoomMenu->addAction(viewZoomFitAction);
        viewZoomMenu->addAction(viewZoomResetAction);
        viewColorsMenu->addAction(viewColorsJackAudioAction);
        viewColorsMenu->addAction(viewColorsJackMidiAction);
        viewColorsMenu->addSeparator();
        viewColorsMenu->addAction(viewColorsResetAction);
        viewSortMenu->addAction(viewSortPortNameAction);
        viewSortMenu->addAction(viewSortPortTitleAction);
        viewSortMenu->addAction(viewSortPortIndexAction);
        viewSortMenu->addSeparator();
        viewSortMenu->addAction(viewSortAscendingAction);
        viewSortMenu->addAction(viewSortDescendingAction);
        helpMenu->addAction(helpAboutAction);
        helpMenu->addAction(helpAboutQtAction);
        ToolBar->addAction(graphConnectAction);
        ToolBar->addAction(graphDisconnectAction);
        ToolBar->addSeparator();
        ToolBar->addAction(viewCenterAction);
        ToolBar->addSeparator();
        ToolBar->addAction(viewZoomInAction);
        ToolBar->addAction(viewZoomOutAction);
        ToolBar->addSeparator();
        ToolBar->addAction(viewZoomFitAction);
        ToolBar->addAction(viewZoomResetAction);
        ToolBar->addSeparator();
        ToolBar->addAction(viewZoomRangeAction);

        retranslateUi(qjackctlGraphForm);

        QMetaObject::connectSlotsByName(qjackctlGraphForm);
    } // setupUi

    void retranslateUi(QMainWindow *qjackctlGraphForm)
    {
        qjackctlGraphForm->setWindowTitle(QCoreApplication::translate("qjackctlGraphForm", "Graph", nullptr));
        graphConnectAction->setText(QCoreApplication::translate("qjackctlGraphForm", "&Connect", nullptr));
        graphConnectAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Connect", nullptr));
#if QT_CONFIG(tooltip)
        graphConnectAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Connect", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        graphConnectAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Connect selected ports", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        graphConnectAction->setShortcut(QCoreApplication::translate("qjackctlGraphForm", "Ctrl+C", nullptr));
#endif // QT_CONFIG(shortcut)
        graphDisconnectAction->setText(QCoreApplication::translate("qjackctlGraphForm", "&Disconnect", nullptr));
        graphDisconnectAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Disconnect", nullptr));
#if QT_CONFIG(tooltip)
        graphDisconnectAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Disconnect", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        graphDisconnectAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Disconnect selected ports", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        graphDisconnectAction->setShortcut(QCoreApplication::translate("qjackctlGraphForm", "Ctrl+D", nullptr));
#endif // QT_CONFIG(shortcut)
        graphCloseAction->setText(QCoreApplication::translate("qjackctlGraphForm", "Cl&ose", nullptr));
        graphCloseAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Close", nullptr));
#if QT_CONFIG(tooltip)
        graphCloseAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Close", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        graphCloseAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Close this application window", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        graphCloseAction->setShortcut(QString());
#endif // QT_CONFIG(shortcut)
        editSelectAllAction->setText(QCoreApplication::translate("qjackctlGraphForm", "Select &All", nullptr));
        editSelectAllAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Select All", nullptr));
#if QT_CONFIG(tooltip)
        editSelectAllAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Select All", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        editSelectAllAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Select All", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        editSelectAllAction->setShortcut(QCoreApplication::translate("qjackctlGraphForm", "Ctrl+A", nullptr));
#endif // QT_CONFIG(shortcut)
        editSelectNoneAction->setText(QCoreApplication::translate("qjackctlGraphForm", "Select &None", nullptr));
        editSelectNoneAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Select None", nullptr));
#if QT_CONFIG(tooltip)
        editSelectNoneAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Select None", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        editSelectNoneAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Select None", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        editSelectNoneAction->setShortcut(QCoreApplication::translate("qjackctlGraphForm", "Ctrl+Shift+A", nullptr));
#endif // QT_CONFIG(shortcut)
        editSelectInvertAction->setText(QCoreApplication::translate("qjackctlGraphForm", "Select &Invert", nullptr));
        editSelectInvertAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Select Invert", nullptr));
#if QT_CONFIG(tooltip)
        editSelectInvertAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Select Invert", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        editSelectInvertAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Select Invert", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        editSelectInvertAction->setShortcut(QCoreApplication::translate("qjackctlGraphForm", "Ctrl+I", nullptr));
#endif // QT_CONFIG(shortcut)
        editRenameItemAction->setText(QCoreApplication::translate("qjackctlGraphForm", "&Rename...", nullptr));
        editRenameItemAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Rename item", nullptr));
#if QT_CONFIG(tooltip)
        editRenameItemAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Rename Item", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        editRenameItemAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Rename Item", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        editRenameItemAction->setShortcut(QCoreApplication::translate("qjackctlGraphForm", "F2", nullptr));
#endif // QT_CONFIG(shortcut)
        editSearchItemAction->setText(QCoreApplication::translate("qjackctlGraphForm", "&Find...", nullptr));
        editSearchItemAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Find", nullptr));
#if QT_CONFIG(tooltip)
        editSearchItemAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Find nodes", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        editSearchItemAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Find nodes", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        editSearchItemAction->setShortcut(QCoreApplication::translate("qjackctlGraphForm", "Ctrl+F", nullptr));
#endif // QT_CONFIG(shortcut)
        viewMenubarAction->setText(QCoreApplication::translate("qjackctlGraphForm", "&Menubar", nullptr));
        viewMenubarAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Menubar", nullptr));
#if QT_CONFIG(tooltip)
        viewMenubarAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Menubar", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewMenubarAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Show/hide the main program window menubar", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewMenubarAction->setShortcut(QCoreApplication::translate("qjackctlGraphForm", "Ctrl+M", nullptr));
#endif // QT_CONFIG(shortcut)
        viewToolbarAction->setText(QCoreApplication::translate("qjackctlGraphForm", "&Toolbar", nullptr));
        viewToolbarAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Toolbar", nullptr));
#if QT_CONFIG(tooltip)
        viewToolbarAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Toolbar", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewToolbarAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Show/hide main program window file toolbar", nullptr));
#endif // QT_CONFIG(statustip)
        viewStatusbarAction->setText(QCoreApplication::translate("qjackctlGraphForm", "&Statusbar", nullptr));
        viewStatusbarAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Statusbar", nullptr));
#if QT_CONFIG(tooltip)
        viewStatusbarAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Statusbar", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewStatusbarAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Show/hide the main program window statusbar", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewStatusbarAction->setShortcut(QString());
#endif // QT_CONFIG(shortcut)
        viewThumbviewTopLeftAction->setText(QCoreApplication::translate("qjackctlGraphForm", "&Top Left", nullptr));
        viewThumbviewTopLeftAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Top left", nullptr));
#if QT_CONFIG(tooltip)
        viewThumbviewTopLeftAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Top left", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewThumbviewTopLeftAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Show the thumbnail overview on the top-left", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewThumbviewTopLeftAction->setShortcut(QString());
#endif // QT_CONFIG(shortcut)
        viewThumbviewTopRightAction->setText(QCoreApplication::translate("qjackctlGraphForm", "Top &Right", nullptr));
        viewThumbviewTopRightAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Top right", nullptr));
#if QT_CONFIG(tooltip)
        viewThumbviewTopRightAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Top right", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewThumbviewTopRightAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Show the thumbnail overview on the top-right", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewThumbviewTopRightAction->setShortcut(QString());
#endif // QT_CONFIG(shortcut)
        viewThumbviewBottomLeftAction->setText(QCoreApplication::translate("qjackctlGraphForm", "Bottom &Left", nullptr));
        viewThumbviewBottomLeftAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Bottom Left", nullptr));
#if QT_CONFIG(tooltip)
        viewThumbviewBottomLeftAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Bottom left", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewThumbviewBottomLeftAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Show the thumbnail overview on the bottom-left", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewThumbviewBottomLeftAction->setShortcut(QString());
#endif // QT_CONFIG(shortcut)
        viewThumbviewBottomRightAction->setText(QCoreApplication::translate("qjackctlGraphForm", "&Bottom Right", nullptr));
        viewThumbviewBottomRightAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Bottom right", nullptr));
#if QT_CONFIG(tooltip)
        viewThumbviewBottomRightAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Bottom right", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewThumbviewBottomRightAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Show the thumbnail overview on the bottom-right", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewThumbviewBottomRightAction->setShortcut(QString());
#endif // QT_CONFIG(shortcut)
        viewThumbviewNoneAction->setText(QCoreApplication::translate("qjackctlGraphForm", "&None", nullptr));
        viewThumbviewNoneAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "None", nullptr));
#if QT_CONFIG(tooltip)
        viewThumbviewNoneAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Hide thumbview", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewThumbviewNoneAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Hide the thumbnail overview", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewThumbviewNoneAction->setShortcut(QString());
#endif // QT_CONFIG(shortcut)
        viewTextBesideIconsAction->setText(QCoreApplication::translate("qjackctlGraphForm", "Text Beside &Icons", nullptr));
        viewTextBesideIconsAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Text beside icons", nullptr));
#if QT_CONFIG(tooltip)
        viewTextBesideIconsAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Text beside icons", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewTextBesideIconsAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Show/hide text beside icons", nullptr));
#endif // QT_CONFIG(statustip)
        viewCenterAction->setText(QCoreApplication::translate("qjackctlGraphForm", "&Center", nullptr));
        viewCenterAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Center", nullptr));
#if QT_CONFIG(tooltip)
        viewCenterAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Center", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewCenterAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Center view", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewCenterAction->setShortcut(QString());
#endif // QT_CONFIG(shortcut)
        viewRefreshAction->setText(QCoreApplication::translate("qjackctlGraphForm", "&Refresh", nullptr));
        viewRefreshAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Refresh", nullptr));
#if QT_CONFIG(tooltip)
        viewRefreshAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Refresh", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewRefreshAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Refresh view", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewRefreshAction->setShortcut(QCoreApplication::translate("qjackctlGraphForm", "F5", nullptr));
#endif // QT_CONFIG(shortcut)
        viewZoomInAction->setText(QCoreApplication::translate("qjackctlGraphForm", "Zoom &In", nullptr));
        viewZoomInAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Zoom In", nullptr));
#if QT_CONFIG(tooltip)
        viewZoomInAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Zoom In", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewZoomInAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Zoom In", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewZoomInAction->setShortcut(QCoreApplication::translate("qjackctlGraphForm", "Ctrl++", nullptr));
#endif // QT_CONFIG(shortcut)
        viewZoomOutAction->setText(QCoreApplication::translate("qjackctlGraphForm", "Zoom &Out", nullptr));
        viewZoomOutAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Zoom Out", nullptr));
#if QT_CONFIG(tooltip)
        viewZoomOutAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Zoom Out", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewZoomOutAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Zoom Out", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewZoomOutAction->setShortcut(QCoreApplication::translate("qjackctlGraphForm", "Ctrl+-", nullptr));
#endif // QT_CONFIG(shortcut)
        viewZoomFitAction->setText(QCoreApplication::translate("qjackctlGraphForm", "Zoom &Fit", nullptr));
        viewZoomFitAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Zoom Fit", nullptr));
#if QT_CONFIG(tooltip)
        viewZoomFitAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Zoom Fit", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewZoomFitAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Zoom Fit", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewZoomFitAction->setShortcut(QCoreApplication::translate("qjackctlGraphForm", "Ctrl+0", nullptr));
#endif // QT_CONFIG(shortcut)
        viewZoomResetAction->setText(QCoreApplication::translate("qjackctlGraphForm", "Zoom &Reset", nullptr));
        viewZoomResetAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Zoom Reset", nullptr));
#if QT_CONFIG(tooltip)
        viewZoomResetAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Zoom Reset", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewZoomResetAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Zoom Reset", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewZoomResetAction->setShortcut(QCoreApplication::translate("qjackctlGraphForm", "Ctrl+1", nullptr));
#endif // QT_CONFIG(shortcut)
        viewZoomRangeAction->setText(QCoreApplication::translate("qjackctlGraphForm", "&Zoom Range", nullptr));
        viewZoomRangeAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Zoom Range", nullptr));
#if QT_CONFIG(tooltip)
        viewZoomRangeAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Zoom Range", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewZoomRangeAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Zoom Range", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewZoomRangeAction->setShortcut(QString());
#endif // QT_CONFIG(shortcut)
        viewColorsJackAudioAction->setText(QCoreApplication::translate("qjackctlGraphForm", "JACK &Audio...", nullptr));
        viewColorsJackAudioAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "JACK Audio color", nullptr));
#if QT_CONFIG(tooltip)
        viewColorsJackAudioAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "JACK Audio color", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewColorsJackAudioAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "JACK Audio color", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewColorsJackAudioAction->setShortcut(QString());
#endif // QT_CONFIG(shortcut)
        viewColorsJackMidiAction->setText(QCoreApplication::translate("qjackctlGraphForm", "JACK &MIDI...", nullptr));
        viewColorsJackMidiAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "JACK MIDI", nullptr));
#if QT_CONFIG(tooltip)
        viewColorsJackMidiAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "JACK MIDI color", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewColorsJackMidiAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "JACK MIDI color", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewColorsJackMidiAction->setShortcut(QString());
#endif // QT_CONFIG(shortcut)
        viewColorsAlsaMidiAction->setText(QCoreApplication::translate("qjackctlGraphForm", "ALSA M&IDI...", nullptr));
        viewColorsAlsaMidiAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "ALSA MIDI", nullptr));
#if QT_CONFIG(tooltip)
        viewColorsAlsaMidiAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "ALSA MIDI color", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewColorsAlsaMidiAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "ALSA MIDI color", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewColorsAlsaMidiAction->setShortcut(QString());
#endif // QT_CONFIG(shortcut)
        viewColorsJackCvAction->setText(QCoreApplication::translate("qjackctlGraphForm", "JACK &CV...", nullptr));
        viewColorsJackCvAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "JACK CV color", nullptr));
#if QT_CONFIG(tooltip)
        viewColorsJackCvAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "JACK CV color", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewColorsJackCvAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "JACK CV color", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewColorsJackCvAction->setShortcut(QString());
#endif // QT_CONFIG(shortcut)
        viewColorsJackOscAction->setText(QCoreApplication::translate("qjackctlGraphForm", "JACK &OSC...", nullptr));
        viewColorsJackOscAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "JACK OSC", nullptr));
#if QT_CONFIG(tooltip)
        viewColorsJackOscAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "JACK OSC color", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewColorsJackOscAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "JACK OSC color", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewColorsJackOscAction->setShortcut(QString());
#endif // QT_CONFIG(shortcut)
        viewColorsResetAction->setText(QCoreApplication::translate("qjackctlGraphForm", "&Reset", nullptr));
        viewColorsResetAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Reset colors", nullptr));
#if QT_CONFIG(tooltip)
        viewColorsResetAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Reset colors", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewColorsResetAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Reset colors", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewColorsResetAction->setShortcut(QString());
#endif // QT_CONFIG(shortcut)
        viewSortPortNameAction->setText(QCoreApplication::translate("qjackctlGraphForm", "Port &Name", nullptr));
#if QT_CONFIG(tooltip)
        viewSortPortNameAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Port name", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewSortPortNameAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Sort by port name", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewSortPortNameAction->setShortcut(QString());
#endif // QT_CONFIG(shortcut)
        viewSortPortTitleAction->setText(QCoreApplication::translate("qjackctlGraphForm", "Port &Title", nullptr));
#if QT_CONFIG(tooltip)
        viewSortPortTitleAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Port title", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewSortPortTitleAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Sort by port title", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewSortPortTitleAction->setShortcut(QString());
#endif // QT_CONFIG(shortcut)
        viewSortPortIndexAction->setText(QCoreApplication::translate("qjackctlGraphForm", "Port &Index", nullptr));
#if QT_CONFIG(tooltip)
        viewSortPortIndexAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Port index", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewSortPortIndexAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Sort by port index", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewSortPortIndexAction->setShortcut(QString());
#endif // QT_CONFIG(shortcut)
        viewSortAscendingAction->setText(QCoreApplication::translate("qjackctlGraphForm", "&Ascending", nullptr));
#if QT_CONFIG(tooltip)
        viewSortAscendingAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Ascending", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewSortAscendingAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Ascending sort order", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewSortAscendingAction->setShortcut(QString());
#endif // QT_CONFIG(shortcut)
        viewSortDescendingAction->setText(QCoreApplication::translate("qjackctlGraphForm", "&Descending", nullptr));
#if QT_CONFIG(tooltip)
        viewSortDescendingAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Descending", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewSortDescendingAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Descending sort order", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewSortDescendingAction->setShortcut(QString());
#endif // QT_CONFIG(shortcut)
        viewRepelOverlappingNodesAction->setText(QCoreApplication::translate("qjackctlGraphForm", "Repel O&verlapping Nodes", nullptr));
        viewRepelOverlappingNodesAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Repel nodes", nullptr));
#if QT_CONFIG(tooltip)
        viewRepelOverlappingNodesAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Repel overlapping nodes", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewRepelOverlappingNodesAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Repel overlapping nodes", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewRepelOverlappingNodesAction->setShortcut(QString());
#endif // QT_CONFIG(shortcut)
        viewConnectThroughNodesAction->setText(QCoreApplication::translate("qjackctlGraphForm", "Connect Thro&ugh Nodes", nullptr));
        viewConnectThroughNodesAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "Connect Through Nodes", nullptr));
#if QT_CONFIG(tooltip)
        viewConnectThroughNodesAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "Connect through nodes", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        viewConnectThroughNodesAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Whether to draw connectors through or around nodes", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        viewConnectThroughNodesAction->setShortcut(QString());
#endif // QT_CONFIG(shortcut)
        helpAboutAction->setText(QCoreApplication::translate("qjackctlGraphForm", "&About...", nullptr));
        helpAboutAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "About...", nullptr));
#if QT_CONFIG(tooltip)
        helpAboutAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "About", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        helpAboutAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Show information about this application program", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        helpAboutAction->setShortcut(QString());
#endif // QT_CONFIG(shortcut)
        helpAboutQtAction->setText(QCoreApplication::translate("qjackctlGraphForm", "About &Qt...", nullptr));
        helpAboutQtAction->setIconText(QCoreApplication::translate("qjackctlGraphForm", "About Qt...", nullptr));
#if QT_CONFIG(tooltip)
        helpAboutQtAction->setToolTip(QCoreApplication::translate("qjackctlGraphForm", "About Qt", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        helpAboutQtAction->setStatusTip(QCoreApplication::translate("qjackctlGraphForm", "Show information about the Qt toolkit", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        helpAboutQtAction->setShortcut(QString());
#endif // QT_CONFIG(shortcut)
        graphMenu->setTitle(QCoreApplication::translate("qjackctlGraphForm", "&Graph", nullptr));
        editMenu->setTitle(QCoreApplication::translate("qjackctlGraphForm", "&Edit", nullptr));
        viewMenu->setTitle(QCoreApplication::translate("qjackctlGraphForm", "&View", nullptr));
        viewThumbviewMenu->setTitle(QCoreApplication::translate("qjackctlGraphForm", "T&humbview", nullptr));
        viewZoomMenu->setTitle(QCoreApplication::translate("qjackctlGraphForm", "&Zoom", nullptr));
        viewColorsMenu->setTitle(QCoreApplication::translate("qjackctlGraphForm", "Co&lors", nullptr));
        viewSortMenu->setTitle(QCoreApplication::translate("qjackctlGraphForm", "S&ort", nullptr));
        helpMenu->setTitle(QCoreApplication::translate("qjackctlGraphForm", "&Help", nullptr));
    } // retranslateUi

};

namespace Ui {
    class qjackctlGraphForm: public Ui_qjackctlGraphForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QJACKCTLGRAPHFORM_H
