#ifndef PROTREEITEM_H
#define PROTREEITEM_H
#include <QTreeWidget>
#include <QTreeWidgetItem>

class ProTreeItem : public QTreeWidgetItem
{
public:
    ProTreeItem(QTreeWidget* view,const QString& name,const QString& path,int type=Type);//顶层节点，挂到树 view 上
    ProTreeItem(QTreeWidgetItem* parent,const QString& name,const QString& path,
                QTreeWidgetItem *root,int type=Type);//子节点，挂到 parent 节点下
    const QString& GetPath();
    QTreeWidgetItem* GetRoot();
    void SetPreItem(QTreeWidgetItem* item);
    void SetNextItem(QTreeWidgetItem* item);
    ProTreeItem* GetPreItem();
    ProTreeItem* GetNextItem();
    ProTreeItem* GetLastPicChild();//递归找最后一张
    ProTreeItem* GetFirstPicChild();//递归找第一张，知道第一张和最后一张才知道从哪开始到哪里结束

private:
    QString _path;//这个节点对应的磁盘路径
    QString _name;//名字
    QTreeWidgetItem * _root;//我所属的顶层项目节点
    QTreeWidgetItem * _pre_item;
    QTreeWidgetItem * _next_item;
};

#endif // PROTREEITEM_H
