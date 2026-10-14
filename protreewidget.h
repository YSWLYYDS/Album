#ifndef PROTREEWIDGET_H
#define PROTREEWIDGET_H

#include <QTreeWidget>
#include <QAction>
#include <QProgressDialog>
#include <QThread>
#include <QHash>
#include "scanworker.h"
#include "importworker.h"
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QList>
#include <QUrl>
class SlideShowDlg;//前项声明，避免互引用
class ProTreeItem;

class ProTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    ProTreeWidget(QWidget * parent=nullptr);
    void AddProToTree(const QString&name,const QString &path);//新建项目落树
private:
    QSet<QString> _set_path;//判断重名
    QTreeWidgetItem * _right_btn_item;//右键点中的那个项目
    QTreeWidgetItem * _active_item;//"活动项目"
    QTreeWidgetItem * _selected_item;//当前正在大图显示的图片节点
    QAction * _action_import;//导入
    QAction * _action_setstart;//设置活动项目
    QAction * _action_closepro;//关闭项目
    QAction * _action_slideshow;//轮播图播放
    QProgressDialog * _dialog_progress;//导入文件夹时的进度条
    QProgressDialog * _open_progressdlg;//打开时的进度条
    QThread * _import_thread;//导入功能的执行环境线程
    ImportWorker * _import_worker;//导入工人：moveToThread 后其槽函数在子线程执行
    QThread * _scan_thread;//线程对象本身不干活，只提供执行环境
    ScanWorker * _scan_worker;//工人：moveToThread 后其槽函数都在子线程执行
    ProTreeItem * _scan_root;//本次扫描顶层项目节点
    QHash<int, ProTreeItem*> _id_to_item;//扫描/导入期间的“编号→节点”查表
    std::shared_ptr<SlideShowDlg>_slide_show_dlg;//轮播图窗口的智能指针
    QMediaPlayer * _player;//只负责播放控制
    QAudioOutput * _audio;//音量与音频输出必须单独绑定
    QList<QUrl> _music_list;//替代QMediaPlaylist
    int _music_index;
    void LoadAndPlayMusic(const QString& pro_path);//扫描 music 文件夹并自动播放
private slots:
    void SlotItemPressed(QTreeWidgetItem *pressedItem, int column);//右键菜单
    void SlotDoubleClickItem(QTreeWidgetItem *doubleItem, int col);//双击看图
    void SlotImprot();//导入项目
    void SlotSetActive();
    void SlotClosePro();
    void SlotUpdateProgress(int count);
    void SlotCancelProgress();
    void SlotImportBatchReady(const QList<NodeInfo> &items);//收导入节点数据，主线程建树
    void SlotImportFinished(bool cancelled);//导入结束：统一清理对话框与线程
    void SlotUpdateOpenProgress(int count);
    void SlotFinishOpenProgress();
    void SlotCancelOpenProgress();
    void SlotSlideShow();
    void SlotMediaStatus(QMediaPlayer::MediaStatus status);//一首播完自动切下一首
    void SlotBatchReady(const QList<NodeInfo> &items);//收批量节点数据，在主线程建树
    void SlotScanFinished(bool cancelled);//扫描结束：取消则清树，正常则收尾；两种情况都要收线程
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
    void SignStartScan(const QString & path);//指挥 worker 开工：必须走信号（队列连接）才会让 SlotStart 在子线程执行
    void SignStartImport(const QString & src_path, const QString & dist_path);//指挥导入 worker 开工
};

#endif // PROTREEWIDGET_H
