#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 统一使用 Fusion 风格：不同 Windows 主题下控件外观一致，QSS 效果更稳定
    QApplication::setStyle(QStringLiteral("Fusion"));

    MainWindow w;
    w.show();
    return a.exec();
}
