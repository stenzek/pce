/********************************************************************************
** Form generated from reading UI file 'debuggerwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DEBUGGERWINDOW_H
#define UI_DEBUGGERWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_DebuggerWindow
{
public:
  QAction* actionPause_Continue;
  QAction* actionStep_Into;
  QAction* actionStep_Over;
  QAction* actionToggle_Breakpoint;
  QAction* action_Close;
  QWidget* centralwidget;
  QGridLayout* gridLayout;
  QSplitter* splitter_3;
  QSplitter* splitter;
  QTreeView* codeView;
  QTreeView* registerView;
  QSplitter* splitter_2;
  QTabWidget* tabWidget;
  QWidget* consoleTab;
  QVBoxLayout* verticalLayout;
  QListWidget* consoleLog;
  QHBoxLayout* horizontalLayout_2;
  QLabel* label;
  QLineEdit* lineEdit;
  QWidget* tabMemoryView;
  QTreeView* stackView;
  QMenuBar* menubar;
  QMenu* menu_Debugger;
  QStatusBar* statusbar;
  QToolBar* toolBar;

  void setupUi(QMainWindow* DebuggerWindow)
  {
    if (DebuggerWindow->objectName().isEmpty())
      DebuggerWindow->setObjectName(QStringLiteral("DebuggerWindow"));
    DebuggerWindow->resize(939, 731);
    actionPause_Continue = new QAction(DebuggerWindow);
    actionPause_Continue->setObjectName(QStringLiteral("actionPause_Continue"));
    actionPause_Continue->setCheckable(true);
    QIcon icon;
    icon.addFile(QStringLiteral(":/icons/debug-run.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionPause_Continue->setIcon(icon);
    actionStep_Into = new QAction(DebuggerWindow);
    actionStep_Into->setObjectName(QStringLiteral("actionStep_Into"));
    QIcon icon1;
    icon1.addFile(QStringLiteral(":/icons/debug-step-into-instruction.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionStep_Into->setIcon(icon1);
    actionStep_Over = new QAction(DebuggerWindow);
    actionStep_Over->setObjectName(QStringLiteral("actionStep_Over"));
    QIcon icon2;
    icon2.addFile(QStringLiteral(":/icons/debug-step-over.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionStep_Over->setIcon(icon2);
    actionToggle_Breakpoint = new QAction(DebuggerWindow);
    actionToggle_Breakpoint->setObjectName(QStringLiteral("actionToggle_Breakpoint"));
    QIcon icon3;
    icon3.addFile(QStringLiteral(":/icons/emblem-important.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionToggle_Breakpoint->setIcon(icon3);
    action_Close = new QAction(DebuggerWindow);
    action_Close->setObjectName(QStringLiteral("action_Close"));
    centralwidget = new QWidget(DebuggerWindow);
    centralwidget->setObjectName(QStringLiteral("centralwidget"));
    gridLayout = new QGridLayout(centralwidget);
    gridLayout->setObjectName(QStringLiteral("gridLayout"));
    splitter_3 = new QSplitter(centralwidget);
    splitter_3->setObjectName(QStringLiteral("splitter_3"));
    splitter_3->setOrientation(Qt::Vertical);
    splitter = new QSplitter(splitter_3);
    splitter->setObjectName(QStringLiteral("splitter"));
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(1);
    sizePolicy.setHeightForWidth(splitter->sizePolicy().hasHeightForWidth());
    splitter->setSizePolicy(sizePolicy);
    splitter->setOrientation(Qt::Horizontal);
    codeView = new QTreeView(splitter);
    codeView->setObjectName(QStringLiteral("codeView"));
    QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(codeView->sizePolicy().hasHeightForWidth());
    codeView->setSizePolicy(sizePolicy1);
    splitter->addWidget(codeView);
    registerView = new QTreeView(splitter);
    registerView->setObjectName(QStringLiteral("registerView"));
    sizePolicy1.setHeightForWidth(registerView->sizePolicy().hasHeightForWidth());
    registerView->setSizePolicy(sizePolicy1);
    registerView->setMaximumSize(QSize(220, 16777215));
    splitter->addWidget(registerView);
    splitter_3->addWidget(splitter);
    splitter_2 = new QSplitter(splitter_3);
    splitter_2->setObjectName(QStringLiteral("splitter_2"));
    splitter_2->setOrientation(Qt::Horizontal);
    tabWidget = new QTabWidget(splitter_2);
    tabWidget->setObjectName(QStringLiteral("tabWidget"));
    consoleTab = new QWidget();
    consoleTab->setObjectName(QStringLiteral("consoleTab"));
    verticalLayout = new QVBoxLayout(consoleTab);
    verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
    consoleLog = new QListWidget(consoleTab);
    consoleLog->setObjectName(QStringLiteral("consoleLog"));

    verticalLayout->addWidget(consoleLog);

    horizontalLayout_2 = new QHBoxLayout();
    horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
    label = new QLabel(consoleTab);
    label->setObjectName(QStringLiteral("label"));

    horizontalLayout_2->addWidget(label);

    lineEdit = new QLineEdit(consoleTab);
    lineEdit->setObjectName(QStringLiteral("lineEdit"));

    horizontalLayout_2->addWidget(lineEdit);

    verticalLayout->addLayout(horizontalLayout_2);

    tabWidget->addTab(consoleTab, QString());
    tabMemoryView = new QWidget();
    tabMemoryView->setObjectName(QStringLiteral("tabMemoryView"));
    tabWidget->addTab(tabMemoryView, QString());
    splitter_2->addWidget(tabWidget);
    stackView = new QTreeView(splitter_2);
    stackView->setObjectName(QStringLiteral("stackView"));
    stackView->setMaximumSize(QSize(220, 16777215));
    splitter_2->addWidget(stackView);
    splitter_3->addWidget(splitter_2);

    gridLayout->addWidget(splitter_3, 0, 0, 1, 1);

    DebuggerWindow->setCentralWidget(centralwidget);
    menubar = new QMenuBar(DebuggerWindow);
    menubar->setObjectName(QStringLiteral("menubar"));
    menubar->setGeometry(QRect(0, 0, 939, 21));
    menu_Debugger = new QMenu(menubar);
    menu_Debugger->setObjectName(QStringLiteral("menu_Debugger"));
    DebuggerWindow->setMenuBar(menubar);
    statusbar = new QStatusBar(DebuggerWindow);
    statusbar->setObjectName(QStringLiteral("statusbar"));
    DebuggerWindow->setStatusBar(statusbar);
    toolBar = new QToolBar(DebuggerWindow);
    toolBar->setObjectName(QStringLiteral("toolBar"));
    DebuggerWindow->addToolBar(Qt::TopToolBarArea, toolBar);

    menubar->addAction(menu_Debugger->menuAction());
    menu_Debugger->addAction(actionPause_Continue);
    menu_Debugger->addSeparator();
    menu_Debugger->addAction(actionStep_Into);
    menu_Debugger->addAction(actionStep_Over);
    menu_Debugger->addSeparator();
    menu_Debugger->addAction(actionToggle_Breakpoint);
    menu_Debugger->addSeparator();
    menu_Debugger->addAction(action_Close);
    toolBar->addAction(actionPause_Continue);
    toolBar->addAction(actionStep_Into);
    toolBar->addAction(actionStep_Over);
    toolBar->addAction(actionToggle_Breakpoint);

    retranslateUi(DebuggerWindow);

    tabWidget->setCurrentIndex(0);

    QMetaObject::connectSlotsByName(DebuggerWindow);
  } // setupUi

  void retranslateUi(QMainWindow* DebuggerWindow)
  {
    DebuggerWindow->setWindowTitle(QApplication::translate("DebuggerWindow", "MainWindow", 0));
    actionPause_Continue->setText(QApplication::translate("DebuggerWindow", "Pause/Continue", 0));
    actionStep_Into->setText(QApplication::translate("DebuggerWindow", "Step Into", 0));
    actionStep_Over->setText(QApplication::translate("DebuggerWindow", "Step Over", 0));
    actionToggle_Breakpoint->setText(QApplication::translate("DebuggerWindow", "Toggle Breakpoint", 0));
    action_Close->setText(QApplication::translate("DebuggerWindow", "&Close", 0));
    label->setText(QApplication::translate("DebuggerWindow", "Command:", 0));
    tabWidget->setTabText(tabWidget->indexOf(consoleTab), QApplication::translate("DebuggerWindow", "Console", 0));
    tabWidget->setTabText(tabWidget->indexOf(tabMemoryView), QApplication::translate("DebuggerWindow", "Memory", 0));
    menu_Debugger->setTitle(QApplication::translate("DebuggerWindow", "&Debugger", 0));
    toolBar->setWindowTitle(QApplication::translate("DebuggerWindow", "toolBar", 0));
  } // retranslateUi
};

namespace Ui {
class DebuggerWindow : public Ui_DebuggerWindow
{
};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DEBUGGERWINDOW_H
