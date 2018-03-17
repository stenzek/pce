SET PATH=%PATH%;%~dp0\..\..\build\vc2015\dep\qt5-x64\bin

moc -o moc_debuggerwindow.cpp debuggerwindow.h
moc -o moc_displaywidget.cpp displaywidget.h
moc -o moc_mainwindow.cpp mainwindow.h
uic -o ui_debuggerwindow.h debuggerwindow.ui
uic -o ui_mainwindow.h mainwindow.ui
rcc -o rcc_icons.cpp resources/icons.qrc

pause