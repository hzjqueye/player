#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDesktopWidget>
#include <QStringList>
#include <QPixmap>
#include <QPalette>
#include <QBrush>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <QString>
#include <QStringListModel>
#include <QListWidget>
#include <QListWidgetItem>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include <QDebug>
#include <pthread.h>
#include <time.h>
#include <QList>
#include <algorithm>
#include <iostream>
#include <QTimer>
#include <QDateTime>

using namespace std;

int mo = 1;
//声音打开标志
int i = 1;
//设置一互斥锁
pthread_mutex_t mutex;
//声明进度条互斥锁
pthread_mutex_t qingbijinjiansuo;
//暂停标志位
int play_pause = 1;
//歌词容器
//QList<LRC> song_lrc;
//歌曲信息容器
list<char *> song_msg;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setStyleSheet("QMainWindow{border-image: url(:/图片/背景2.jpg)}");

    //设置窗口固定大小
    this->setFixedSize(800, 600);

    ui->label_3->setAlignment(Qt::AlignHCenter);
    ui->label_4->setAlignment(Qt::AlignHCenter);
    ui->label_5->setAlignment(Qt::AlignHCenter);

    //设置listwidget透明
    ui->listWidget->setStyleSheet("background:transparent;border-width:0;border-style:outset");

    ui->label_2->setStyleSheet("background:transparent;border-width:0;border-style:outset");
    ui->label_6->setStyleSheet("background:transparent;border-width:0;border-style:outset");
    ui->label_7->setStyleSheet("background:transparent;border-width:0;border-style:outset");
    ui->label_3->setStyleSheet("background:transparent;border-width:0;border-style:outset");
    ui->label_4->setStyleSheet("background:transparent;border-width:0;border-style:outset");
    ui->label_5->setStyleSheet("background:transparent;border-width:0;border-style:outset");

    //使窗口显示在屏幕正中间
    ui->label_6->setText("");
    ui->label_7->setText("");
    QDesktopWidget *deskdop=QApplication::desktop();
    move((deskdop->width()-this->width())/2, (deskdop->height()-this->height())/2);

    //设置随机种子，保证每次运行代码时，随机值都不一样
    srand(time(NULL));

    //设置播放器名字
    this->setWindowTitle("坚哥的音乐播放器");

    //设置歌曲列表
    setsonglist();

    //设定歌曲播放模式
    ui->pushButton_5->setText("循环播放");

    //打开与mplayer的通信管道
    int fd_u_m = open("ui_mp", O_WRONLY);

    //将音量进度条设定为99
    ui->horizontalSlider_2->setValue(99);

    QTimer *timer = new QTimer(this);
    timer->start(30000);
    connect(timer,&QTimer::timeout,this,&MainWindow::change);

    //切歌
    connect(ui->listWidget, &QListWidget::itemDoubleClicked, [=](QListWidgetItem *item){
        freelock();
        song_lrc.clear();
        ui->label_2->setText("");
        ui->label_6->setText("");
        ui->label_7->setText("");
        printf_lrc(ui->listWidget->currentItem()->text());
    });
}
//析够函数
MainWindow::~MainWindow()
{
    delete ui;
}

//设置歌曲列表
void MainWindow::setsonglist()
{
    DIR * dir = opendir("./song");
      while(1)
      {

          struct dirent *dirp = readdir(dir);
          if(dirp == NULL)
          {
              break;
          }
          if(dirp->d_type == DT_REG)
          {
              QListWidgetItem * item = new QListWidgetItem(dirp->d_name);
              item->setTextAlignment(Qt::AlignHCenter);
              ui->listWidget->addItem(item);
              if(this->Number_songs == 0)
              {
                  ui->listWidget->setCurrentItem(item,QItemSelectionModel::Select);
              }

              Number_songs++;
          }
      }
      closedir(dir);
}

//关闭子进程
void MainWindow::closeEvent(QCloseEvent *event)
{
    pthread_cancel(this->pth);
    char uu[128] = {0};
    sprintf(uu, "kill -9 %d", pid);
    system(uu);
}


//**********************暂停/播放**********************
//进度条
void MainWindow::slider()
{
    ui->label_3->setText(this->title);
    ui->label_4->setText(this->singer);
    ui->label_5->setText(this->album);
    pthread_mutex_lock(&qingbijinjiansuo);
    ui->horizontalSlider->setMaximum(this->song_length);
    ui->horizontalSlider->setValue(this->now_second);
    pthread_mutex_unlock(&qingbijinjiansuo);
}

//歌曲进度条
void MainWindow::on_horizontalSlider_valueChanged(int value)
{
    this->value = value;
}


//**********************声音**********************
//打开/关闭声音
void MainWindow::on_pushButton_4_clicked()
{
    if(i == 1)
    {
        SendMsgToMplayer("mute 1\n");
        ui->pushButton_4->setText("off");
    }
    else if(i == -1)
    {
        SendMsgToMplayer("mute 0\n");
        ui->pushButton_4->setText("on");
    }
    i *= -1;
}

//调整声音
void MainWindow::on_horizontalSlider_2_valueChanged(int value)
{
    char s1[20] = "";
    sprintf(s1, "volume  %d  1\n", value);
    SendMsgToMplayer(s1);
}

//暂停/播放
void MainWindow::on_pushButton_clicked()
{
        setlock();
        SendMsgToMplayer("pause\n");
}

//**********************歌词**********************
//上一首歌曲
void MainWindow::on_pushButton_2_clicked()
{
    freelock();
    song_lrc.clear();
    ui->label_2->setText("");
    ui->label_6->setText("");
    ui->label_7->setText("");
    if(ui->listWidget->currentRow() == 0)//当光标在第一个文件时，点击上一个光标移动到最下面的文件，不播放
    {
        ui->listWidget->setCurrentRow(ui->listWidget->count()-1);
    }
    else
    {
        ui->listWidget->setCurrentRow(ui->listWidget->currentRow()-1);
    }
    printf_lrc(ui->listWidget->currentItem()->text());
}

//下一首歌曲
void MainWindow::on_pushButton_3_clicked()
{
    freelock();
    song_lrc.clear();
    ui->label_2->setText("");
    ui->label_6->setText("");
    ui->label_7->setText("");
    if(play_mode == 1 || play_mode == 2)
    {
        if(ui->listWidget->currentRow() == ui->listWidget->count()-1)
        {
            ui->listWidget->setCurrentRow(0);//当光标在最后一个文件时，点击下一个，光标移动到第一个，不播放
        }
        else
        {
            ui->listWidget->setCurrentRow(ui->listWidget->currentRow()+1);
        }
    }
    else if(play_mode == 3)
    {
        int r = rand() % (Number_songs - 1);
        ui->listWidget->setCurrentRow(r);
    }
    printf_lrc(ui->listWidget->currentItem()->text());
}

//********************播放模式********************
//播放模式
void MainWindow::on_pushButton_5_clicked()
{
    //循环播放
    if(this->play_mode == 1)
    {
        ui->pushButton_5->setText("单曲循环");
        this->play_mode++;
    }
    //单曲循环
    else if(this->play_mode == 2)
    {
        ui->pushButton_5->setText("随机播放");
        this->play_mode++;
    }
    //随机播放
    else
    {
        ui->pushButton_5->setText("循环播放");
        this->play_mode -= 2;
    }
}

//循环放歌
void MainWindow::Circulation_patterns()
{
    freelock();
    song_lrc.clear();
    ui->label_2->setText("");
    ui->label_6->setText("");
    ui->label_7->setText("");
    if(ui->listWidget->currentRow() == ui->listWidget->count()-1)
    {
        ui->listWidget->setCurrentRow(0);//当光标在最后一个文件时，点击下一个，光标移动到第一个，不播放
    }
    else
    {
        ui->listWidget->setCurrentRow(ui->listWidget->currentRow()+1);
    }
    printf_lrc(ui->listWidget->currentItem()->text());
}

//单曲循环
void MainWindow::Single_cycle()
{
    freelock();
    song_lrc.clear();
    ui->label_2->setText("");
    ui->label_6->setText("");
    ui->label_7->setText("");
    printf_lrc(ui->listWidget->currentItem()->text());
}

//随机播放
void MainWindow::Random_broadcast()
{
    freelock();
    song_lrc.clear();
    ui->label_2->setText("");
    ui->label_6->setText("");
    ui->label_7->setText("");
    int r = rand() % (Number_songs - 1);
    ui->listWidget->setCurrentRow(r);
    printf_lrc(ui->listWidget->currentItem()->text());
}

//快进200秒
void MainWindow::on_pushButton_6_clicked()
{
    ui->label_2->setText("");
    int value = this->song_length -10;
    char s1[32] = "";
    sprintf(s1, "seek  %d\n", value);
    qDebug() << s1;
    SendMsgToMplayer(s1);
}



//********************歌词********************
//打开歌词文件
int MainWindow::Parsing_Lyrics(char *s)
{
    FILE *fp;
        //调用歌词文件
        if((fp = fopen(s,"r")) == NULL)
        {
            perror("fail to open");
            printf("这首歌没有歌词\n");
            return -1;
        }
        char s1[100] = {0};
        while(fgets(s1, sizeof(s1), fp) != NULL)
        {
            GetLrcText(s1);
        }
   fclose(fp);
}

//将歌词读入容器
void MainWindow::GetLrcText(char *s)
{
    int time[10] = {0};
    int min, sec;
    int n = 0;
    int k = 0;
    //提取一句歌词中的时间和歌词部分
    //[03:34.64][02:34.71][01:05.83]我想就这样牵着你的手不放开
    if(*(s + 1) == '0')
    {
        while(*s == '[')
        {
            sscanf(s, "[%d:%d", &min, &sec);
            time[n] = min * 60 + sec;
            n++;
            s = s + 10;
        }
        //通过list容器按照时间保存时间和歌词
        int i;
        for(i = 0; i < n; i++)
        {
            LRC mydata;
            mydata.time = time[i];
            strcpy(mydata.text, s);
            //通过list容器按照时间保存时间和歌词
            song_lrc.push_back(mydata);
        }
    }
}

//显示歌词
int MainWindow::show_lrc()
{
    while(1)
    {
        usleep(500*1000);
        QList<LRC>::iterator it = song_lrc.begin();
        while(it != song_lrc.end())
        {
            if(it->time == now_second)
            {
                if(it == (song_lrc.end()-2))
                {
                    ui->label_2->setAlignment(Qt::AlignHCenter);
                    QPalette ca;
                    ca.setColor(QPalette::WindowText,Qt::green);
                    ui->label_2->setPalette(ca);
                    ui->label_2->setText(it->text);
                    it++;
                    ui->label_6->setAlignment(Qt::AlignHCenter);
                    ui->label_6->setText(it->text);
                    ui->label_7->setText("");
                    it--;
                }
                else if(it == (song_lrc.end()-1))
                {
                    ui->label_2->setAlignment(Qt::AlignHCenter);
                    QPalette aa;
                    aa.setColor(QPalette::WindowText,Qt::green);
                    ui->label_2->setPalette(aa);
                    ui->label_2->setText(it->text);
                    ui->label_6->setText("");
                    ui->label_7->setText("");
                }
                else
                {
                    ui->label_2->setAlignment(Qt::AlignHCenter);
                    QPalette pa;
                    pa.setColor(QPalette::WindowText,Qt::green);
                    ui->label_2->setPalette(pa);
                    ui->label_2->setText(it->text);
                    it++;
                    ui->label_6->setAlignment(Qt::AlignHCenter);
                    ui->label_6->setText(it->text);
                    it++;
                    ui->label_7->setAlignment(Qt::AlignHCenter);
                    ui->label_7->setText(it->text);
                    it--;
                    it--;
                }
            }
            it++;
        }
    }
}


//遍历歌词
void MainWindow::printf_lrc(QString stg)
{
    char s[255] = "";
    strcat(s, "./LRC/");

    char buff1[64] = "";
    QByteArray qq = stg.toUtf8();
    strcpy(buff1, qq.data());

    char *buff2 = strtok(buff1, ".");
    strcat(s, buff2);
    strcat(s, ".lrc");
    Parsing_Lyrics(s);

    qDebug() << stg;
    QString str = "loadfile \"./song/";
    str += stg;
    str += "\"";
    str += "\n";
    qDebug() << s;
    char mm[128] = "";
    QByteArray ca = str.toUtf8();
    strcpy(mm, ca.data());
    qDebug() << mm;
    SendMsgToMplayer(mm);

}

//歌曲进度条上锁
void MainWindow::on_horizontalSlider_sliderPressed()
{
    ui->label_2->setText("");
    pthread_mutex_lock(&qingbijinjiansuo);
    char s1[20] = "";
    char s2[20] = "";
    int s;
    if(this->value > this->now_second)
    {
        s = this->value - this->now_second;
        sprintf(s1, "seek  %d\n", s);
        SendMsgToMplayer(s1);
    }
    else
    {
        s = this->value - this->now_second;
        sprintf(s2, "seek  %d\n", s);
        printf("%d", s);
        SendMsgToMplayer(s2);
    }

}

//歌曲进度条解锁
void MainWindow::on_horizontalSlider_sliderReleased()
{
    ui->label_2->setText("");
    ui->label_6->setText("");
    ui->label_7->setText("");
    pthread_mutex_unlock(&qingbijinjiansuo);
    char s1[20] = "";
    char s2[20] = "";
    int s;
    if(this->value > this->now_second)
    {
        s = this->value - this->now_second;
        sprintf(s1, "seek  %d\n", s);
        SendMsgToMplayer(s1);
    }
    else
    {
        s = this->value - this->now_second;
        sprintf(s2, "seek  %d\n", s);
        printf("%d", s);
        SendMsgToMplayer(s2);
    }

}

void MainWindow::change(){
    if(mo == 1)
    {
        this->setStyleSheet("QMainWindow{border-image: url(:/图片/背景2.jpg)}");
        mo += 1;
    }
    else if(mo == 2)
    {
        this->setStyleSheet("QMainWindow{border-image: url(:/图片/背景3.jpg)}");
        mo += 1;

    }
    else if(mo == 3)
    {
        this->setStyleSheet("QMainWindow{border-image: url(:/图片/背景.jpg)}");
        mo = mo - 2;

    }
}




//********************类外函数**********************

//向mplayer发送指令
void SendMsgToMplayer(char *str)
{
    int fd_u_m = open("ui_mp", O_WRONLY);
    write(fd_u_m, str, strlen(str));
    close(fd_u_m);
}

//子进程（发送获取歌曲当前相关信息的指令）
void *getMsgfromMplayer(void *arg)
{
    usleep(500*1000);
    //不停的给mplayer发送获取当前时间以及进度
    while(1)
    {
        pthread_mutex_lock(&mutex);
        usleep(1000*50);//0.05秒发指令
        SendMsgToMplayer("get_percent_pos\n");//获取当前进度
        usleep(1000*50);//0.05秒发指令
        SendMsgToMplayer("get_time_pos\n");//获取当前歌曲时间
        usleep(1000*50);//0.05秒发指令
        SendMsgToMplayer("get_time_length\n");//获取当前歌曲长度
        usleep(1000*50);//0.05秒发指令
        SendMsgToMplayer("get_meta_title\n");//获取当前歌名
        usleep(1000*50);//0.05秒发指令
        SendMsgToMplayer("get_meta_artist\n");//获取当前歌手
        usleep(1000*50);//0.05秒发指令
        SendMsgToMplayer("get_meta_album\n");//获取当前专辑
        pthread_mutex_unlock(&mutex);
        usleep(1000*50);//0.05秒发指令
    }
}

//子进程（获取歌曲当前相关信息）
void *MyGetTimeAndBar(void *arg)
{
    //int fd = (int)(long)(arg);
    MainWindow *w = (MainWindow *)arg;
    int fd_m_u = open("mp_ui", O_RDWR);
    while(1)
    {
        char buf[128] = "";
        read(fd_m_u,buf,sizeof (buf));
        char cmd[128] = "";
        sscanf(buf,"%[^=]",cmd);
        if(strcmp(cmd,"ANS_PERCENT_POSITION") == 0)//百分比
        {
            int percent_pos = 0;
            sscanf(buf,"%*[^=]=%d",&percent_pos);
            w->song_percent = percent_pos;
        }
        else if(strcmp(cmd,"ANS_TIME_POSITION") == 0)//当前时间
        {
            float time_pos = 0;
            sscanf(buf,"%*[^=]=%f", &time_pos);
            w->now_second = (int)time_pos;
        }
        else if(strcmp(cmd,"ANS_LENGTH") == 0)//当前歌曲长度
        {
            float song_length = 0;
            sscanf(buf,"%*[^=]=%f", &song_length);
            w->song_length = (int)song_length;
        }
        else if(strcmp(cmd,"ANS_META_TITLE") == 0)//当前歌曲名
        {
            char song_title[16] = "0";
            sscanf(buf,"ANS_META_TITLE='%[^']", &song_title);
            w->title = song_title;
        }
        else if(strcmp(cmd,"ANS_META_ARTIST") == 0)//当前歌曲歌手
        {
            char song_singer[16] = "0";
            sscanf(buf,"ANS_META_ARTIST='%[^']", &song_singer);
            w->singer = song_singer;
        }
        else if(strcmp(cmd,"ANS_META_ALBUM") == 0)//当前歌曲专辑
        {
            char song_album[16] = "0";
            sscanf(buf,"ANS_META_ALBUM='%[^']", &song_album);
            w->album = song_album;
        }
        fflush(stdout);
    }
}

//上暂停锁
void setlock()
{
    if(play_pause == 1)
    {
        pthread_mutex_lock(&mutex);
    }
    else if(play_pause == -1)
    {
        pthread_mutex_unlock(&mutex);
    }
    play_pause *= -1;
}

//释放暂停锁
void freelock()
{
    pthread_mutex_unlock(&mutex);
    if(play_pause == -1)
    {
    play_pause *= -1;
    }
}

