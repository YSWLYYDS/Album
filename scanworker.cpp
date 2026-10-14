#include "scanworker.h"
#include <QDir>
#include <const.h>
#include <QFileInfo>
ScanWorker::ScanWorker(QObject *parent)
    : QObject{parent}
{}

void ScanWorker::Enqueue(const NodeInfo &info)
{
    _buffer.append(info);
    if(_buffer.size()>=200){//攒够约 200 个再投递：避免上万次跨线程队列事件拖慢主线程
        emit sigBatchReady(_buffer);
        _buffer.clear();
    }
}

void ScanWorker::ScanDir(const QString &dir_path, int parentId, int &prePicId)
{
    QDir dir(dir_path);
    dir.setFilter(QDir::Dirs|QDir::Files|QDir::NoDotAndDotDot);
    dir.setSorting(QDir::Name);
    auto list=dir.entryInfoList();

    for(int i=0;i<list.size();++i){
        if(_cancel){
            return;
        }
        const QFileInfo &fileInfo=list.at(i);
        if(fileInfo.isDir()){
            NodeInfo node;
            node.id=_next_id++;
            node.parentId=parentId;
            node.prePicId=0;
            node.name=fileInfo.fileName();
            node.path=fileInfo.absoluteFilePath();
            node.type=TreeItemDir;
            Enqueue(node);
            ++_count;
            if(_count%50==0){
                emit sigProgress(_count);//进度每 50 个发一次
            }

            ScanDir(fileInfo.filePath(),node.id,prePicId);
        }else
        {
            const QString& suffix=fileInfo.completeSuffix();//取出完整后缀
            if(suffix!="png"&&suffix!="jpeg"&&suffix!="jpg"){
                continue;
            }
            _count++;
            NodeInfo node;
            node.id=_next_id++;
            node.parentId=parentId;
            node.prePicId=prePicId;
            node.name=fileInfo.fileName();
            node.path=fileInfo.absoluteFilePath();
            node.type=TreeItemPic;
            Enqueue(node);
            if(_count%50==0){
                emit sigProgress(_count);//进度每 50 个发一次
            }
            prePicId=node.id;
        }
    }
}

void ScanWorker::SlotStart(const QString &src_path)
{
    _cancel=false;
    _next_id=1;
    _count=0;
    _buffer.clear();//worker 可能被复用，上次残留的批必须清掉

    NodeInfo root;
    root.id=_next_id++;
    root.parentId=0;
    root.prePicId=0;
    root.name=QDir(src_path).dirName();
    root.path=src_path;
    root.type=TreeItemPro;
    Enqueue(root);
    ++_count;

    //递归扫描
    int prePicId=0;
    ScanDir(src_path,root.id,prePicId);

    if(!_buffer.isEmpty()){//不足 200 个的尾巴冲出去，否则最后一批永远发不出去
        emit sigBatchReady(_buffer);
        _buffer.clear();
    }
    emit sigFinished(_cancel.load());
}

void ScanWorker::SlotCancel()
{
    _cancel=true;
}
