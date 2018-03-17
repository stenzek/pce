/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
  QAction* action_About;
  QAction* action_DisplayScale1x;
  QAction* action_DisplayScale2x;
  QAction* action_DisplayScale3x;
  QAction* actionReset;
  QAction* actionSend_Ctrl_Alt_Delete;
  QAction* actionCapture;
  QAction* actionClose;
  QAction* actionChange_Floppy_A;
  QAction* actionChange_Floppy_B;
  QAction* actionEnableDebugger;
  QAction* actionPower;
  QAction* actionPause;
  QAction* actionLoadState;
  QAction* actionSaveState;
  QWidget* centralwidget;
  QMenuBar* menubar;
  QMenu* menu_System;
  QMenu* menu_View;
  QMenu* menuDisplay_Scale;
  QMenu* menu_Help;
  QMenu* menu_Help_2;
  QMenu* menu_Input;
  QStatusBar* statusbar;
  QToolBar* toolBar;

  void setupUi(QMainWindow* MainWindow)
  {
    if (MainWindow->objectName().isEmpty())
      MainWindow->setObjectName(QStringLiteral("MainWindow"));
    MainWindow->resize(800, 600);
    action_About = new QAction(MainWindow);
    action_About->setObjectName(QStringLiteral("action_About"));
    action_DisplayScale1x = new QAction(MainWindow);
    action_DisplayScale1x->setObjectName(QStringLiteral("action_DisplayScale1x"));
    action_DisplayScale2x = new QAction(MainWindow);
    action_DisplayScale2x->setObjectName(QStringLiteral("action_DisplayScale2x"));
    action_DisplayScale3x = new QAction(MainWindow);
    action_DisplayScale3x->setObjectName(QStringLiteral("action_DisplayScale3x"));
    actionReset = new QAction(MainWindow);
    actionReset->setObjectName(QStringLiteral("actionReset"));
    QIcon icon;
    icon.addFile(QStringLiteral(":/icons/system-reboot.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionReset->setIcon(icon);
    actionSend_Ctrl_Alt_Delete = new QAction(MainWindow);
    actionSend_Ctrl_Alt_Delete->setObjectName(QStringLiteral("actionSend_Ctrl_Alt_Delete"));
    QIcon icon1;
    icon1.addFile(QStringLiteral(":/icons/emblem-symbolic-link.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionSend_Ctrl_Alt_Delete->setIcon(icon1);
    actionCapture = new QAction(MainWindow);
    actionCapture->setObjectName(QStringLiteral("actionCapture"));
    QIcon icon2;
    icon2.addFile(QStringLiteral(":/icons/input-mouse.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionCapture->setIcon(icon2);
    actionClose = new QAction(MainWindow);
    actionClose->setObjectName(QStringLiteral("actionClose"));
    QIcon icon3;
    icon3.addFile(QStringLiteral(":/icons/system-shutdown.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionClose->setIcon(icon3);
    actionChange_Floppy_A = new QAction(MainWindow);
    actionChange_Floppy_A->setObjectName(QStringLiteral("actionChange_Floppy_A"));
    QIcon icon4;
    icon4.addFile(QStringLiteral(":/icons/media-floppy.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionChange_Floppy_A->setIcon(icon4);
    actionChange_Floppy_B = new QAction(MainWindow);
    actionChange_Floppy_B->setObjectName(QStringLiteral("actionChange_Floppy_B"));
    actionChange_Floppy_B->setIcon(icon4);
    actionEnableDebugger = new QAction(MainWindow);
    actionEnableDebugger->setObjectName(QStringLiteral("actionEnableDebugger"));
    actionEnableDebugger->setCheckable(true);
    actionPower = new QAction(MainWindow);
    actionPower->setObjectName(QStringLiteral("actionPower"));
    actionPower->setCheckable(true);
    QIcon icon5;
    icon5.addFile(QStringLiteral(":/icons/system-run.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionPower->setIcon(icon5);
    actionPause = new QAction(MainWindow);
    actionPause->setObjectName(QStringLiteral("actionPause"));
    actionPause->setCheckable(true);
    actionPause->setChecked(false);
    QIcon icon6;
    icon6.addFile(QStringLiteral(":/icons/media-playback-pause.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionPause->setIcon(icon6);
    actionLoadState = new QAction(MainWindow);
    actionLoadState->setObjectName(QStringLiteral("actionLoadState"));
    QIcon icon7;
    icon7.addFile(QStringLiteral(":/icons/media-seek-backward.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionLoadState->setIcon(icon7);
    actionSaveState = new QAction(MainWindow);
    actionSaveState->setObjectName(QStringLiteral("actionSaveState"));
    QIcon icon8;
    icon8.addFile(QStringLiteral(":/icons/media-record.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionSaveState->setIcon(icon8);
    centralwidget = new QWidget(MainWindow);
    centralwidget->setObjectName(QStringLiteral("centralwidget"));
    QSizePolicy sizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(centralwidget->sizePolicy().hasHeightForWidth());
    centralwidget->setSizePolicy(sizePolicy);
    MainWindow->setCentralWidget(centralwidget);
    menubar = new QMenuBar(MainWindow);
    menubar->setObjectName(QStringLiteral("menubar"));
    menubar->setGeometry(QRect(0, 0, 800, 21));
    menu_System = new QMenu(menubar);
    menu_System->setObjectName(QStringLiteral("menu_System"));
    menu_View = new QMenu(menubar);
    menu_View->setObjectName(QStringLiteral("menu_View"));
    menuDisplay_Scale = new QMenu(menu_View);
    menuDisplay_Scale->setObjectName(QStringLiteral("menuDisplay_Scale"));
    menu_Help = new QMenu(menubar);
    menu_Help->setObjectName(QStringLiteral("menu_Help"));
    menu_Help_2 = new QMenu(menubar);
    menu_Help_2->setObjectName(QStringLiteral("menu_Help_2"));
    menu_Input = new QMenu(menubar);
    menu_Input->setObjectName(QStringLiteral("menu_Input"));
    MainWindow->setMenuBar(menubar);
    statusbar = new QStatusBar(MainWindow);
    statusbar->setObjectName(QStringLiteral("statusbar"));
    MainWindow->setStatusBar(statusbar);
    toolBar = new QToolBar(MainWindow);
    toolBar->setObjectName(QStringLiteral("toolBar"));
    MainWindow->addToolBar(Qt::TopToolBarArea, toolBar);

    menubar->addAction(menu_System->menuAction());
    menubar->addAction(menu_Input->menuAction());
    menubar->addAction(menu_Help->menuAction());
    menubar->addAction(menu_View->menuAction());
    menubar->addAction(menu_Help_2->menuAction());
    menu_System->addAction(actionEnableDebugger);
    menu_System->addSeparator();
    menu_System->addAction(actionPower);
    menu_System->addAction(actionPause);
    menu_System->addAction(actionReset);
    menu_System->addSeparator();
    menu_System->addAction(actionLoadState);
    menu_System->addAction(actionSaveState);
    menu_System->addSeparator();
    menu_System->addAction(actionClose);
    menu_View->addAction(menuDisplay_Scale->menuAction());
    menuDisplay_Scale->addAction(action_DisplayScale1x);
    menuDisplay_Scale->addAction(action_DisplayScale2x);
    menuDisplay_Scale->addAction(action_DisplayScale3x);
    menu_Help->addAction(actionChange_Floppy_A);
    menu_Help->addAction(actionChange_Floppy_B);
    menu_Help_2->addAction(action_About);
    menu_Input->addAction(actionCapture);
    menu_Input->addAction(actionSend_Ctrl_Alt_Delete);
    toolBar->addAction(actionCapture);
    toolBar->addAction(actionSend_Ctrl_Alt_Delete);
    toolBar->addSeparator();
    toolBar->addAction(actionPower);
    toolBar->addAction(actionPause);
    toolBar->addAction(actionReset);
    toolBar->addSeparator();
    toolBar->addAction(actionLoadState);
    toolBar->addAction(actionSaveState);
    toolBar->addSeparator();
    toolBar->addAction(actionChange_Floppy_A);
    toolBar->addAction(actionChange_Floppy_B);

    retranslateUi(MainWindow);
    QObject::connect(actionClose, SIGNAL(triggered()), MainWindow, SLOT(close()));

    QMetaObject::connectSlotsByName(MainWindow);
  } // setupUi

  void retranslateUi(QMainWindow* MainWindow)
  {
    MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", 0));
    action_About->setText(QApplication::translate("MainWindow", "&About", 0));
    action_DisplayScale1x->setText(QApplication::translate("MainWindow", "&1x", 0));
    action_DisplayScale2x->setText(QApplication::translate("MainWindow", "&2x", 0));
    action_DisplayScale3x->setText(QApplication::translate("MainWindow", "&3x", 0));
    actionReset->setText(QApplication::translate("MainWindow", "&Reset", 0));
    actionSend_Ctrl_Alt_Delete->setText(QApplication::translate("MainWindow", "Send Ctrl+Alt+&Del", 0));
    actionCapture->setText(QApplication::translate("MainWindow", "&Capture", 0));
    actionClose->setText(QApplication::translate("MainWindow", "&Close", 0));
    actionChange_Floppy_A->setText(QApplication::translate("MainWindow", "Change Floppy &A", 0));
    actionChange_Floppy_B->setText(QApplication::translate("MainWindow", "Change Floppy &B", 0));
    actionEnableDebugger->setText(QApplication::translate("MainWindow", "Enable &Debugger", 0));
#ifndef QT_NO_TOOLTIP
    actionEnableDebugger->setToolTip(QApplication::translate("MainWindow", "Enable Debugger", 0));
#endif // QT_NO_TOOLTIP
    actionPower->setText(QApplication::translate("MainWindow", "P&ower", 0));
    actionPause->setText(QApplication::translate("MainWindow", "&Pause", 0));
    actionLoadState->setText(QApplication::translate("MainWindow", "&Load State", 0));
    actionSaveState->setText(QApplication::translate("MainWindow", "&Save State", 0));
    menu_System->setTitle(QApplication::translate("MainWindow", "&System", 0));
    menu_View->setTitle(QApplication::translate("MainWindow", "&View", 0));
    menuDisplay_Scale->setTitle(QApplication::translate("MainWindow", "Display &Scale", 0));
    menu_Help->setTitle(QApplication::translate("MainWindow", "&Devices", 0));
    menu_Help_2->setTitle(QApplication::translate("MainWindow", "&Help", 0));
    menu_Input->setTitle(QApplication::translate("MainWindow", "&Input", 0));
    toolBar->setWindowTitle(QApplication::translate("MainWindow", "toolBar", 0));
  } // retranslateUi
};

namespace Ui {
class MainWindow : public Ui_MainWindow
{
};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
