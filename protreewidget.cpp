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
    _dialog_progress(nullptr),_selected_item(nullptr),_thread_create_pro(nullptr),_thread_open_pro(nullptr),
    _player(nullptr),_audio(nullptr),_music_index(0)
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

    //Qt6：播放控制与音频输出分离，必须显式绑定，否则没有声音
    _player=new QMediaPlayer(this);
    _audio=new QAudioOutput(this);
    _player->setAudioOutput(_audio);
    connect(_player,&QMediaPlayer::mediaStatusChanged,this,&ProTreeWidget::SlotMediaStatus);
    //排查“没有声音”：打印播放链路的状态变化
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
    int file_count=0;
    _dialog_progress=new QProgressDialog(this);
    _thread_create_pro=std::make_shared<ProTreeThread>(std::ref(import_path),std::ref(path),_right_btn_item,
file_count,this,_right_btn_item,nullptr);

    connect(_thread_create_pro.get(),&ProTreeThread::SignUpdateProgress,
            this,&ProTreeWidget::SlotUpdateProgress);
    connect(_thread_create_pro.get(),&ProTreeThread::SignFinishProgress,
            this,&ProTreeWidget::SlotFinishProgress);
    connect(_dialog_progress,&QProgressDialog::canceled,this,&ProTreeWidget::SlotCancelProgress);
    connect(this,&ProTreeWidget::SignCancelProgress,_thread_create_pro.get(),&ProTreeThread::SlotCancelProgress);
    _thread_create_pro->start();
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
        _active_item->setFont(0,nullFont);
    }
    _active_item=_right_btn_item;
    nullFont.setBold(true);
    _active_item->setFont(0,nullFont);
}

void ProTreeWidget::SlotClosePro()
{
    RemoveProDialog remove_pro_dialog;
    auto res=remove_pro_dialog.exec();
    if(res!=QDialog::Accepted){
        return;
    }
    bool b_remove=remove_pro_dialog.IsRemoved();
    auto index_right_btn=this->indexOfTopLevelItem(_right_btn_item);
    auto * protreeitem=dynamic_cast<ProTreeItem*>(_right_btn_item);
    auto * selecteditem=dynamic_cast<ProTreeItem*>(_selected_item);
    auto delete_path=protreeitem->GetPath();
    _set_path.remove(delete_path);
    if(b_remove){
        QDir delete_dir(delete_path);
        delete_dir.removeRecursively();
    }
    if(protreeitem==_active_item){
        _active_item=nullptr;
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

void ProTreeWidget::SlotFinishProgress()
{
    if(!_dialog_progress){
        return;
    }
    _dialog_progress->setValue(PROGRESS_MAX);
    _dialog_progress->deleteLater();
    _dialog_progress=nullptr;
}

void ProTreeWidget::SlotCancelProgress()
{
    emit SignCancelProgress();
    delete _dialog_progress;
    _dialog_progress=nullptr;
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
    //影子构建时 exe 在 build/.../debug 下，从多个起点逐级向上查找 music 文件夹
    QStringList start_dirs;
    start_dirs<<QCoreApplication::applicationDirPath()
             <<QDir::currentPath()
             <<pro_path;

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
        qDebug()<<"[music] folder not found. exe dir:"
               <<QCoreApplication::applicationDirPath()
               <<" work dir:"<<QDir::currentPath();
        return;
    }

    QDir music_dir(music_dir_path);
    //music 文件夹里目前是 flac，mp3/flac/wav 都纳入扫描
    QStringList filters;
    filters<<"*.mp3"<<"*.flac"<<"*.wav";
    auto files=music_dir.entryList(filters,QDir::Files,QDir::Name);
    if(files.isEmpty()){
        qDebug()<<"[music] no audio file in"<<music_dir_path;
        return;
    }

    _music_list.clear();
    for(const auto &file : files){
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
    _player->setSource(_music_list[_music_index]);
    _player->play();
}

void ProTreeWidget::SlotOpenPro(const QString &path)
{
    if(_set_path.find(path)!=_set_path.end()){
        return;
    }

    _set_path.insert(path);
    int file_count=0;
    QDir pro_dir(path);
    QString proname=pro_dir.dirName();

    _thread_open_pro=std::make_shared<OpenTreeThread>(path,file_count,this,nullptr);

    _open_progressdlg=new QProgressDialog(this);
    connect(_thread_open_pro.get(),&OpenTreeThread::SignUpdateProgress,
            this,&ProTreeWidget::SlotUpdateOpenProgress);

    connect(_thread_open_pro.get(),&OpenTreeThread::SignFinishProgress,
            this,&ProTreeWidget::SlotFinishOpenProgress);

    connect(_open_progressdlg,&QProgressDialog::canceled,this,&ProTreeWidget::SlotCancelOpenProgress);

    connect(this,&ProTreeWidget::SignCancelOpenProgress,_thread_open_pro.get(),&OpenTreeThread::SlotCancelProgress);
    _thread_open_pro->start();
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
    if(status!=QMediaPlayer::EndOfMedia){
        return;
    }
    if(_music_list.isEmpty()){
        return;
    }
    _music_index=(_music_index+1)%_music_list.size();
    _player->setSource(_music_list[_music_index]);
    _player->play();
}


