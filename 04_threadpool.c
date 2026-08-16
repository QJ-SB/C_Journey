/*多线程操作列表，列表是共享资源，多线程也是mian()函数中的pthread_create()函数创造出来的，不然只有
   一个主线程！！！ */

//任务队列(客户) → 管理组件(大堂经理) → 执行队列(柜员)
//                  【线程池】

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

//【要注意，宏定义函数 ≠ 一般函数，宏本质上是对当前代码里宏定义的位置进行“文本替换”，所以访问的是“实参”】
//宏定义：【双向链表的插入】
/*item:被操作的节点；list:头指针*/
#define LIST_INSERT(item, list)  \
do                               \
{                                \
    item->next = (list);         \
    if((list) != NULL)           \
        (list)->pre = item;      \
    (list) = item;               \
    item->pre = NULL;            \
                                 \
} while (0);
//宏定义：【双向链表的删除】
/*item:被操作的节点；list:头指针*/
#define LIST_REMOVE(item, list)          \
do                                       \
{                                        \
    if(item->pre != NULL)                \
        item->pre->next = item->next;    \
    if(item->next != NULL)               \
        item->next->pre = item->pre;     \
    if(item == (list))                   \
        (list) = item->next;             \
    item->pre = NULL;                    \
    item->next = NULL;                   \
} while (0);

struct nTask//【客户】👉组成-共享任务队列
/*单个待执行任务节点的结构体的定义：
（准备以链表方式形成数据结构）
  存放1：存放'该任务的任务函数指针'；
  存放2：存放'该任务的相关参数的内容的指针'）；
  存放3：存放'前指针'；
  存放4：存放'后指针'； */
{
    void(*task_func)(void *arg);//👉【通过“该结构体的指针” -> “函数指针” ()】的形式就可以调用该函数！！！
    /*任务执行的函数的指针（指向任务执行函数）
    函数指针格式：“ 返回值类型（*指针变量名）（接收参数列表）”
      👇
      “void”表示该函数返回值为void;
      “（void *arg）表示该函数接收一个任意类型的指针”;
      以上这两个参数确保了编译器能在编译到这个结构体时，针对这个结构体内所定义的函数指针，生成对于这个
      函数指针所指向的函数的正确的调用代码类型，相当于说明了“要生成...样一个指针，要指向...样的一个函
      数，这是结构体在定义函数指针时的必要格式！！！”*/
    void *user_data;
    //定义一个任意类型的普通指针，只需要明确 “指针类型(指向的数据类型)” 和 “指针名”
    struct nTask *pre;//前指针
    struct nTask *next;//后指针
    
};

struct nWorker//【柜员】👉组成-线程队列
/*单个执行线程的结构体的定义：
（准备以链表方式形成数据结构）
  存放1：存放'该线程的线程遥控器(线程ID)'；
  存放2：存放'终止数'；
  存放3：存放【大堂经理的联系方式】
  存放4：存放'前指针'；
  存放5：存放'后指针'； */
{
    pthread_t threadid;
    int terminate;
    struct nManager *manager;

    struct nWorker *pre;
    struct nWorker *next;
};

typedef struct nManager//【大堂经理】->【线程池】
/*单个管理者的结构体的定义：
（准备以链表方式形成数据结构）
  存放1：存放'【客户】的头指针'；
  存放2：存放'【柜员】的头指针'；
  存放3：存放'锁工具'； 
  存放4：存放'条件信号(什么时候用锁工具)'  */
{
    struct nTask *tasks;
    struct nWorker *workers;
    
    pthread_mutex_t mutex;
    pthread_cond_t cond;//condition(条件)，一种信号机制-条件变量，判断是否满足唤醒或还是等待的条件
}ThreadPool;

//-------------------------------------------------------------------------------------------

//（一.1）线程入口函数
//创建的每个【柜员】的新线程函数：传进来的是这个【柜员】的指针worker
static void* nThreadPoolCallback(void *arg)
/*static(静态的)【本质是让变量或函数，固定在当前的源文件、整个程序中，对小则变大，对大则变小】
 （1）修饰局部变量时 -> 让变量从“栈存储”变为“静态存储”，生命周期从局部延长至整个程序main()运行期间；
 （2）修饰全局变量时 -> 限制为内部链接性，限制其仅在当前文件（当前的源代码.c文件）可见，避免被其他文件
                       访问，未被static修饰的全局变量默认具有“外部链接性”，可通过extern声明来访问；
 
 （3）修饰函数时 -> 限制为内部链接性，关闭“外部链接性”，仅能在所定义其的源文件中被调用。  */
{
    struct nWorker *worker =(struct nWorker*)arg;
    //pthread_create让传进来的指针擦除为void*，所以必须强转一下，才能进行下一步！！

    while(1)//让这个线程不断地去“上锁 → 判断、取任务 → 解锁 → 执行任务”工作 √
    {
        pthread_mutex_lock(&worker->manager->mutex);//上锁，并调用锁的位置

        while(worker->manager->tasks == NULL) //当前【柜员】线程通过【大堂经理】不断确定有无task在队列中
        {   
            if(worker->terminate) break;//如果“终止数”=1，则跳出当前循环，不用再循环等待task了！

            //如果没有任务，就让该线程先“挂起”并释放锁
            pthread_cond_wait(&worker->manager->cond,&worker->manager->mutex);
            /* pthread_cond_wait(2个参数)返回0成功，非0失败
            作用：（1）使调用本函数的当前线程进入阻塞状态，等待条件变量cond唤醒；
                             （把该线程的ID等信息存储到pthread_cond_t这个排队“容器”中）
                  （2）释放传入的互斥锁指针所指向的互斥锁。
            参数1：pthread_cond_t*,指向条件变量的指针，用于线程间传递当前的条件状态
            参数2：pthread_mutex_t*,指向互斥锁的指针，必须是已经加锁的状态*/
        }
        if(worker->terminate) 
        /*如果“终止数”=1，就算当前有任务，也跳出当前循环，并解锁，不然就形成了“死锁”，因为前面上锁
          了，这里跳出最大的while(1)循环之前，肯定需要先解锁的！！！*/
        {
          pthread_mutex_unlock(&worker->manager->mutex);//解锁，并调用锁的位置，更新锁状态
          break;//跳出最大的循环
        }

        //【取任务】-从头开始取（类似于链表头删法）
        struct nTask *task = worker->manager->tasks;//传入【大堂经理】中【客户】的“头指针”
        LIST_REMOVE(task, worker->manager->tasks);//把该任务【客户】从 任务队列【客户列表】中移除

        pthread_mutex_unlock(&worker->manager->mutex);//解锁，并调用锁的位置，更新锁状态

        //【执行取出的任务】
        task->task_func(task);
      //通过task结构体指针调用里面的函数(函数指针)，这里传入的参数是task结构体的指针 √
      //相当于“执行任务”，这个任务的 函数 和 传参 都被封装在这个task结构体中了！！！

      /*--------------------对于【取任务】上锁，而【执行任务】却不上锁-------------------------
      【取任务】：任务队列是所有线程共享的（都可以通过链表访问），给取任务上锁是防止多个线程同时取一个
                 任务，不通过锁保护的话，会出现数据竞争，如：两个线程同时修改任务队列的头指针等；
      【执行任务】：任务本身是独立的，一旦任务被原子性地从共享任务队列中被取出，这个任务就变成了当前线
                   程的私有数据，这样一来就可与让各线程“并发性”地执行各个任务，最大化CPU利用率，称为
                   形成“池化”！！！  */
    }
    
    //【柜员下班】
    /*重置了terminate终止数，又通过broadcast唤醒的线程，都会依次抢夺锁然后跳出循环，然后执行👇 */
    //需要上锁： LIST_REMOVE(worker, worker->manager->workers);//释放线程队列里每个线程的前后指针
    free(worker);//释放内存
    worker = NULL;//指针置空

}

//（一）创建线程池API
int nThreadPoolCreate(ThreadPool *pool, int numWorkers)
{
    if(pool == NULL) return -1;//判定是否传入有效的线程池对象
    if(numWorkers < 1) numWorkers = 1;//确保工作线程数量至少为1

    //初始化外面main()函数创造的pool结构体
    memset(pool, 0, sizeof(ThreadPool));

    //初始化线程条件变量：
    pthread_cond_t blank_cond = PTHREAD_COND_INITIALIZER;
    /*PTHREAD_COND_INITIALIZER，系统(posix线程库)内部预先定义好的宏，用于初始化pthread_cond_t类型
      的变量，条件变量pthread_cond_t内部结构很复杂，利用宏可以把预先编写好的初始化代码替换过来。  */
    memcpy(&pool->cond, &blank_cond, sizeof(pthread_cond_t));//memcpy()传2个地址，1个步长
    /*再把这个初始化好的pthread_cont_t类型的变量blank_cond用memcpy()函数给“粗暴、拐弯地”拷贝给pool
      指针所指向的结构体的内部成员(pthread_cond_t类型的变量)cond，这时要改变这个cond的内容，就需要对
      这个cond取地址（&pool->cond），不然就只是访问它的变量值（pool->cond）而已 */

    // pthread_cond_init(&pool->cond, NULL); //一步到位
    /* pthread_cond_init(2个参数)：初始化pthread_cond_t类型的变量，返0成功，非0失败
       参数1：pthread_cond_t*,指向要初始化的pthread_cond_t类型的条件变量
       参数2：const pthread_condattr_t*,指向条件变量的属性控制结构体的指针，指定条件变量的属性，NULL
              表示使用默认属性 */

    //初始化ThreadPool结构体中的互斥锁：
    pthread_mutex_init(&pool->mutex,NULL);

    //用循环对传进来的可开辟numWorkers数量进行线程创建：
    int i = 0;
    for(i = 0; i < numWorkers; i++)
    {
        //申请【柜员结构体】
       struct nWorker *worker =(struct nWorker*)malloc(sizeof(struct nWorker));
      if(worker == NULL) 
      {
        perror("malloc");
        /*void perror(const char *s)：打印最近一次错误信息。这个函数会先打印const char *s所指向的
          非NULL字符串，然后自动打印一个冒号和空格，再输出对应的错误描述*/
        return -2;/*无论return嵌套多少层，直接结束当前函数的执行，并且固定值。
                    "return"退出当前函数👈|👉"exit"退出main()函数、退出整个程序*/
      }
      memset(worker, 0, sizeof(struct nWorker));//初始化malloc()函数所分配的【柜员】堆内存
      worker->manager = pool;//也给【柜员】留下当下【大堂经理】的联系信息

      //创建新线程，并把nThreadPoolCallback()函数作为入口函数，传入worker-柜员结构体指针作为参数：
      int ret = pthread_create(&worker->threadid, NULL, nThreadPoolCallback, worker);
      if(ret)
      {
        perror("pthread_create");//如果失败打印线程创造失败原因
        free(worker);//释放内存
        worker = NULL;//指针置空

        return -3;
      }

      //worker是新添加的【柜员】节点
      //workers是【柜员】的头指针
      LIST_INSERT(worker, pool->workers);//宏定义函数：把参数带入后，把代码复制过来 √
      /* 这里不需要加锁，理论上是因为：
             这个对【柜员队列】的插入操作只是在这个单一函数下的同一个循环里，并且每个循环里的
             pthread_create()函数所创建的线程入口函数里也并没有对该【柜员队列】的操作，所以
             对于【柜员队列】的插入这一操作来讲，它就是一个不会被多线程给同时争夺的资源。  
             在callback()函数里，也不过只是判定task==null，然后挂起等待而已，就算它抢夺CPU
             计算时间，最终也会形成：所有创建的新线程都挂起、所有【柜员】都成功插入【柜员队列】。
             */
    }

    return 0;//成功
}

//（二）销毁线程池【柜员下班】API
int nThreadPoolDestroy(ThreadPool *pool)
{
  struct nWorker *worker = NULL;//创建一个空的nWorker类型结构体指针，用来遍历“销毁”【柜员队列】
  for(worker = pool->workers; worker != NULL; worker = worker->next)
  {
    worker->terminate = 1;//把终止数设置为'1'，以此来跳出线程任务循环
  }

  pthread_mutex_lock(&pool->mutex);
  /* 这里为什么要给pthread_cond_broadcast()广播函数上锁！？又为什么必须和任务函数中“判断-取任务”的
     相关操作用“同一把锁”！？
     
     （1）给广播函数上锁，是为了防止当广播函数唤醒所有的线程后，还没来得及进行下一步，就又有新的线程
          通过pthread_cond_wait()函数进入了等待队列中；
     （2）用同一把锁，是为了让这个pthread_cond_broadcast()唤醒和pthread_cond_wait()判定等待这两个
         函数具有“互斥性”，也就是一个执行，另一个就必定不会执行，这样就一来可以防止逻辑漏洞，确保唤醒
         来重新竞争互斥锁的是“当前所有等待的线程”，二来可以防止用不同的锁产生互相锁在当前线程等待对方
         释放资源的“死锁”状态。  */

  /*用来叫醒还停在条件变量容器中的线程出来，以此执行后面的terminate判定，因为我前面已经循环重置了所有
    terminate“终止数”，后面给broadcast加锁，也算双重保障了 */       
  pthread_cond_broadcast(&pool->cond);
  /*pthread_cond_broadcast(1个参数) 返回0成功，非0失败
    作用：唤醒所有因调用pthread_cond_wait()函数而阻塞在某个pthread_cond_t条件变量容器里的线程！！
          被唤醒的线程，重新串行依次竞争互斥锁，如果抢到锁，抢到锁的线程被唤醒的线程返回线程函数后直接
          从断点Pthread_cond_wait()后面的代码开始执行，所以一般要把Pthread_cond_wait()函数写在一个
          while循环里，这样的话，在线程被唤醒后夺得锁以后，从断点pthread_cond_wait()函数开始往后继续
          执行时就会重新回到这个包裹住pthread_cond_wait()函数的while循环判断语句里,进行一系列执行任务
          前的检查。
    参数1：pthread_cond_t,指向一个条件变量（容器）类型的指针，并对里面的线程进行广播。  */

  pthread_mutex_unlock(&pool->mutex);

  //头指针置空，这里比较粗暴，因为每个线程也并不是按顺序执行链表的节点的移除的
  pool->tasks = NULL;
  pool->workers = NULL;

}

//（三）推送任务（线程池操作）API
int nThreadPoolPushTask(ThreadPool *pool, struct nTask *task)
{
//针对于【共享队列】的操作最好都加上锁，防止竞争访问
  pthread_mutex_lock(&pool->mutex);

  LIST_INSERT(task, pool->tasks);//先把这个任务插入到【任务队列中】

  //唤醒一个等待的进程来“取任务”
  pthread_cond_signal(&pool->cond);
  /* pthread_cond_signal(1个参数) 返回0成功，非0失败
   作用：从等待容器中唤醒一个线程，具体唤醒哪一个，取决于操作系统的线程调度策略，是不确定的，唤醒后
         同样从pthread_cond_wait()函数处返回，继续执行之后的代码；
   参数1：pthread_cond_t*,用于指定要唤醒的等待容器。  */

   pthread_mutex_unlock(&pool->mutex);
}




// sdk --> debug thread pool(线程池调试程序)

#define THREAD_QTY_INIT 20
#define TASK_QTY_INIT 1000

//编写任务函数：void(*task_func)(void *arg)
void task_entry(void* arg)
{
  struct nTask *task = (struct nTask *)arg;//强转后接收
  int idx = *(int *)task->user_data;
  //把接收来的task中user_data这个指针指向的内存解引用后赋值给idx，强转后才能解引用！！

  printf("idx: %d\n", idx);//打印 -> 每个线程的任务就是打印

  free(task->user_data);//打印完后释放task中的user_data所指向的动态分配的int类型堆内存
  free(task);//最后再释放这个取出的任务结构体本身（刚才在main函数里malloc的）
}

//（四）把各个API链接在一起，debug各个API的主程序：
#if 1
int main(void)
{
  ThreadPool pool;//初始化在nThreadPoolCreate()函数中

  nThreadPoolCreate(&pool, THREAD_QTY_INIT);//创建20个线程

  int i = 0;
  for(i = 0; i < TASK_QTY_INIT; i++)//循环创建1000个任务
  {
    struct nTask *task = (struct nTask *)malloc(sizeof(struct nTask));//创建堆内存
    if(task == NULL) //如果堆内存分配失败
    {
      perror("malloc");
      exit(1);//exit()非0表示异常退出
    }
    memset(task, 0, sizeof(struct nTask));//初始化动态分配的每一个任务结构体
    
    task->task_func = task_entry;//让任务队列内每一个任务的任务函数都指向task_entry()函数入口
    task->user_data = malloc(sizeof(int));//任务队列里的每一个用户数据指针分配一块堆区int大小内存

    *(int*)task->user_data = i;
    //强转int*，确定了指针类型才能解引用 -> 把i赋值给user_data所指向的内存（刚分配的）

    nThreadPoolPushTask(&pool, task);
    //每创建一个任务，就push推送一个任务（插入共享队列、调度线程来执行）

  }
  
  //主线程阻塞等待子线程👇
  struct nWorker* tool = NULL;//声明一块struct nWorker类型的指针，用来遍历线程ID
  //从【柜员】队列的头指针开始遍历
  for(tool = pool.workers; tool != NULL; tool = tool->next)
  {
    pthread_join(tool->threadid, NULL);//循环阻塞每一个子线程ID
  }
  
  return 0;
}

#endif