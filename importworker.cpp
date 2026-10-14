#include "importworker.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include "const.h"

ImportWorker::ImportWorker(QObject *parent)
    :QObject(parent),_next_id(1),_count(0)
{

}

void ImportWorker::Enqueue(const NodeInfo &node)
{
    _buffer.append(node);
    ++_count;
    if(_count % 50 == 0){
        emit sigProgress(_count);
    }
    if(_buffer.size() >= 200){
        emit sigBatchReady(_buffer);
        _buffer.clear();
    }
}

void ImportWorker::ScanDir(const QString &src_dir, const QString &dist_dir, int parentId, int &prePicId)
{
    //prePicId 必须引用传递：图片链要跨目录连续串下去。
    //旧代码把 QTreeWidgetItem* preItem 按值传，子目录递归返回后外层拿不到更新，链会断
    QDir import_dir(src_dir);
    import_dir.setFilter(QDir::Dirs|QDir::Files|QDir::NoDotAndDotDot);//保留目录和文件
    import_dir.setSorting(QDir::Name);//按名字排序，保证建树顺序稳定
    const QFileInfoList list=import_dir.entryInfoList();

    for(const QFileInfo &fileInfo : list){
        //取消检查点只放循环开头：目录/文件两个分支都能快速退出，无需重复写
        if(_cancel.load()){
            return;
        }
        if(fileInfo.isDir()){
            //把源子文件夹镜像到项目文件夹（不存在则创建）
            QString sub_dist_path=QDir(dist_dir).absoluteFilePath(fileInfo.fileName());
            if(!QDir(sub_dist_path).exists()){
                if(!QDir().mkpath(sub_dist_path)){
                    continue;//创建失败就跳过这棵子树
                }
            }
            NodeInfo node;
            node.id=_next_id++;
            node.parentId=parentId;
            node.prePicId=0;//目录不参与图片轮播链
            node.name=fileInfo.fileName();
            node.path=sub_dist_path;
            node.type=TreeItemDir;
            Enqueue(node);
            //递归：传子目录 id 当父 id，prePicId 继续用同一个引用往下传
            ScanDir(fileInfo.absoluteFilePath(),sub_dist_path,node.id,prePicId);
        }
        else{
            const QString suffix=fileInfo.completeSuffix();//完整后缀
            if(suffix!="png"&&suffix!="jpeg"&&suffix!="jpg"){
                continue;
            }
            QString dist_file_path=QDir(dist_dir).absoluteFilePath(fileInfo.fileName());
            //源和目标同一路径（把项目文件夹导入自己）时跳过拷贝，只补建树
            if(fileInfo.absoluteFilePath()!=dist_file_path){
                if(!QFile::copy(fileInfo.absoluteFilePath(),dist_file_path)){
                    continue;//拷贝失败（如同名文件已存在）：与旧代码一致，不加入树
                }
            }
            NodeInfo node;
            node.id=_next_id++;
            node.parentId=parentId;
            node.prePicId=prePicId;//轮播链：我接在谁后面
            node.name=fileInfo.fileName();
            node.path=dist_file_path;//树里存拷贝后的路径，不是源路径
            node.type=TreeItemPic;
            Enqueue(node);
            prePicId=node.id;//当前图入队后，它就是下一张图的"上一张"（引用逐层带出去）
        }
    }
}

void ImportWorker::SlotStart(const QString &src_path, const QString &dist_path)
{
    //每次开工先把状态归零（worker 对象理论上可能复用）
    _cancel=false;
    _next_id=1;
    _count=0;
    _buffer.clear();

    //parentId==0 的顶层项由主线程挂到项目节点下，worker 不需要知道任何界面信息
    int prePicId=0;
    ScanDir(src_path,dist_path,0,prePicId);

    if(!_buffer.isEmpty()){
        emit sigBatchReady(_buffer);//把不足 200 的尾巴冲出去
        _buffer.clear();
    }
    //完成信号只在收尾发一次：递归内部发会导致主线程提前收尾、漏收后面的批次
    emit sigFinished(_cancel.load());
}

void ImportWorker::SlotCancel()
{
    //协作式取消：只置标记，让递归在自己的检查点自然退出。
    //QThread::terminate() 强杀会内存泄漏/死锁/随机崩溃，禁止使用
    _cancel=true;
}
