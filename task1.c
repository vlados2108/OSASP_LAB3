#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>

void PrintInfo()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME,&ts);
    time_t seconds = ts.tv_sec;
    int mseconds = ts.tv_nsec/1e6;
    struct tm *tm = localtime(&seconds);
    printf("Данный процесс: %d\tРодительский процесс: %d\n",getpid(),getppid()); 
    printf("Время: %d:%d:%d:%d\n",tm->tm_hour,tm->tm_min,tm->tm_sec,mseconds);
}

int main()
{
    pid_t pid;
   
    PrintInfo();
    for (int i=0;i<2;i++)
    {  
        pid = fork();
        switch (pid)
        {
            case -1:
            {
                fputs("Проблема при создании дочернего процесса",stderr);
                _exit(errno);
            }
            case 0:
            {
                puts("--------------------------");
                printf("Дочерний процесс %d:\n",i+1);
                PrintInfo();          
                _exit(EXIT_SUCCESS);               
            }
            default:
                break;
        }   
    }  
    if (pid > 0)
    {
        system("ps -x");
        while (wait(NULL) != -1);       
    }
    return 0;
}