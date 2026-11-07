#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include "nodeinfo.h"
#include <QTimer>
#include <QTableWidget>
#include <QFontDatabase>
#include <QDir>
#include <QFileInfo>
#include <QTabWidget>
#include "picshow.h"
#include "protreewidget.h"
#include "protreeitem.h"
#include "fitimageview.h"
#include <QTreeWidgetItemIterator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFontDatabase::addApplicationFont(QDir(qEnvironmentVariable("WINDIR","C:/Windows")).filePath("Fonts/msyh.ttc"));
    a.setFont(QFont("Microsoft YaHei",10));
    a.setOrganizationName("LiuZiqiang");
    a.setApplicationName("ProductImageReview");
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
    w.setWindowTitle("营销素材归档与审核工具");
    w.setWindowIcon(QIcon(":/icon/bulm.ico"));
    w.showMaximized();
    if(a.arguments().contains("--smoke-test")){
        w.showNormal();w.resize(1360,920);
        if(qEnvironmentVariableIsSet("BUSINESS_BROWSER_IMAGE")){
            QTimer::singleShot(100,&w,[&]{
                w.findChild<QTabWidget*>("assetPages")->setCurrentIndex(1);
                auto *tree=w.findChild<ProTreeWidget*>();
                const QString directory=qEnvironmentVariable("BUSINESS_BROWSER_DIRECTORY");
                if(!directory.isEmpty()){tree->SlotOpenPro(directory);tree->expandAll();}
                const QString image=qEnvironmentVariable("BUSINESS_BROWSER_IMAGE");
                bool selected=false;
                for(QTreeWidgetItemIterator it(tree);*it;++it){
                    auto *item=dynamic_cast<ProTreeItem*>(*it);
                    if(item&&QFileInfo(item->GetPath()).absoluteFilePath()==QFileInfo(image).absoluteFilePath()){
                        tree->setCurrentItem(item);
                        QMetaObject::invokeMethod(tree,"SlotDoubleClickItem",Qt::DirectConnection,Q_ARG(QTreeWidgetItem*,item),Q_ARG(int,0));selected=true;break;
                    }
                }
                if(!selected)w.findChild<PicShow*>()->SlotSelectItem(image);
            });
        }
        QTimer::singleShot(300,&w,[&]{auto *table=w.findChild<QTableWidget*>("reviewImageTable");if(table&&table->rowCount()>0)table->selectRow(0);});
        QTimer::singleShot(2000,&w,[&]{
            const QString path=qEnvironmentVariable("BUSINESS_SCREENSHOT");
            if(!path.isEmpty())w.grab().save(path);
            if(qEnvironmentVariableIsSet("BUSINESS_BROWSER_IMAGE")){
                auto *canvas=static_cast<FitImageView*>(w.findChild<QWidget*>("imageViewport"));
                if(!canvas||canvas->imageSize().isEmpty()||!QRectF(canvas->rect()).contains(canvas->imageRect())||w.width()!=1360||w.height()!=920){a.exit(2);return;}
                const QRect canvasBounds(canvas->mapTo(&w,QPoint(0,0)),canvas->size());
                if(!w.rect().contains(canvasBounds)){a.exit(3);return;}
            }
            a.quit();
        });
    }
    return a.exec();
}
