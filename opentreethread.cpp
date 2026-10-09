#include "opentreethread.h"
#include "const.h"
#include <ProTreeItem.h>
#include <QDir.h>

OpenTreeThread::OpenTreeThread(const QString &src_path, int file_count, QTreeWidget *self, QObject *parent)
    :QThread(parent),_bstop(false),_src_path(src_path),_file_count(file_count),
    _self(self),_root(nullptr)
{

}

void OpenTreeThread::OpenProTree(const QString &src_path, int &file_count, QTreeWidget *self)
{
    QDir src_dir(src_path);
    auto name=src_dir.dirName();
    auto *item=new ProTreeItem(self,name,src_path,TreeItemPro);
    item->setData(0,Qt::DisplayRole,name);
    item->setData(0,Qt::DecorationRole,QIcon(":/icon/dir.png"));
    item->setData(0,Qt::ToolTipRole,src_path);
    _root=item;
    RecursiveProTree(src_path,file_count,self,_root,item,nullptr);

}

void OpenTreeThread::run()
{
    OpenProTree(_src_path,_file_count,_self);
    if(_bstop){
        auto path=dynamic_cast<ProTreeItem*>(_root)->GetPath();
        auto index=_self->indexOfTopLevelItem(_root);
        delete _self->takeTopLevelItem(index);
        QDir dir(path);
        dir.removeRecursively();
        return;
    }
    emit SignFinishProgress(_file_count);
}

void OpenTreeThread::RecursiveProTree(const QString &src_path, int &file_count, QTreeWidget *self, QTreeWidgetItem *root, QTreeWidgetItem *parent, QTreeWidgetItem *preitem)
{
    QDir src_dir(src_path);
    src_dir.setFilter(QDir::Dirs|QDir::Files|QDir::NoDotAndDotDot);
    src_dir.setSorting(QDir::Name);
    QFileInfoList list=src_dir.entryInfoList();
    for(int i=0;i<list.size();i++){
        if(_bstop){
            return;
        }
        QFileInfo fileInfo=list.at(i);
        bool bIsDir=fileInfo.isDir();
        if(bIsDir){
            if(_bstop){
                return;
            }
            file_count++;
            emit SignUpdateProgress(file_count);
            auto * item=new ProTreeItem(parent,fileInfo.fileName(),fileInfo.absoluteFilePath(),
                                         _root,TreeItemDir);
            item->setData(0,Qt::DisplayRole,fileInfo.fileName());
            item->setData(0,Qt::DecorationRole,QIcon(":/icon/dir.png"));
            item->setData(0,Qt::ToolTipRole,fileInfo.absoluteFilePath());
            RecursiveProTree(fileInfo.absoluteFilePath(),file_count,
                             self,root,item,preitem);
        }
        else {
            if(_bstop){
                return;
            }
            const QString& suffix=fileInfo.completeSuffix();
            if(suffix!="png"&&suffix!="jpeg"&&suffix!="jpg"){
                continue;
            }
            file_count++;
            emit SignUpdateProgress(file_count);
            auto * item=new ProTreeItem(parent,fileInfo.fileName(),fileInfo.absoluteFilePath(),
                                         _root,TreeItemPic);
            item->setData(0,Qt::DisplayRole,fileInfo.fileName());
            item->setData(0,Qt::DecorationRole,QIcon(":/icon/pic.png"));
            item->setData(0,Qt::ToolTipRole,fileInfo.absoluteFilePath());
            if(preitem){
                auto * pre_proitem=dynamic_cast<ProTreeItem* >(preitem);
                pre_proitem->SetNextItem(item);
            }
            item->SetPreItem(preitem);
            preitem=item;
        }
    }
    // emit SignFinishProgress(file_count);
}

void OpenTreeThread::SlotCancelProgress()
{
    this->_bstop=true;
}
