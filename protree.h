#ifndef PROTREE_H
#define PROTREE_H

#include <QDialog>
#include <QTreeWidget>
namespace Ui {
class ProTree;
}

class ProTree : public QDialog
{
    Q_OBJECT

public:
    explicit ProTree(QWidget *parent = nullptr);
    ~ProTree();
    QTreeWidget* GetTreeWidget();//把内部的树交出去,MainWindow 用它拿树接线

private:
    Ui::ProTree *ui;

public slots:
    void AddProToTree(const QString name,const QString path);//把收到的项目名和路径转发给内部的`ProTreeWidget`
};

#endif // PROTREE_H
