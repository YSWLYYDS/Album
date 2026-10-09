#ifndef PROTREEWIDGET_H
#define PROTREEWIDGET_H

#include "protreethread.h"
#include <QTreeWidget>
#include <QAction>
#include <QProgressDialog>
#include "opentreethread.h"
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QList>
#include <QUrl>
class SlideShowDlg;//前项声明，避免互引用

class ProTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    ProTreeWidget(QWidget * parent=nullptr);
    void AddProToTree(const QString&name,const QString &path);
private:
    QSet<QString> _set_path;//判断重名
    QTreeWidgetItem * _right_btn_item;
    QTreeWidgetItem * _active_item;
    QTreeWidgetItem * _selected_item;
    QAction * _action_import;//导入
    QAction * _action_setstart;//设置活动项目
    QAction * _action_closepro;
    QAction * _action_slideshow;//轮播图播放
    QProgressDialog * _dialog_progress;
    QProgressDialog * _open_progressdlg;
    std::shared_ptr<ProTreeThread> _thread_create_pro;
    std::shared_ptr<OpenTreeThread>_thread_open_pro;
    std::shared_ptr<SlideShowDlg>_slide_show_dlg;
    QMediaPlayer * _player;//Qt6：只负责播放控制
    QAudioOutput * _audio;//Qt6：音量与音频输出必须单独绑定
    QList<QUrl> _music_list;//替代已移除的 QMediaPlaylist
    int _music_index;
    void LoadAndPlayMusic(const QString& pro_path);//扫描 music 文件夹并自动播放
private slots:
    void SlotItemPressed(QTreeWidgetItem *pressedItem, int column);
    void SlotDoubleClickItem(QTreeWidgetItem *doubleItem, int col);
    void SlotImprot();
    void SlotSetActive();
    void SlotClosePro();
    void SlotUpdateProgress(int count);
    void SlotFinishProgress();
    void SlotCancelProgress();
    void SlotUpdateOpenProgress(int count);
    void SlotFinishOpenProgress();
    void SlotCancelOpenProgress();
    void SlotSlideShow();
    void SlotMediaStatus(QMediaPlayer::MediaStatus status);//一首播完自动切下一首
public slots:
    void SlotOpenPro(const QString& path);
    void SlotNextShow();
    void SlotPreShow();
    void SlotSetMusic(bool);
    void SlotStartMusic();
    void SlotStopMusic();
signals:
    void SignCancelProgress();
    void SignCancelOpenProgress();
    void SignUpdateSelected(const QString& );
    void SigUpdatePic(const QString& );
    void SigClearSelected();
    void SigClosePro () ;
};

#endif // PROTREEWIDGET_H
