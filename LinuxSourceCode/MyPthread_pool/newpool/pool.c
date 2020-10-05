#include"pool.h"

int main()
{
    pthread_t thread_manager_tid, task_manager_tid,monitor_tid;
    PthreadPool_system_init();
    pthread_create(&task_manager_tid,NULL,Task_Manager,NULL);
    pthread_create(&thread_manager_tid,NULL,Thread_Manager,NULL);
    pthread_create(&monitor_tid,NULL,monitor,NULL);
    pthread_join (thread_manager_tid, NULL);
    pthread_join (task_manager_tid, NULL);
    pthread_join(monitor_tid,NULL);
    sys_clean ();
    return 0;

}