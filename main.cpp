#include "mainwindow.h"
#include "Maps.h"

#include <QApplication>
#include <QFile>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFile styleSheet(":/stylesheet/StyleSheet/style.qss");
    styleSheet.open(QFile::ReadOnly);

    QString style=styleSheet.readAll();
    a.setStyleSheet(style);
    MainWindow *w=MainWindow::getInstance();
    w->setWindowIcon(QIcon(":/Icons/Icons/Icon.png"));
    w->show();
    return a.exec();
}
