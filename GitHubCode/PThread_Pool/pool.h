#ifndef _MyPthread_pool_
#define _MyPthread_pool_

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>
#include <net/if_arp.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>

#define POST 9005
#define PTHREAD_MAX_NUMBER 50
#define PTHREAD_MIN_NUMBER 5
#define PTHREAD_DEF_NUMBER 20

typedef struct task_node
{
    pthread_t tid;
    //pthread_cond_t cond;
    pthread_mutex_t mutex;
    int flag;                   //1:busy,2:free
    int work_id;
    struct task_node * next;
    void * (*fun)(void *);
    void * arg;
}TASK_NODE;


typedef struct task_queue
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    struct task_node * head;
    struct task_node * rear;
    int size;
}TASK_QUEUE;


typedef struct pthread_node
{
    pthread_t tid;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    struct pthread_node * prev;
    struct pthread_node * next;
    int flag;
    struct task_node * work;
}PTHREAD_NODE;

typedef struct pthread_queue
{
    struct pthread_node * head;
    struct pthread_node * rear;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int size;
}PTHREAD_QUEUE;

extern PTHREAD_QUEUE * pthread_leisure_queue;
extern PTHREAD_QUEUE * pthread_busy_queue;
extern TASK_QUEUE * task_NeedWork_queue;


void sys_clean(void);
void * Pthread_Work(void * ptr);
void Create_Pthread_Node();
void PthreadPool_system_init();
void *prcoess_client(void * ptr);
void Task_Manager();
void Thread_Manager();
void monitor(void * ptr);
#endif