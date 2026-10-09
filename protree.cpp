#include "protree.h"
#include "ui_protree.h"

ProTree::ProTree(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ProTree)
{
    ui->setupUi(this);
    this->setMinimumWidth(378);
    this->setMaximumWidth(378);
}

ProTree::~ProTree()
{
    delete ui;
}

QTreeWidget *ProTree::GetTreeWidget()
{
    return ui->treeWidget;
}

void ProTree::AddProToTree(const QString name, const QString path)//把收到的项目名和路径转发给内部的`ProTreeWidget`
{
    ui->treeWidget->AddProToTree(name,path);
}
