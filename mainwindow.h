#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <pthread.h>

typedef struct lrc
{
   char text[128];
   int  time;
}LRC;

//子进程（获取歌曲当前时间及进度）
extern void *getMsgfromMplayer(void *arg);

//子进程(获得时间和进度)
extern void *MyGetTimeAndBar(void *arg);

//子进程（设定进度条）
//extern void *sliderchange(void *arg);

//向mplayer发送指令
extern void SendMsgToMplayer(char *str);

//声明暂停互斥锁
extern pthread_mutex_t mutex;

//声明进度条互斥锁
extern pthread_mutex_t qingbijinjiansuo;

//上锁
void setlock();

//释放锁
void freelock();


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    //子程序进程号
    pid_t pid;

    //打印歌词子线程号
    pthread_t pth;

    //进度百分比
    int song_percent;

    //当前歌曲秒数
    int now_second;

    //当前歌曲长度
    int song_length;

    //播放模式标志
    int play_mode = 1;

    //歌曲数目
    int Number_songs = 0;

    //进度条进度
    int value;

    //当前歌手名
    QString singer;

    //当前专辑
    QString album;

    //当前歌名
    QString title;

    //歌词容器
    QList<LRC> song_lrc;

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    //获取歌曲列表
    void setsonglist();
    //关闭子进程
    void closeEvent(QCloseEvent *event);
    //进度条刷新
    void slider();
    //打开歌词文件
    int Parsing_Lyrics(char *s);
    //循环模式
    void Circulation_patterns();
    //单曲循环
    void Single_cycle();
    //随机播放
    void Random_broadcast();
    //将歌词读入容器
    void GetLrcText(char *s);
    //显示歌词
    int show_lrc();

    void change();


private slots:
    //暂停/播放
    void on_pushButton_clicked();
    //进度条
    void on_horizontalSlider_valueChanged(int value);
    //声音打开/关闭
    void on_pushButton_4_clicked();
    //音量调整
    void on_horizontalSlider_2_valueChanged(int value);
    //歌曲上一首
    void on_pushButton_2_clicked();
    //下一首歌曲
    void on_pushButton_3_clicked();
    //播放模式
    void on_pushButton_5_clicked();
    //快进
    void on_pushButton_6_clicked();
    //遍历歌词
    void printf_lrc(QString s);

    void on_horizontalSlider_sliderPressed();

    void on_horizontalSlider_sliderReleased();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
