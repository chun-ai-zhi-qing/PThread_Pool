#include "pool.h"
PTHREAD_QUEUE * pthread_leisure_queue;
PTHREAD_QUEUE * pthread_busy_queue;
TASK_QUEUE * task_NeedWork_queue;
void sys_clean(void)
{
    perror("system error");
    exit(EXIT_FAILURE);
}
void * Pthread_Work(void * ptr)
{
    PTHREAD_NODE * self = (PTHREAD_NODE *) ptr;
    pthread_mutex_lock (&self->mutex);
    //self->tid = gettid();
    self->tid = syscall (SYS_gettid);
    pthread_mutex_unlock (&self->mutex);
    while (1)
    {
        pthread_mutex_lock (&self->mutex);
        if(self->work==NULL)
            pthread_cond_wait(&self->cond,&self->mutex);
        pthread_mutex_lock (&self->work->mutex);
        self->work->fun(self->work->arg);
        self->work->fun = NULL;
        free(self->work->arg);
        self->work->flag = 0;
        self->next = NULL;
        self->tid = 0;
        self->work->arg=NULL;
        pthread_mutex_unlock (&self->work->mutex);
        free (self->work);
        self->work = NULL;

        pthread_mutex_lock(&pthread_busy_queue->mutex);
        if(pthread_busy_queue->size==1)
        {
            pthread_busy_queue->rear = NULL;
            pthread_busy_queue->head = NULL;
        }
        else if(pthread_busy_queue->rear==self&&pthread_busy_queue->head!=self)
        {
            pthread_busy_queue->rear = self->prev;
            pthread_busy_queue->rear->next = NULL;
        }
        else if(pthread_busy_queue->head == self && pthread_busy_queue->rear != self)
        {
            pthread_busy_queue->head = self->next;
            self->next->prev = NULL;
        }
        else
        {
            self->prev->next = self->next;
            self->next->prev = self->prev;
        }
        self->prev = self->next = NULL;
        pthread_busy_queue->size--;
        pthread_mutex_unlock(&pthread_busy_queue->mutex);

        pthread_mutex_lock(&pthread_leisure_queue->mutex);
        if(pthread_leisure_queue->size==0)
        {
            pthread_leisure_queue->head = self ;
            pthread_leisure_queue->rear = self;
        }
        else
        {
            self->prev = pthread_leisure_queue->rear;
            pthread_leisure_queue->rear->next = self;
            pthread_leisure_queue->rear = self;
        }
        pthread_leisure_queue->size++;
        pthread_mutex_unlock(&pthread_leisure_queue->mutex);
        pthread_mutex_unlock(&self->mutex);
        pthread_cond_signal(&pthread_leisure_queue->cond);

    }
    
}
void Create_Pthread_Node()
{
    pthread_mutex_lock (&pthread_leisure_queue->mutex);  
    PTHREAD_NODE * temp = NULL;
    PTHREAD_NODE * preTmp = NULL;
    for(int i = 1;i<=PTHREAD_DEF_NUMBER;i++)
    {
        temp = (PTHREAD_NODE *) malloc(sizeof(PTHREAD_NODE));
        if(temp == NULL)
        {
            printf ("malloc failure\n");
            exit (EXIT_FAILURE);
        }
        if(i==1)
            pthread_leisure_queue->head = temp;
        temp->flag = 0;
        temp->prev = preTmp;
        temp->work = NULL;
        pthread_cond_init(&(temp->cond),NULL);
        pthread_mutex_init(&(temp->mutex),NULL);
        //temp->prev = preTmp;
        if(preTmp!=NULL)
            preTmp->next = temp;
        pthread_create(&temp->tid,NULL,Pthread_Work,(void *)temp);
        preTmp = temp;
        temp = NULL;
    }
    pthread_leisure_queue->rear = preTmp;
    pthread_leisure_queue->size = PTHREAD_DEF_NUMBER;
    pthread_mutex_unlock(&(pthread_leisure_queue->mutex));
}
void PthreadPool_system_init()
{
    pthread_leisure_queue = (PTHREAD_QUEUE *)malloc(sizeof(PTHREAD_QUEUE));
    pthread_leisure_queue->head = NULL;
    pthread_leisure_queue->rear = NULL;
    pthread_leisure_queue->size = 0;
    pthread_cond_init(&(pthread_leisure_queue->cond),NULL);
    pthread_mutex_init(&(pthread_leisure_queue->mutex),NULL);

    pthread_busy_queue = (PTHREAD_QUEUE *)malloc(sizeof(PTHREAD_QUEUE));
    pthread_busy_queue->size = 0;
    pthread_busy_queue->rear = NULL;
    pthread_busy_queue->head = NULL;
    pthread_cond_init(&(pthread_busy_queue->cond),NULL);
    pthread_mutex_init(&(pthread_busy_queue->mutex),NULL);

    task_NeedWork_queue = (TASK_QUEUE * )malloc(sizeof(TASK_QUEUE));
    task_NeedWork_queue->size = 0;
    task_NeedWork_queue->head = NULL;
    task_NeedWork_queue->rear = NULL;
    pthread_cond_init(&(task_NeedWork_queue->cond),NULL);
    pthread_mutex_init(&(task_NeedWork_queue->mutex),NULL);

    Create_Pthread_Node();
}
void *prcoess_client(void * ptr)
{
    int net_fd;
    net_fd = atoi((char *)ptr);
    char buff[128];
    if(-1 == recv (net_fd, buff, sizeof (buff), 0))
    {
        printf ("recv msg error\n");      
        close (net_fd);
        goto clean;  
    }
    printf("%d\n",syscall (SYS_gettid));
    strcpy(buff,"hello i'm server");
    if(-1==send(net_fd,buff,strlen(buff),0))
    {
        printf ("send msg error\n");      
        close (net_fd);
        goto clean;
    }
    close (net_fd);
    return;
clean:
    sys_clean();
}
void Task_Manager()
{
    int sock_fd;
    sock_fd = socket(AF_INET,SOCK_STREAM,0);
    if(sock_fd==-1)
    {
        perror("listen error");
        goto clean;
    }
    struct sockaddr_in myaddrs;
    memset(&myaddrs,0,sizeof(myaddrs));
    myaddrs.sin_port = htons(POST);
    myaddrs.sin_family = AF_INET;
    myaddrs.sin_addr.s_addr = INADDR_ANY;
    if(-1==(bind(sock_fd,(struct sockaddr *)&myaddrs,sizeof(myaddrs))))
    {
        perror("bind");
        goto clean;
    }
    if (-1 == listen (sock_fd, 5))    
    {      
        perror ("listen");
        goto clean;   
    }
    TASK_NODE * newtask;
    for(int number = 1;;number++)
    {
        int newfd;
        struct sockaddr_in client;
        socklen_t len = sizeof (client);
        if (-1 ==(newfd = accept (sock_fd, (struct sockaddr *) &client, &len)))        
        {  
            perror ("accept");
            goto clean;        
        }
        newtask = (TASK_NODE *) malloc (sizeof (TASK_NODE));
        if (newtask == NULL)        
        {          
            printf ("malloc error");       
            goto clean;        
        }
        newtask->flag = 0;

        newtask->arg = (void *) malloc (128);
        memset (newtask->arg, '\0', 128);
        sprintf (newtask->arg, "%d", newfd);

        newtask->next = NULL;
        newtask->fun = prcoess_client;
        newtask->tid = 0;
        newtask->work_id = number;
        //pthread_cond_init(&(newtask->cond),NULL);
        pthread_mutex_init(&(newtask->mutex),NULL);
        
        pthread_mutex_lock(&(task_NeedWork_queue->mutex));
        if(task_NeedWork_queue->size == 0)
        {
            task_NeedWork_queue->head  = newtask;
            task_NeedWork_queue->rear = newtask;
        }
        else
        {
            task_NeedWork_queue->rear->next = newtask;
            task_NeedWork_queue->rear = newtask;
        }
        task_NeedWork_queue->size++;
        pthread_mutex_unlock(&(task_NeedWork_queue->mutex));
        pthread_cond_signal(&(task_NeedWork_queue->cond));
    }
    return;
clean:
    sys_clean();
}
void Thread_Manager()
{
    PTHREAD_NODE * temp_thread;
    TASK_NODE * temp_task;
    while(1)
    {
        temp_thread = NULL;
        temp_task = NULL;
        pthread_mutex_lock(&(task_NeedWork_queue->mutex));
        if(task_NeedWork_queue->size==0)
            pthread_cond_wait(&(task_NeedWork_queue->cond),&(task_NeedWork_queue->mutex));
        task_NeedWork_queue->size--;
        temp_task = task_NeedWork_queue->head;
        task_NeedWork_queue->head = task_NeedWork_queue->head->next;
        temp_task->next = NULL;
        pthread_mutex_unlock(&(task_NeedWork_queue->mutex));

        pthread_mutex_lock(&(pthread_leisure_queue->mutex));
        if(pthread_leisure_queue->size==0)
            pthread_cond_wait(&(pthread_leisure_queue->cond),&(pthread_leisure_queue->mutex));
        temp_thread = pthread_leisure_queue->head;
        if(pthread_leisure_queue->head==pthread_leisure_queue->rear)
        {
            pthread_leisure_queue->head = NULL;
            pthread_leisure_queue->rear = NULL;
        }
        else
        {
            pthread_leisure_queue->head = pthread_leisure_queue->head->next;
            pthread_leisure_queue->head->prev = NULL;
        }
        pthread_leisure_queue->size--;
        pthread_mutex_unlock(&(pthread_leisure_queue->mutex));

        pthread_mutex_lock(&(temp_thread->mutex));
        temp_thread->flag = 1;
        temp_thread->next = NULL;
        temp_thread->prev =NULL;
        temp_thread->work = temp_task;
        pthread_mutex_unlock(&(temp_thread->mutex));
        
        pthread_mutex_lock(&(pthread_busy_queue->mutex));
        if(pthread_busy_queue->size==0)
        {
            pthread_busy_queue->rear = temp_thread;
            pthread_busy_queue->head = temp_thread;
        }
        else
        {
            pthread_busy_queue->rear->next = temp_thread;
            temp_thread->prev = pthread_busy_queue->rear;
            pthread_busy_queue->rear = temp_thread;
        }
        pthread_busy_queue->size++;
        pthread_mutex_unlock(&(pthread_busy_queue->mutex));
        pthread_cond_signal(&(temp_thread->cond));
        
    }
}


void monitor(void * ptr)
{
    //THREAD_NODE * temp_thread = NULL; 
    int number = 1;
    while (1)    
    {      
        printf("--------------------------------------------\n");
        printf("检测数:%d\n",number);
        pthread_mutex_lock(&(pthread_busy_queue->mutex));
        printf("忙碌线程:%d\n",pthread_busy_queue->size);
        pthread_mutex_unlock(&(pthread_busy_queue->mutex));
        pthread_mutex_lock(&(pthread_leisure_queue->mutex));
        printf("空闲线程:%d\n",pthread_leisure_queue->size);
        pthread_mutex_unlock(&(pthread_leisure_queue->mutex));
        pthread_mutex_lock(&(task_NeedWork_queue->mutex));
        printf("任务数还有:%d\n",task_NeedWork_queue->size);
        pthread_mutex_unlock(&(task_NeedWork_queue->mutex));
        printf("--------------------------------------------\n\n\n");
        number++;
        sleep(1);
    }  
    return;
}