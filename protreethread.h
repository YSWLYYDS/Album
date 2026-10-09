#ifndef PROTREETHREAD_H
#define PROTREETHREAD_H
#include <QTreeWidget>
#include <QThread>

class ProTreeThread : public QThread
{
    Q_OBJECT
public:
    ProTreeThread(const QString &src_path,const QString& dist_path,QTreeWidgetItem* parent_item,
int file_count,QTreeWidget* self,QTreeWidgetItem* root,QObject* parent=nullptr);
    ~ProTreeThread();
protected:
    virtual void run();

private:
    void CreateProTree(const QString& src_path,const QString& dist_path,QTreeWidgetItem* parent_item,
                       int &file_count,QTreeWidget* self,QTreeWidgetItem* root, QTreeWidgetItem* preItem = nullptr);

    QString _src_path;
    QString _dist_path;
    int _file_count;
    QTreeWidgetItem* _parent_item;
    QTreeWidget* _self;
    QTreeWidgetItem * _root;
    bool _bstop;//控制线程退出
public slots:
    void SlotCancelProgress();
signals:
    void SignUpdateProgress(int);
    void SignFinishProgress(int);
};

#endif // PROTREETHREAD_H
