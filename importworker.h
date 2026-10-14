#ifndef IMPORTWORKER_H
#define IMPORTWORKER_H

#include <QObject>
#include <QString>
#include <QList>
#include <atomic>
#include "nodeinfo.h"

// 导入工人：与 ScanWorker 同一套 Worker Object 模式。
// 职责：把源文件夹的图片拷贝进项目文件夹 + 递归收集节点信息；
// 自己绝不碰任何 UI，建树由主线程收到 NodeInfo 后完成。
// 使用约束：必须 new 出来且不带 parent（否则 moveToThread 失效）。
class ImportWorker : public QObject
{
    Q_OBJECT
public:
    explicit ImportWorker(QObject *parent=nullptr);
public slots:
    void SlotStart(const QString &src_path, const QString &dist_path);//干活入口（在子线程执行）
    void SlotCancel();//协作式取消：只置标记，绝不强杀线程
signals:
    void sigBatchReady(const QList<NodeInfo> &items);//攒 200 个发一批，减少跨线程信号次数
    void sigProgress(int count);//每 50 个发一次进度
    void sigFinished(bool cancelled);//只在全部结束时发一次；递归内部禁止发完成信号
private:
    void ScanDir(const QString &src_dir, const QString &dist_dir, int parentId, int &prePicId);
    void Enqueue(const NodeInfo &node);
    std::atomic_bool _cancel{false};//跨线程开关量必须 atomic，普通 bool 是未定义行为
    int _next_id;
    int _count;
    QList<NodeInfo> _buffer;
};

#endif // IMPORTWORKER_H
