//理解进程、线程、互斥锁、自旋锁、原子操作、并发 √

/*并发背后的真正场景：
     主线程是进程启动后默认创建的第一个线程，负责执行main()函数，但它和其他子线程地位平等(都是进程内的
  执行流)，操作系统会调度主线程和子线程“并发”执行，所以显式的代码块就是主线程的执行逻辑！！！main()
  函数就是主线程的执行体，main()函数里的所有代码都默认由主线程执行，因为子线程在背后默默并发，所以
  主线程代码中被子线程所操作的变量如果在主线程中有重复打印，就会观到子线程在背后对于该变量的操作，只
  有传给pthread_create()函数里的入口函数才是子线程的专属执行逻辑。
  
  同时也正因如此，进程的“生死大权”由主线程掌控，当主线程的main()函数返回出发进程退出时，所有的子线程
  都会被弊掉，所以需要用pthread_join()函数，在此由主线程等待所有子线程的“汇合”，主线程才继续销毁锁
  并执行main()函数退出。*/


//头文件：
#include <stdio.h>
#include <pthread.h> //线程函数头文件 + link到Linux系统自带库下的pthread包（符合Posix规范）
                     //Posix(Portable Operating System Interface)-可以指操作系统接口                  
                     //@老版的ubuntu,编译时还要在终端编译命令后加上"- lpthread"(link pthread)语句!!
                     //pthread 就是->Posix thread(Posix标准线程)的缩写！！！
#include <unistd.h>

//宏定义L：S
#define THREAD_COUNT    10//定义线程数量

//Mutual Exclusion Lock 互斥锁，由标准库定义的一个结构体类型，用于表示互斥锁变量这种类型：
pthread_mutex_t mutex;//【全局创建互斥锁】方便所有线程函数、都能方便地访问这个互斥锁
//而只需要再 main()函数内部、具体的线程函数执行前，再初始化，保证第一个调用它的线程函数是初始化过的锁

//spinlock 自旋锁，由标准库定义的一个结构体类型，用于表示自旋锁变量这种类型：
pthread_spinlock_t spinlock;//【全局创建自旋锁】
/*当线程尝试获取自旋锁时，若锁已被其他线程持有，申请锁的该线程不会进入阻塞(休眠)状态，而是持续自旋
  检查锁的状态，期间不会释放CPU的资源，直到自旋锁被释放。优势是“响应快”，缺点是“占用CPU资源” */

/*--------原子操作！！！---------*/
// int inc(int *value, int add)//把“count++用内联汇编语言汇编为一条命令语句 √”
// {  
//     int old;

//     __asm__volatile
//     (
//         "lock; xaddl %1, %2;"
//         : "=a"(old)
//         : "m"(*value), "a"(add)
//         :"cc", "memory"
//     );
// }


void *thread_callback(void *arg)//定义入口函数，返回任意类型指针，传入任意类型指针
{
    int *pcount = (int *)arg;//把传进来的指针强转为int*类型（被pthread_create擦除了类型）
    int i= 0;

    while(i++ < 100000)//i从0开始，所以实际执行循环100000次
    {
        //条件编译语法(#if,#else,#endif)，可用于C/C++,核心作用是快速切换需要参与编译的代码块
        #if 0//这段不加锁的暂时不编译、执行
            (*pcount)++;//不加锁

        #elif 0
            inc(pcount, 1);//原子操作的函数调用

        #elif 0
            pthread_mutex_lock(&mutex);
            /*获取指定的互斥锁，若该锁处于未锁定状态，函数会立即调用该锁，
              传入要获取的互斥锁变量的指针，返回0成功，非0失败*/
            (*pcount)++; //每个线程进来都把传进来的int *pcount 也就是👉 count 实现++
                         //    因为传进来的是count的指针，所以这个修改结果也带了出去 √
            pthread_mutex_unlock(&mutex);
            /*释放指定的互斥锁，让该锁处于未锁定状态，让其他线程有机会调用该锁，
              传入要释放的互斥锁变量的指针，返回0成功，非0失败*/
        #elif 1
            pthread_spin_lock(&spinlock);
             /*获取指定的自旋锁，若该锁处于未锁定状态，函数会立即调用该锁，
              传入要获取的自旋锁变量的指针，返回0成功，非0失败*/
            (*pcount)++; //加自旋锁
            pthread_spin_unlock(&spinlock);
            /*释放指定的自旋锁，让该锁处于未锁定状态，让其他线程有机会调用该锁，
              传入要释放的自旋锁变量的指针，返回0成功，非0失败*/

        #endif//到这里条件编译语法结束，后面的代码正常编译、执行。
            /* usleep(1);/*让当前线程休眠1微妙（CPU切出去），（1秒 = 1,000,00微秒）
                    在while循环对pcount自增时 → 线程间有切换的机会，模拟线程间的CPU资源竞争 √ */
    }
}

//主函数：
int main()
{   //pthread_t是posix thread type类型的变量（一般为无符号long，结构体或指针）
    pthread_t threadid[THREAD_COUNT] = {0};
    /*创建pthread_t(线程标识符类变量)类型的数组threadid，用以接收新创建的线程ID
      这种类型的变量具体类型不确定（可能是unsigned long，也有可能是结构体，取决于操作系统）*/

    pthread_mutex_init(&mutex, NULL);
    /*用于初始化“互斥锁”类型变量，pthread_mutex_init(2个参数)，返回0则执行成功，非0失败
    参数1：pthread_mutex_t*, 【互斥锁指针】指向要初始化的pthread_mutex_t类型的变量的指针；
    参数2：const pthread_mutexattr_t*,【互斥锁属性类-结构体】
          （加const表示不希望此函数修改该指针所指向的“互斥锁属性类结构体”内容）
          指向互斥锁的属性，若传入NULL，则表示使用默认属性。*/
    
    pthread_spin_init(&spinlock, PTHREAD_PROCESS_PRIVATE);
    /*用于初始化“自旋锁”类型变量，pthread_spin_init(2个参数)，返回0则执行成功，非0失败
    参数1：pthread_spinlock_t*, 【自旋锁指针】指向要初始化的pthread_spinlock_t类型的变量的指针；
    参数2：int pshared,【宏定义值指定自旋锁类型】(int pthread shared->进程间共享的int操作数)
                        PTHREAD_PROCESS_PRIVATE -> 线程_进程_私有：表示该自旋锁是进程内私有的，
                                                                  只能当前进程中的线程使用，不
                                                                  能跨进程共享。
                        PTHREAD_PROCESS_SHARED  -> 线程_进程_共享：表示该自旋锁可以跨进程共享，
                                                                  即多个不同进程可以通过共享内
                                                                  存访问同一个自旋锁。 */

    int i = 0;//给pthead_create()函数循环创建新线程计数用
    int count = 0;//给pthread_create()创建的新线程的入口函数的传参
    for(i = 0; i < THREAD_COUNT; i++)//遍历所创建的pthread_t
    {
        pthread_create(&threadid[i], NULL, thread_callback, &count);
        //【其核心功能是创建一个新的线程，并让它开始执行入口函数！！！】

        /* int pthread_create(4个参数)；返回0则执行成功，非0失败
        参数1：pthread_t*,创建新线程的"遥控器"，这个指针指向遥控器【pthread_t变量-线程ID】，
              也就是说这个函数会创建一个新线程，并把这个线程的唯一标识符(pthread_t类型)
              写入到这个指针所对应的内存地址，也就是让这个指针指向新线程标识符(线程遥控器)！
              再用相关的线程函数来操作这个线程遥控器，以此来操作和管理具体的线程。
        参数2：const pthread_attr_t*, pthread_attribute_type【线程属性类-结构体】
              加const表示不希望此函数修改该指针所指向的“线程属性类结构体”内容
             （用const向函数传指针，此函数就不能修改该指针所指向的内容了！）
              该参数用于设置线程属性，通过指针来选择内容，传NULL意味着用默认属性。
        参数3：void *(*start_routine)(void*),【入口函数】
              实际填写时，只需要填函数名->会被隐式转化为指针！
              这是指向新线程要执行的函数，新线程启动后，会从这个函数开始执行代码；
              这个函数的返回这必须是一个指针(void*)！为了传递线程的执行结果；
              并且此函数只接收！一个指针(void*)，如果需要传入多个指针，可以封装到结构体中再进行传参。
        参数4：void *arg,【传给线程入口函数的参数】
              用指针(void*)主要是因为它可以指向任何类型的地址，也就是可以传递任何类型的数据，
              甚至是结构体，避免了入口函数接口的局限性，注意！这是create函数的强制类型擦除！ */
    }
    
    for(i = 0; i < 100; i++)
    {
        printf("count: %d\n", count);//观察100次前面那10个线程对count的处理结果，每次隔1秒。
        sleep(1);//让当前线程暂停执行一段时间（CPU切出去），在Linux/Unix系统中单位是秒(s)，
                 // windows中是毫秒(ms)。  →  这里是为了不让终端时刻都在打印
    }

    for(i = 0; i < THREAD_COUNT; i++)
    {
        pthread_join(threadid[i], NULL);
    /* pthread_join(2个参数)：【线程汇入】
            用于主线程等待指定子线程（线程遥控器、线程ID）结束并回收其资源，成功返回0，非0失败
       第一个参数：pthread_t,表示要等待的子线程的ID（该函数还会回收pthread_t这种控制结构）；
       第二个参数：retval**,类型为void**(任意类型二级指针)，用于接收子线程的返回值，若需要获取
                  需定义一个void*类型指针，传入这个一级指针的二级指针来就收子线程的返回值（指针，
                  本质上指向一块实际存储子线程具体返回值的内存地址），也可以传入NULL，即表示不
                  需要获取子线程的返回值、不需要获取子线程的返回内容。*/
    }
    
    pthread_mutex_destroy(&mutex);//销毁互斥锁，传入要销毁的互斥锁的指针

    pthread_spin_destroy(&spinlock);//销毁自旋锁，传入要销毁的互斥锁的指针

    return 0;
}