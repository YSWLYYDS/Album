#include "protree.h"
#include "ui_protree.h"

ProTree::ProTree(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ProTree)
{
    ui->setupUi(this);
    setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    ui->label_pro->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
    ui->treeWidget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
}

ProTree::~ProTree()
{
    delete ui;
}

QTreeWidget *ProTree::GetTreeWidget()
{
    return ui->treeWidget;
}

void ProTree::AddProToTree(const QString name, const QString path)
{
    ui->treeWidget->AddProToTree(name,path);
}
