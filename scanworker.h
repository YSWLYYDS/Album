#ifndef SCANWORKER_H
#define SCANWORKER_H

#include <QObject>
#include <QString>
#include <QList>
#include <atomic>
#include "nodeinfo.h"
class ScanWorker : public QObject
{
    Q_OBJECT
public:
    explicit ScanWorker(QObject *parent = nullptr);
private:
    void ScanDir(const QString &dir_path,int parentId,int &prePicId);//打开一个文件夹，把它里面的每一个子文件夹和图片都登记成 NodeInfo 打包发走，遇到子文件夹就钻进去重复同样的事
    void Enqueue ( const NodeInfo &info) ; // 攒批发货
    QList<NodeInfo> _buffer; // 批缓冲
    std::atomic_bool _cancel{false};//取消开关
    int _next_id=1;//下一个可用的节点编号
    int _count=0;//已扫描到的节点计数
public slots:
    void SlotStart(const QString &src_path);//开始扫描
    void SlotCancel();//请求取消
signals:
    void sigBatchReady ( const QList<NodeInfo> &items) ;//攒够一批节点数据，一次性发给主线程
    void sigProgress(int count);//报进度
    void sigFinished(bool cancelled);//扫描结束
};

#endif // SCANWORKER_H
