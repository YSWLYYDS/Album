#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include "nodeinfo.h"

int main(int argc, char *argv[])
{
    qputenv("QT_LOGGING_RULES", "qt.multimedia.ffmpeg=false");//关掉多媒体ffmpeg刷屏日志
    QApplication a(argc, argv);
    qRegisterMetaType<NodeInfo>();//跨线程队列信号槽传 NodeInfo 必须注册
    qRegisterMetaType<QList<NodeInfo>>();//批量信号的参数类型同样要注册
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
