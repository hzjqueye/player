#include "mainwindow.h"
#include <QApplication>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>

void *sliderchange(void *arg)
{
    MainWindow *w = (MainWindow *)arg;
    while(1)
    {              
        usleep(500*1000);
        w->slider();
    }
}

void *setmod(void *arg)
{
    MainWindow *w = (MainWindow *)arg;
    sleep(3);
    while(1)
    {
        sleep(1);
        if(w->song_percent == 99)
        {
            if(w->play_mode == 1)
            {
                w->Circulation_patterns();//循环播放
            }
            else if(w->play_mode == 2)
            {
                w->Single_cycle();//单曲循环
            }
            else if(w->play_mode == 3)
            {
                w->Random_broadcast();//随机播放
            }
        }
    }
}

void *printlrc(void *arg)
{
    MainWindow *w = (MainWindow *)arg;
    sleep(1);
    while(1)
    {
//        if(w->song_lrc.back().time > w->now_second)
//        {
            w->show_lrc();
//        }
    }
}

int main(int argc, char *argv[])
{

    pthread_mutex_init(&mutex, NULL);
    //创建有名管道来用于ui和mplayer之间的通信
    int ret = mkfifo("ui_mp", 0666);
    if(ret < 0)
    {
        perror("mkfifo");
    }
    int get = mkfifo("mp_ui", 0777);

    if(get < 0)
    {
        perror("pipe");
    }

    //创建子进程用来控制mplayer
    pid_t pid = fork();
    if(pid < 0)
    {
        perror("fork");
    }
    else if(pid == 0)
    {
        int fd_m_u = open("mp_ui", O_RDWR);
        close(1);
        dup2(fd_m_u, 1);
        execlp("mplayer",\
               " mplayer ",\
               "-slave", "-quiet","-idle",\
               "-input", "file=./ui_mp",\
               "./我为什么这么帅.mp3", NULL);
    }
    else
    {
        QApplication a(argc, argv);
        MainWindow w;
        w.show();
        //传递mplayer的进程号
        w.pid = pid;

        //创建子进程
        pthread_t pth1,pth2,pth3,pth4,pth5;
        pthread_create(&pth1, NULL, getMsgfromMplayer, NULL);
        pthread_create(&pth2, NULL, MyGetTimeAndBar, (void *)(&w));
        pthread_create(&pth3, NULL, sliderchange, (void *)(&w));
        pthread_create(&pth4, NULL, setmod, (void *)(&w));
        pthread_create(&pth5, NULL, printlrc, (void *)(&w));

        w.pth = pth5;
        return a.exec();
    }
    //return a.exec();
}
