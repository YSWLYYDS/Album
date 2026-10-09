#include "mainwindow.h"

#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    qputenv("QT_LOGGING_RULES", "qt.multimedia.ffmpeg=false");
    QApplication a(argc, argv);
    QFile qss(":/style/style.qss");
    if(qss.open(QFile::ReadOnly))
    {
        qDebug("open qss success");
        QString style=QLatin1StringView(qss.readAll());
        a.setStyleSheet(style);
        qss.close();
    }
    else{
        qDebug("open qss filed");
        return 0;
    }

    MainWindow w;
    w.setWindowTitle("Album");
    w.showMaximized();
    return a.exec();
}
