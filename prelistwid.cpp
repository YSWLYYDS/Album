#include "prelistwid.h"
#include "ProTreeItem.h"
#include "const.h"
#include "QPainter"
#include <QPixmap>
#include <qguiapplication.h>
#include "prelistitem.h"
#include "thumbcache.h"
#include "thumbnailtask.h"
#include <QThreadPool>
PreListWid::PreListWid(QWidget* parent):QListWidget(parent),_global(0),_last_index(17),_generation(0)
{
    this->setViewMode(QListWidget::IconMode);
    this->setIconSize(QSize(PREICON_SIZE,PREICON_SIZE));
    this->setSpacing(5);
    connect(this,&PreListWid::itemPressed,this,&PreListWid::SlotItemPressed);
}

PreListWid::~PreListWid()
{

}

void PreListWid::AddListItem(const QString &path)
{
    _global++;

    // 1. 先查 LRU 缓存：命中就直接用现成图，不再开子线程（重开项目时秒出真图）
    QPixmap cached = ThumbCache::Instance().Get(path);
    QPixmap icon;
    if(!cached.isNull()){
        icon = MakeIconPixmap(cached);
    }else{
        // 2. 未命中：先放纯灰底占位图，列表立刻能显示，不卡界面
        //    （旧代码在这里主线程同步 QPixmap(path) 解码原图，大图会卡住轮播窗口）
        icon = MakeIconPixmap(QPixmap());
        // 3. 解码任务扔进线程池子线程执行，完成后经信号回主线程更新图标。
        //    _generation 是提交时刻的批次号快照，结果回来用于过期判定
        auto *task = new ThumbnailTask(path, PREICON_SIZE, _generation);
        connect(task, &ThumbnailTask::sigThumbReady, this, &PreListWid::SlotThumbReady,
                Qt::QueuedConnection);
        QThreadPool::globalInstance()->start(task);
    }

    PreListItem * pItem=new PreListItem(QIcon(icon),path,_global,this);
    pItem->setSizeHint(QSize(PREITEM_SIZE,PREITEM_SIZE));
    this->addItem(pItem);
    _set_items[path]=pItem;
    if(_global==1){
        _pos_origin=this->pos();
    }
}

QPixmap PreListWid::MakeIconPixmap(const QPixmap &src) const
{
    // 把任意尺寸的图等比缩放后合成到 90x90 灰底格子中间；
    // src 为空时返回纯灰底，作为列表项的占位图
    QPixmap src_pixmap = src.scaled(PREICON_SIZE,PREICON_SIZE,Qt::KeepAspectRatio);
    QPixmap dist_pixmap(QSize(PREICON_SIZE,PREICON_SIZE));
    dist_pixmap.fill(QColor(220,220,220,50));
    if(!src_pixmap.isNull()){
        QPainter painter(&dist_pixmap);
        auto x=(dist_pixmap.width()-src_pixmap.width())/2;
        auto y=(dist_pixmap.height()-src_pixmap.height())/2;
        painter.drawPixmap(x,y,src_pixmap);
    }
    return dist_pixmap;
}

void PreListWid::SlotThumbReady(int gen, const QString &path, const QImage &img)
{
    // 1. 批次号对不上：这是清空列表之前提交的过期结果，直接丢弃（generation 竞态防护）
    if(gen != _generation){
        return;
    }
    // 2. 列表里已找不到这个路径：项已被删除，不更新
    auto iter=_set_items.find(path);
    if(iter==_set_items.end()){
        return;
    }
    // 3. QImage 转回 QPixmap（QPixmap 只能在主线程创建和使用）
    QPixmap pix = QPixmap::fromImage(img);
    // 4. 存入 LRU，下次打开同一项目直接命中，不再解码
    ThumbCache::Instance().Put(path, pix);
    // 5. 把灰块占位换成真图
    iter.value()->setIcon(QIcon(MakeIconPixmap(pix)));
}

void PreListWid::ClearList()
{
    this->clear();
    _set_items.clear();
    // 批次号 +1：之前提交、还没回来的缩略图结果会因批次号不匹配被全部丢弃。
    // 当前流程里 PreListWid 每次轮播都是全新对象、列表只增不清，
    // 所以今天它不会被调到——留作将来列表复用时的保险丝
    ++_generation;
}



void PreListWid::SlotUpPreList(QTreeWidgetItem *tree_item)
{
    if(!tree_item){
        qDebug()<<"tree_item is empty"<<Qt::endl;
        return;
    }

    auto * pro_item=dynamic_cast<ProTreeItem*>(tree_item);
    auto path=pro_item->GetPath();
    auto iter=_set_items.find(path);
    if(iter!=_set_items.end()){
        qDebug()<<"path"<<path<<" exists"<<Qt::endl;
        return;
    }

    AddListItem(path);


}

void PreListWid::SlotUpSelect(QTreeWidgetItem *tree_item)
{
    if(!tree_item)
    {
        return;
    }
    auto * pro_item=dynamic_cast<ProTreeItem*>(tree_item);
    auto path = pro_item->GetPath();
    auto iter=_set_items.find(path);
    if(iter==_set_items.end()){
        return;
    }

    auto * list_item=dynamic_cast<PreListItem*>(iter.value());
    auto index=list_item->GetIndex();
    if(index > 17)
    {
        auto pos_cur=this->pos();
        this->move(pos_cur.x()-(index-_last_index)*100,pos_cur.y());
        _last_index = index;
    }else {
        this->move(_pos_origin);
        _last_index=17;
    }

    this->setCurrentItem(iter.value());
}

void PreListWid::SlotItemPressed(QListWidgetItem *item)
{
    if(QGuiApplication::mouseButtons()!=Qt::LeftButton){
        return;
    }

    auto * list_item=dynamic_cast<PreListItem*>(item);
    auto cur_index=list_item->GetIndex();
    auto path=list_item->GetPath();
    this->setCurrentItem(item);
    emit SigUpSelectShow(path);
}
