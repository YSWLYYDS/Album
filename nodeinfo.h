#ifndef NODEINFO_H
#define NODEINFO_H
#include <QString>
#include <QMetaType>

struct NodeInfo
{
    int id=0;//本节点编号
    int parentId=0;//父节点编号
    int prePicId=0;//上一张图片的编号
    QString name;//显示名
    QString path;//绝对路径
    int type=0;
};
Q_DECLARE_METATYPE(NodeInfo);//Qt 的信号槽在 跨线程 时，参数不是直接传过去的，
//而是先打包成`QVariant` 投递给主线程的事件队列，到了主线程再拆包。
#endif // NODEINFO_H
