#include "protreewidget.h"
#include <ProTreeItem.h>
#include <QDir.h>
#include <QDebug>
#include <QCoreApplication>
#include <QStringList>
#include <QFileInfo>
#include "const.h"
#include "QGuiApplication"
#include <QFileDialog>
#include <QMenu>
#include "removeprodialog.h"
#include "slideshowdlg.h"
ProTreeWidget::ProTreeWidget(QWidget *parent)
    : QTreeWidget(parent),_active_item(nullptr),_right_btn_item(nullptr),_open_progressdlg(nullptr),
    _dialog_progress(nullptr),_selected_item(nullptr),_import_thread(nullptr),_import_worker(nullptr),
    _player(nullptr),_audio(nullptr),_music_index(0),
    _scan_thread(nullptr),_scan_worker(nullptr),_scan_root(nullptr)
{
    this->setHeaderHidden(true);
    connect(this,&ProTreeWidget::itemPressed,this,&ProTreeWidget::SlotItemPressed);//&ProTreeWidget::itemPressed随便点击一个按钮就会触发
    _action_import=new QAction(QIcon(":/icon/import.png"),tr("导入文件"),this);
    _action_setstart=new QAction(QIcon(":/icon/core.png"),tr("设置活动项目"),this);
    _action_closepro=new QAction(QIcon(":/icon/close.png"),tr("关闭项目"),this);
    _action_slideshow=new QAction(QIcon(":/icon/slideshow.png"),tr("轮播图播放"),this);
    connect(_action_import,&QAction::triggered,this,&ProTreeWidget::SlotImprot);
    connect(_action_setstart,&QAction::triggered,this,&ProTreeWidget::SlotSetActive);
    connect(_action_closepro,&QAction::triggered,this,&ProTreeWidget::SlotClosePro);
    connect(this,&ProTreeWidget::itemDoubleClicked,this,&ProTreeWidget::SlotDoubleClickItem);
    connect(_action_slideshow,&QAction::triggered,this,&ProTreeWidget::SlotSlideShow);

    _player=new QMediaPlayer(this);//管播什么
    _audio=new QAudioOutput(this);//管声音从哪个设备出来、音量多大
    _player->setAudioOutput(_audio);
    connect(_player,&QMediaPlayer::mediaStatusChanged,this,&ProTreeWidget::SlotMediaStatus);
    //排查“没有声音”：打印播放链路的状态
    connect(_player,&QMediaPlayer::mediaStatusChanged,this,[](QMediaPlayer::MediaStatus status){
        qDebug()<<"[music] mediaStatus:"<<status;
    });
    connect(_player,&QMediaPlayer::playbackStateChanged,this,[](QMediaPlayer::PlaybackState state){
        qDebug()<<"[music] playbackState:"<<state;
    });
    connect(_player,&QMediaPlayer::errorOccurred,this,[](QMediaPlayer::Error error,const QString &errorString){
        qDebug()<<"[music] error:"<<error<<errorString;
    });
}

void ProTreeWidget::AddProToTree(const QString &name, const QString &path)
{
    QDir dir(path);
    QString file_path=dir.absoluteFilePath(name);//将文件夹中的名字拼成一个新的路径
    if(_set_path.find(file_path)!=_set_path.end())//没有到尾就找到了，说明重名了
    {
        return;
    }

    QDir pro_dir(file_path);//创建一个文件夹
    if(!pro_dir.exists()){
        bool enable=pro_dir.mkpath(file_path);//二级目录不存在也会创建这个文件夹
        if(!enable)
        {
            return;
        }
    }
    _set_path.insert(file_path);
    auto * item=new ProTreeItem(this,name,file_path,TreeItemPro);
    item->setData(0,Qt::DisplayRole,name);//Qt::DisplayRole展示的角色
    item->setData(0,Qt::DecorationRole,QIcon(":/icon/dir.png"));//Qt::DecorationRole图片
    item->setData(0,Qt::ToolTipRole,file_path);//鼠标悬浮上去会提示文件路径
    this->addTopLevelItem(item);

}

void ProTreeWidget::SlotItemPressed(QTreeWidgetItem *pressedItem, int column)
{
    if(QGuiApplication::mouseButtons()==Qt::RightButton)//右键弹出菜单
    {
        QMenu menu(this);
        int itemtype =pressedItem->type();
        if(itemtype==TreeItemPro)//判断项目类型
        {
            _right_btn_item=pressedItem;
            menu.addAction(_action_import);
            menu.addAction(_action_setstart);
            menu.addAction(_action_closepro);
            menu.addAction(_action_slideshow);
            menu.exec(QCursor::pos());//弹出菜单在鼠标的位置
        }

    }

}

void ProTreeWidget::SlotDoubleClickItem(QTreeWidgetItem *doubleItem, int col)
{
    if(QGuiApplication::mouseButtons()==Qt::LeftButton){
        auto * tree_doubleItem=dynamic_cast<ProTreeItem*>(doubleItem);
        if(!tree_doubleItem){
            return;
        }
        int itemtype=tree_doubleItem->type();
        if(itemtype==TreeItemPic){
            emit SignUpdateSelected(tree_doubleItem->GetPath());
            _selected_item=doubleItem;
        }
    }
}

void ProTreeWidget::SlotImprot()
{
    if(_import_thread){
        return;//上一次导入还没收尾，避免重入覆盖线程/工人指针
    }
    QFileDialog file_dialog;
    file_dialog.setFileMode(QFileDialog::Directory);//选择文件夹
    file_dialog.setWindowTitle(tr("选择导入的文件夹"));
    QString path="";
    if(!_right_btn_item){
        qDebug()<<"_right_btn_item is empty"<<Qt::endl;
        return;
    }
    path=dynamic_cast<ProTreeItem*>(_right_btn_item)->GetPath();
    file_dialog.setDirectory(path);
    file_dialog.setViewMode(QFileDialog::Detail);

    QStringList fileNames;
    if(file_dialog.exec())
    {
        fileNames=file_dialog.selectedFiles();//用这个变量缓存选中的文件
    }

    if(fileNames.length()<=0)
    {
        return;
    }

    QString import_path=fileNames.at(0);
    _dialog_progress=new QProgressDialog(this);

    //导入与打开项目统一采用 Worker Object 模式：worker 不带 parent，
    //moveToThread 后其槽函数经队列信号在子线程执行，子线程绝不直接碰 QTreeWidget
    _import_thread=new QThread(this);
    _import_worker=new ImportWorker();//不能带 parent，否则无法 moveToThread
    _import_worker->moveToThread(_import_thread);

    connect(_import_worker,&ImportWorker::sigBatchReady,this,&ProTreeWidget::SlotImportBatchReady);
    connect(_import_worker,&ImportWorker::sigProgress,this,&ProTreeWidget::SlotUpdateProgress);
    connect(_import_worker,&ImportWorker::sigFinished,this,&ProTreeWidget::SlotImportFinished);
    connect(this,&ProTreeWidget::SignStartImport,_import_worker,&ImportWorker::SlotStart);
    connect(_dialog_progress,&QProgressDialog::canceled,this,&ProTreeWidget::SlotCancelProgress);
    connect(this,&ProTreeWidget::SignCancelProgress,_import_worker,&ImportWorker::SlotCancel);

    _import_thread->start();
    emit SignStartImport(import_path,path);//队列连接：SlotStart 排进子线程事件循环执行
    _dialog_progress->setWindowTitle("Please wait...");
    _dialog_progress->setFixedWidth(PROGRESS_WIDTH);
    _dialog_progress->setRange(0,PROGRESS_WIDTH);
    _dialog_progress->exec();
}

void ProTreeWidget::SlotSetActive()
{
    if(!_right_btn_item){
        return;
    }
    QFont nullFont;
    nullFont.setBold(false);
    if(_active_item){
        _active_item->setFont(0,nullFont);//旧活动项目取消加粗
    }
    _active_item=_right_btn_item;
    nullFont.setBold(true);
    _active_item->setFont(0,nullFont);//新活动项目加粗
}

void ProTreeWidget::SlotClosePro()
{
    RemoveProDialog remove_pro_dialog;//自定义确认框
    auto res=remove_pro_dialog.exec();
    if(res!=QDialog::Accepted){
        return;
    }
    bool b_remove=remove_pro_dialog.IsRemoved();
    auto index_right_btn=this->indexOfTopLevelItem(_right_btn_item);//项目节点在顶层的下标
    auto * protreeitem=dynamic_cast<ProTreeItem*>(_right_btn_item);//右键的项目节点
    auto * selecteditem=dynamic_cast<ProTreeItem*>(_selected_item);//当前正在显示的图片节点
    auto delete_path=protreeitem->GetPath();//项目完整路径
    _set_path.remove(delete_path);
    if(b_remove){
        QDir delete_dir(delete_path);
        delete_dir.removeRecursively();// 递归删除文件夹及全部内容
    }
    if(protreeitem==_active_item){
        _active_item=nullptr;//指针悬空
    }

    if(selecteditem&&protreeitem==selecteditem->GetRoot()){
        selecteditem=nullptr;
        _selected_item=nullptr;
        emit SigClearSelected();
    }
    delete this -> takeTopLevelItem (index_right_btn);
    _right_btn_item = nullptr ;
    emit SigClosePro () ;

}

void ProTreeWidget::SlotUpdateProgress(int count)
{
    if(!_dialog_progress){
        return;
    }
    if(count>=PROGRESS_MAX){
        _dialog_progress->setValue(count%PROGRESS_MAX);
    }else{
        _dialog_progress->setValue(count);
    }
}

void ProTreeWidget::SlotCancelProgress()
{
    //只发取消标记；对话框和线程的清理统一放到 SlotImportFinished，
    //避免在对话框自己的 canceled 处理里直接 delete 它（exec 栈还没退出的 UB 风险）
    emit SignCancelProgress();
}

void ProTreeWidget::SlotImportBatchReady(const QList<NodeInfo> &items)
{
    //防御：导入途中项目节点被关闭（已不在树上）就不再建树
    if(!_right_btn_item || this->indexOfTopLevelItem(_right_btn_item)<0){
        return;
    }
    for(const NodeInfo &info : items){
        ProTreeItem *parent=nullptr;
        if(info.parentId==0){
            //导入的顶层项挂到项目节点下（项目节点本身不重建）
            parent=dynamic_cast<ProTreeItem*>(_right_btn_item);
        }else{
            parent=_id_to_item.value(info.parentId,nullptr);
            if(!parent){
                continue;//防御：父节点缺失（理论上不会发生）就跳过，不让程序崩
            }
        }
        auto *item=new ProTreeItem(parent,info.name,info.path,_right_btn_item,info.type);
        item->setData(0,Qt::DisplayRole,info.name);
        item->setData(0,Qt::DecorationRole,QIcon(info.type==TreeItemPic?":/icon/pic.png":":/icon/dir.png"));
        item->setData(0,Qt::ToolTipRole,info.path);
        if(info.type==TreeItemPic){
            //串上一张/下一张链表：逻辑与打开项目完全一致，只是数据来源换成 _id_to_item 查表
            auto *pre=_id_to_item.value(info.prePicId,nullptr);
            if(pre){
                pre->SetNextItem(item);
            }
            item->SetPreItem(pre);
        }
        _id_to_item[info.id]=item;
    }
}

void ProTreeWidget::SlotImportFinished(bool cancelled)
{
    if(cancelled){
        //取消：保留已导入的部分（已拷贝的文件和树节点都不删）。
        //旧线程 run() 取消时 removeRecursively 会连用户项目一起删掉，属于危险行为，已移除
        qDebug()<<"[import] cancelled, partial import kept";
    }
    if(_dialog_progress){
        _dialog_progress->deleteLater();//必须 deleteLater：此刻对话框可能还挂在 exec() 栈上
        _dialog_progress=nullptr;
    }
    _id_to_item.clear();
    if(_import_thread){
        _import_thread->quit();//先让子线程事件循环退出
        _import_thread->wait();//再等它真正结束，之后才能安全 delete
        delete _import_worker;  _import_worker=nullptr;
        delete _import_thread;  _import_thread=nullptr;
    }
}

void ProTreeWidget::SlotUpdateOpenProgress(int count)
{
    if(!_open_progressdlg){
        return;
    }
    if(count>=PROGRESS_MAX){
        _open_progressdlg->setValue(count%PROGRESS_MAX);
    }else{
        _open_progressdlg->setValue(count);
    }
}

void ProTreeWidget::SlotFinishOpenProgress()
{
    if(!_open_progressdlg){
        return;
    }
    _open_progressdlg->setValue(PROGRESS_MAX);
    delete _open_progressdlg;
    _open_progressdlg=nullptr;
}

void ProTreeWidget::SlotCancelOpenProgress()
{
    emit SignCancelOpenProgress();
    delete _open_progressdlg;
    _open_progressdlg=nullptr;
}

void ProTreeWidget::SlotBatchReady(const QList<NodeInfo> &items)
{
    for(const NodeInfo &info : items){
        if(info.parentId==0){
            //根节点：parentId==0，挂到 QTreeWidget 顶层
            _scan_root=new ProTreeItem(this,info.name,info.path,info.type);
            _scan_root->setData(0,Qt::DisplayRole,info.name);
            _scan_root->setData(0,Qt::DecorationRole,QIcon(":/icon/dir.png"));
            _scan_root->setData(0,Qt::ToolTipRole,info.path);
            _id_to_item[info.id]=_scan_root;
            continue;
        }
        auto *parent=_id_to_item.value(info.parentId,nullptr);
        if(!parent){
            continue;//防御：父节点缺失（理论上不会发生）就跳过，不让程序崩
        }
        auto *item=new ProTreeItem(parent,info.name,info.path,_scan_root,info.type);
        item->setData(0,Qt::DisplayRole,info.name);
        item->setData(0,Qt::DecorationRole,QIcon(info.type==TreeItemPic?":/icon/pic.png":":/icon/dir.png"));
        item->setData(0,Qt::ToolTipRole,info.path);
        if(info.type==TreeItemPic){
            //串上一张/下一张链表：和旧代码一致，只是上一张的来源从局部指针换成 _id_to_item 查表
            auto *pre=_id_to_item.value(info.prePicId,nullptr);
            if(pre){
                pre->SetNextItem(item);
            }
            item->SetPreItem(pre);
        }
        _id_to_item[info.id]=item;
    }
}

void ProTreeWidget::SlotScanFinished(bool cancelled)
{
    if(cancelled&&_scan_root){
        //只删树节点，绝不碰磁盘（旧 run() 里 removeRecursively 会删用户项目，是危险行为）
        int index=this->indexOfTopLevelItem(_scan_root);
        if(index>=0){
            delete this->takeTopLevelItem(index);//先摘下来再 delete，避免悬空指针
        }
        _set_path.remove(_scan_root->GetPath());//允许下次重新打开同一个路径
    }
    _id_to_item.clear();
    _scan_root=nullptr;
    SlotFinishOpenProgress();//收尾进度框（内部判空，取消路径下对话框已被删，安全返回）
    if(_scan_thread){
        _scan_thread->quit();//先让子线程事件循环退出
        _scan_thread->wait();//再等它真正结束，之后才能安全 delete
        delete _scan_worker;  _scan_worker=nullptr;
        delete _scan_thread;  _scan_thread=nullptr;
    }
}

void ProTreeWidget::SlotSlideShow()
{
    if(!_right_btn_item){
        return;
    }
    auto * right_pro_item=dynamic_cast<ProTreeItem*>(_right_btn_item);
    auto * last_child_item=right_pro_item->GetLastPicChild();
    if(!last_child_item){
        return;
    }
    auto * first_child_item=right_pro_item->GetFirstPicChild();
    if(!first_child_item){
        return;
    }
    qDebug()<<"first_child_item is"<<first_child_item->GetPath();
    qDebug()<<"last_child_item is"<<last_child_item->GetPath();

    //播放轮播图前，自动加载 music 文件夹中的背景音乐
    LoadAndPlayMusic(right_pro_item->GetPath());

    _slide_show_dlg=std::make_shared<SlideShowDlg>(this,first_child_item,last_child_item);
    //关闭轮播窗口时停止背景音乐
    connect(_slide_show_dlg.get(),&QDialog::finished,this,[this](int){
        _player->stop();
    });
    _slide_show_dlg->setModal(true);
    _slide_show_dlg->showMaximized();
}

void ProTreeWidget::LoadAndPlayMusic(const QString &pro_path)
{
    QStringList start_dirs;
    start_dirs<<QCoreApplication::applicationDirPath()
             <<QDir::currentPath()
             <<pro_path;//搜索exe所在目录、工作目录、项目目录

    QString music_dir_path;
    for(const auto &start : start_dirs){
        QDir dir(start);
        for(int level=0; level<8 && dir.exists(); ++level){
            if(dir.exists("music")){
                music_dir_path=dir.filePath("music");
                break;
            }
            if(!dir.cdUp()){
                break;
            }
        }
        if(!music_dir_path.isEmpty()){
            break;
        }
    }

    if(music_dir_path.isEmpty()){
        qDebug()<<"music folder not found. exe dir:"
               <<QCoreApplication::applicationDirPath()
               <<" work dir:"<<QDir::currentPath();
        return;
    }

    QDir music_dir(music_dir_path);
    QStringList filters;
    filters<<"*.mp3"<<"*.flac"<<"*.wav";
    auto files=music_dir.entryList(filters,QDir::Files,QDir::Name);
    if(files.isEmpty()){
        qDebug()<<"music no audio file in"<<music_dir_path;
        return;
    }

    _music_list.clear();
    for(const auto &file : files)//过滤坏文件，装进播放列表
    {
        QString abs_path=music_dir.absoluteFilePath(file);
        QFileInfo info(abs_path);
        //跳过不存在或空文件，避免加载后无声
        if(!info.exists()||info.size()==0){
            qDebug()<<"[music] skip invalid file:"<<abs_path;
            continue;
        }
        _music_list.append(QUrl::fromLocalFile(abs_path));
    }
    if(_music_list.isEmpty()){
        return;
    }
    qDebug()<<"[music] load from"<<music_dir_path<<"count:"<<_music_list.size();
    qDebug()<<"[music] first file:"<<_music_list.first()<<"volume:"<<_audio->volume();

    _music_index=0;
    _player->setSource(_music_list[_music_index]);//告诉播放器播哪个
    _player->play();
}

void ProTreeWidget::SlotOpenPro(const QString &path)
{
    if(_scan_thread){
        return;//上一次扫描还没收尾，避免重入覆盖线程/工人指针
    }
    if(_set_path.find(path)!=_set_path.end()){
        return;
    }

    _set_path.insert(path);

    _scan_thread=new QThread(this);
    _scan_worker=new ScanWorker();//不带 parent：带 parent 的 QObject 无法 moveToThread
    _scan_worker->moveToThread(_scan_thread);

    connect(_scan_worker,&ScanWorker::sigBatchReady,this,&ProTreeWidget::SlotBatchReady);
    connect(_scan_worker,&ScanWorker::sigProgress,this,&ProTreeWidget::SlotUpdateOpenProgress);
    connect(_scan_worker,&ScanWorker::sigFinished,this,&ProTreeWidget::SlotScanFinished);
    connect(this,&ProTreeWidget::SignStartScan,_scan_worker,&ScanWorker::SlotStart);
    connect(this,&ProTreeWidget::SignCancelOpenProgress,_scan_worker,&ScanWorker::SlotCancel);

    _open_progressdlg=new QProgressDialog(this);
    connect(_open_progressdlg,&QProgressDialog::canceled,this,&ProTreeWidget::SlotCancelOpenProgress);

    _scan_thread->start();
    emit SignStartScan(path);//队列连接：SlotStart 排进子线程事件循环执行
    _open_progressdlg->setWindowTitle("Please wait...");
    _open_progressdlg->setFixedWidth(PROGRESS_WIDTH);
    _open_progressdlg->setRange(0,PROGRESS_WIDTH);
    _open_progressdlg->exec();
}

void ProTreeWidget::SlotNextShow()
{
    if(!_selected_item){
        return;
    }
    auto *curItem=dynamic_cast<ProTreeItem*>(_selected_item)->GetNextItem();
    if(!curItem){
        return;
    }
    emit SigUpdatePic(curItem->GetPath());
    _selected_item=curItem;
    this->setCurrentItem(curItem);
}

void ProTreeWidget::SlotPreShow()
{
    if(!_selected_item){
        return;
    }
    auto *curItem=dynamic_cast<ProTreeItem*>(_selected_item)->GetPreItem();
    if(!curItem){
        return;
    }
    emit SigUpdatePic(curItem->GetPath());
    _selected_item=curItem;
    this->setCurrentItem(curItem);
}

void ProTreeWidget::SlotSetMusic(bool)
{
    qDebug() << "SlotSetMusic" <<Qt::endl;
    QFileDialog file_dialog;
    file_dialog.setFileMode(QFileDialog::ExistingFiles);
    file_dialog.setWindowTitle("选择背景音乐");

    file_dialog.setDirectory(QDir::currentPath());
    file_dialog.setViewMode(QFileDialog::Detail);
    file_dialog.setNameFilter("(*.mp3)");

    QStringList fileNames;
    if (file_dialog.exec()){
        fileNames = file_dialog.selectedFiles();
    }

    if(fileNames.length() <= 0){
        return;
    }

    _music_list.clear();

    for(auto filename : fileNames){
        qDebug() << "filename is " << filename << Qt::endl;
        _music_list.append(QUrl::fromLocalFile(filename));
    }

    if(_player->playbackState()!=QMediaPlayer::PlayingState)
    {
        _music_index=0;
        _player->setSource(_music_list[_music_index]);
        _player->play();
    }


}

void ProTreeWidget::SlotStartMusic()
{
    qDebug()<< "ProTreeWidget::SlotStartMusic" << Qt::endl;
    _player->play();
}

void ProTreeWidget::SlotStopMusic()
{
    qDebug()<< "ProTreeWidget::SlotStopMusic" << Qt::endl;
    _player->pause();
}

void ProTreeWidget::SlotMediaStatus(QMediaPlayer::MediaStatus status)
{
    //一首播放完毕，自动切到下一首；到列表尾部后回到第一首循环
    if(status!=QMediaPlayer::EndOfMedia)
    {
        return;
    }
    if(_music_list.isEmpty()){
        return;
    }
    _music_index=(_music_index+1)%_music_list.size();//播放完一首循环进入下一首
    _player->setSource(_music_list[_music_index]);
    _player->play();
}


