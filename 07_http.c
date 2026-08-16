/* TCP编程 --> http报文 --> 客户端和服务器间通信和数据交换  */

# include <stdio.h>
# include <stdlib.h>
# include <string.h>

# include <sys/socket.h>//套接字库头文件
# include <arpa/inet.h>//IP地址转换库头文件
# include <netinet/in.h>//网络变成头文件
# include <netdb.h>//网络数据库操作头文件

# include <unistd.h>//Unix标准库头文件
# include <fcntl.h>//文件描述符控制头文件


//定义http报文缓存区大小：
# define BUFFER_SIZE    4096 //4kb

//定义http版本号：
#define HTTP_VERSION    "HTTP/1.1"//1.1版本，目前应用最广泛

//定义一个http头部字段：connection -> 控制客户端与服务器之间的连接状态
#define CONNECTION_TYPE "Connection:close\r\n"//表示请求处理完后关闭TCP链接


//【'域名'转'点分十进制IP地址' -> 返回IP的指针】：
char *host_to_ip(const char *hostname)
{
    //解析域名：
    struct hostent *host_entry = gethostbyname(hostname);
    /* struct hostent* gethostbyname(1个参数)【该函数仅支持IPv4地址！】 
    返回值：struct hostent*是指向域名所对应的服务器(主机)的相关网络信息的
           结构体，包括对应主机的：
           （1）官方域名：         (char *)h_name
           （2）别名字符串数组：    (char **)h_aliases
           （3）地址类型：          (int) h_addrtype（固定为AF_INET-IPv4）
           （4）IP地址长度：        (int) h_length
           （5）IPv4地址列表数组：   (char **)h_addr_list👇
          (每个IP地址指针所指向的IP地址以32位（unsigned int）网络字节序存储)
    失败返回NULL

    作用：将域名转换为对应的IPv4地址
    参数1：const char*，指向需要解析的域名   */

    if(host_entry != NULL)
    {
        //对hostent中的h_addr_list解引用（得到指向第一个IPv4地址的指针）
        // -> 强转类型（转为一个也指向32位网络字节序的IPv4地址的指针）
        // -> 解引用（强转为struct in_addr*后，再解引用得到struct in_addr）
        // -> 带入函数（带入struct in_addr结构体类型的变量）
        // -> 返回最终指针（指向解析好的IP地址字符串）
        return inet_ntoa(*(struct in_addr*)*(host_entry->h_addr_list));
     /*（1）struct in_addr是sockaddr结构体（套接字地址）中的成员，它里面存
            储的IP地址就是以32位网络字节序(无符号整型)的形式存储的  
       
       （2）inet_ntoa(1个参数) 【network to ASCII】
            返回值：char*，指向'点分十进制字符'的指针
            作用：将网络字节序的IPv4地址转换为人类可读的'点分十进制'字符串
            参数1：struct in_addr，网络字节序IPv4地址-结构体
                                          （本质是32位的unsigned int）
     */
    }

    return NULL;
}

//【创建本地套接字并建立TCP网络链接 -> 返回套接字描述符】：
int http_create_socket(char *ip)
{
    //创建TCP-本地套接字，并接收套接字ID：
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) return -1;//判定返回值非负

    //创建目标套接字地址：
    struct sockaddr_in sin = {0};
    sin.sin_family = AF_INET;
    sin.sin_port = htons(80);//http协议报默认端口
    sin.sin_addr.s_addr = inet_addr(ip);//点分十进制IP -> 32位网络字节序

    //建立TCP链接：
    if(0 !=connect(
        sockfd,//套接字标识符
        (struct sockaddr*)&sin,//目标套接字地址指针
        sizeof(struct sockaddr_in)//目标套接字地址长度
    )) return -1;//判定失败与否

    //【fcntl() -> file control-'文件'控制函数】
    /*文件描述符FD->File descriptor
               Unix/Linux-'一切皆文件': 无论是底层的硬件、软件、通信管道等
                                       都被抽象化地封装位'文件'
    （而通过这个'文件描述符'就可以唯一地操作这个'文件'，本质上就是ID）
    （！！！覆盖了所有的I/O资源的交互！！！）  */

    //将本地套接字设置为：非阻塞模式（支持高并发）
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    /* int fcntl(2个固定参数 + n个可选参数) 返回值取决于参数2对应的操作命令 
       作用：👇（各种各样）

       参数1：int，文件描述符（套接字描述符是其中的一种 -> 进程or网络I/O）
       参数2：int，宏定义好的操作命令：
                 （1）F_GETFL：获取 文件描述符对应的'文件' 的状态标志👇
                 （2）F_SETFL：设置 文件描述符对应的'文件' 的状态标志👇
                                    （套接字描述符也是一种文件描述符）
       【'文件'的状态标志flags：只读、只写、读写、阻塞、非阻塞、异步等】
       【这里实际上就是：设置套接字（网络接口）的类型】

                 （3）F_GETLK：检查 套接字ID 对应的文件的锁状态
                 （4）F_SETLK：设置或释放 套接字ID 对应的文件锁
                 （5）F_SETLKW：上一条加一个'阻塞'直到锁可用

                 （6）F_GETFD：获取文件描述符标志
                 （7）F_SETFD：设置文件描述符标志
                 （8）F_DUPFD：复制文件描述符标志
        参数3：可选参数'arg' -> 根据参数2的操作命令来搭配👇
    （这里的参数3 -> 'open_noneblock' -> 让这个套接字(网络接口)为非阻塞模式）
    */

    //返回一切弄好的套接字描述符：
    return sockfd;
    //【创建本地套接字->创建目标套接字地址->建立链接->设置本地套接字模式 √】
}

//【发送 并 接收 http协议报文 - 返回接收数据的指针】
//如：www.github.com/QjRepository -> [域名/资源]
char *http_send_request(const char *hostname, const char *resource)
{
    //域名 -> 点分十IP（指针）：
    char *ip = host_to_ip(hostname);

    //创建TCP网络链接：
    int sockfd = http_create_socket(ip);

    //创建缓存区 -> 存储http报文：
    char buffer[BUFFER_SIZE] = {0};

    //编辑http报文：👇
    //请求方法：用于指定客户端对服务器资源的操作 -> 常见的如GET：获取资源，等
    /*
    URL：Uniform Resource Locator -> 统一资源定位符：
    协议+用户名:密码+域名/ip+端口(可选)+路径(path-具体资源位置)+查询参数+片段
    【但在http请求报文中仅以 "路径+查询参数" 的形式出现】
    */
    //\r\n：回车符（回到该行起点）+ 换行符（光标下一行）-> 相当于重起一行
    //续行符：相当于“\ + 终端敲入的回车”，编译时把 '\ + 回车' 给略过
    //头部字段名+值：用于描述报文属性、传递附加信息等
    //http报文格式👇
    /* 
       请求方法|空格|URL|协议版本|\r\n|续行符     -----【起始行】
       头部字段名|:|值|\r\n|续行符               -----【头部字段起始】
       ...
       ...
       头部字段名|:|值|\r\n|续行符               -----【头部字段结束】
       主体部分(可选)|\r\n                      -----【主体部分(可选)】             
    */
    sprintf(
        buffer,
        "GET %s %s\r\n"\
        "Host:%s\r\n"\
        "%s\r\n"\
        "\r\n",
        resource,
        HTTP_VERSION,
        hostname,
        CONNECTION_TYPE
    );
    /* int sprintf(1个固定参数 + n个可变参数)
    成功时返回写入字符串的字符总数（ASCII下刚好是字节数）
    失败返回负数  
       作用：将多种不同类型的数据，拼接成一个字符串

       参数1：char*，存储格式化后结果的字符串指针
       参数2：const char*，要格式化字符串的样式"..."
       参数n：...配合参数2的参数
       */

    /* send()函数要先connect()建立链接，sendto()函数手动填目标地址，但是
       先connect()了以后照样可以用sendto()函数，不过这时会忽略sendto()函
       函中指定的目标地址，而是使用connect()函数中已绑定的目标地址！！   */

    //发送http报文：
    send(
        sockfd,//本地套接字id
        buffer,//http报文缓存区指针
        sizeof(buffer),//缓存区大小
        0//发送标志
    );
    /* ssize_t send(4个参数) 成功返回实际发送的字节数，失败返回-1
       作用：向已建立链接的对端的套接字发送数据

       参数1：int，已建立号连接的套接字描述符
       参数2：要发送数据的缓存区的指针
       参数3：要发送数据的实际长度（字节数）
       参数4：发送标志flags，通常设为0
       */

    //设置文件描述符集合（底层是bid_array-比特位数组，通常上限是1024位）：
    fd_set fd_read;
    /* fd_set(file desriptor_set-文件描述符集合)数据类型只要用于select()
       函数，核心作用是存储一组文件描述符fd,供给select()函数检测这些描述符
       是否有可以读、可以写或异常事件发生等 
       -> 实现多路I/O复用(用一个线程就可以同时监测多个I/O源)   */

    //初始化文件描述符集合：
    FD_ZERO(&fd_read);
    /* FD_ZERO()是一个宏函数，其本质是宏替换
       作用：初始化文件描述符集合
       参数1：fd_set*，指向一个fd_set类型的数据  */
    
    FD_SET(sockfd, &fd_read);
    /* FD_SET(2个参数)也是一个宏函数
       作用：向文件描述符集合中添加指定的文件描述符（一个非负的整数n），添加
       后fd_set中对应的第n个二进制位(bit位)会被设为'1'，表示这个值n所对应的
       文件描述符被监控，也即是监控着这个文件描述符所对应的I/O

       参数1：int，要添加的文件描述符ID（如套接字文件描述符等）
       参数2：fd_set*，指向要添加进的文件描述符集合的指针  */

    //创建时间间隔结构体：
    struct timeval tv;
    tv.tv_sec = 5;    //5秒
    tv.tv_usec = 0;   //0微秒

    //先创建一块堆内存，用来接收buffer的数据
    char *result = malloc(sizeof(int));//先分一小块
    memset(result, 0, sizeof(int));//初始化

    //循环 + 等5秒的select -> 周期性地每次监测5秒文件描述符集合:
    //并且http是流式数据报，一般要多次接收recv():
    while(1)
    {
        /* 前面用fcntl()函数设置了套接字非阻塞，如果这里反而采取select()函
           数的多路监测阻塞（参数5 = NULL），这样的核心思路就是：
           “以单线程高效监测多路I/O、多事件，select相当于【智能监控中心】” 
        */
        //配置select()多路I/O'可读'监测：
        int selection = select(
            sockfd+1,   //要监测的最大的文件描述符值+1（select遍历范围）
            &fd_read,      //监测'可读事件'（这些I/O是否收到数据）
            NULL,       //监测'可写事件'（这些I/O是否可以发送数据）
            NULL,       //监测'异常事件'（这些I/O是否异常、出错）
            &tv         //最大等待时间：5秒
        );
        /* int select(5个参数) 
        成功返回触发事件的文件描述符总个数
        失败返回-1
        超时返回 0（无事件发生）
                        ↓ ↓ ↓
           作用：同时监控多个文件描述符（监测文件描述符集合），并且监控对应
                 的集合中的文件描述符是否发生了对应参数位的事件：
                 '2-可读'：这个套接字是否收到数据
                 '3-可写'：这个套接字是否可以发数据
                 '4-异常'：这个套接字是否异常（除了2、3外的情况）
                        ↓ ↓ ↓
           参数1：int，'需要监测的最大的文件描述符的值+1'（用于确定遍历范围）
           参数2：fd_set*，要监测的"可读事件"的文件描述符集合的指针
           参数3：fd_set*，要检测的"可写事件"的文件描述符集合的指针
           参数4：fd_set*，要监测的"异常事件"的文件描述符集合的指针
           参数5：struct timeval*，时间间隔结构体指针 -> 超时时间设置
                                 （1）非NULL：设置几秒后返回
                                 （2）NULL：一直阻塞，直到有事件发生
                        ↓ ↓ ↓      
           struct timeval是用于表示'时间间隔（秒s和微妙us级精度）'，用于
           select()函数中用于指定'最大等待时间：
           strcut timeval
           {
                time_t tv_sec;         //time interval_second -> 秒
                suseconds_t tv_usec;   //time interval_usecond -> 微秒          
           }
       */

       //如果'无事件发生'or'没发生事件已被移除（在拷贝的副本里）'
        if(!selection || !FD_ISSET(sockfd, &fd_read))
        /* int FD_ISSET(2个参数)  
        若该fd在集合中返回非0（真），反之返回0（假）

           作用：检查参数1对应的文件描述符是否还在参数2对应的文件描述符集合的
                拷贝副本中👇
                select()在执行前，会先拷贝一份源集合，生成一份副本
                select()在返回时，会清空这份副本的集合中中未触发事件的fd,
                只保留触发了事件的fd，所以此时检测还在副本fd_read集合中的fd
                就是触发了'可读'事件的fd。
           参数1：int，要检查的文件描述符
           参数2：fd_set*，要检查的文件描述符集合的指针  */
        {
            break; //没事发生就跳出循环ba
        }
        else//否则有'可读'事件发生，当然就接收数据咯
        {
            //清空一下刚从用来发送的缓存区，接下来用来接收了：
            memset(buffer, 0, BUFFER_SIZE);

            //接收数据：
            int len = recv(sockfd, buffer, BUFFER_SIZE, 0);
            /* ssize_t recv()  
            成功返回实际接收的字节数，失败返回-1，若链接被对方关闭返回0

               作用：从指定的已建立链接(connect)的套接字入口接收数据
               参数1：int，本地已创建链接的套接字描述符
               参数2：void*，指向接收数据的缓存区的指针
               参数3：size_t，缓存区大小（字节数）
               参数4：接收状态标志，通常设为0

               ！！！
              （这个recv()函数和recvfrom()函数的区别，就像send()函数和
                sendto()函数的区别一样，不加'from'、'to'就代表发送或接收
                数据前就已经connect()过了，不需要目标服务器的套接字地址
                结构体指针和相应的套接字地址结构体的大小了，自然少了2个参数）
            */
            if(0 == len)
            {
                break;//disconnect-对方已断开链接
                /* 因为我们前面编辑并发送的http报文中的头部字段选择了
                   connect: close，所以服务端会在发送完报文后主动关闭
                   链接，这里以len == 0（对方关闭链接）为退出条件是可
                   以的   */
            }

            /*直接用原来的指针来接受扩容后的内存地址，不过万一扩容失败返
               回NULL,原来的内存块就丢失了，所以这是危险的简化做法：  */
            result = realloc(
                    result,
                    (strlen(result) + len +1)*sizeof(char)
                    /* strlen()算的是有效字符数，所以一开始是0，并且http报
                       文是流式数据报，也就是说随着while循环不断地计算上一
                       次存入的有效字符 再+ 新一次接收的len 最后再补+1，这样
                       就可以动态地扩容需要的内存，并且始终留出'\0'的位置 ，
                       因为http报文本身不包含C中的'\0'。

                       最后*sizeof(char)是为了代码可读性，表示这是按照字符数
                       进行计算的（实际是字节数）
                    */
                );
            /* void* realloc(2个参数) 
            成功返回新内存块指针，失败返回NULL原内存块保持不变
               作用：修改已分配内存块的大小，实现动态扩容或缩容

               参数1：void*，指向之前通过mallo()函数分配的内存块的指针
               参数2：size_t，新的内存块大小（以字节为单位）
            */

            //把新接收到的缓冲区里的数据拼接到刚扩容好的result里
            //拼过去len个字符，刚好每次都预留出'\0'的位置：
            strncat(result, buffer, len);
            /* char* strncat(3个参数)  返回要拼接过去的目标字符串的指针 
               作用：有长度限制地拼接字符串，并在拼接后自动添上'\0'！！！

               参数1：char*，要拼接过去的目标字符串指针
                                （算上自动补'\0'的空间得够）
               参数2：const char*，源字符串的指针
               参数3：size_t，要复制并拼接的字符数
                             （或在复制到源字符串的'\0'前自动停止）
            【与strcat()函数区分，这个没有字符数限制，'cat'表拼接】
            */
        }
    }
    return result;//返回最后收齐数据的堆内存指针
}

//【主函数】
int main(int argc, char *argv[])
{
    if(argc < 3) return -1;

    //【argv[1]->hostname、argv[2]->resource】
    char *response = http_send_request(argv[1], argv[2]);

    //打印结果：
    printf("response:%s\n", response);

    free(response);//释放堆内存
    response = NULL;//指针置空

    return 0;
}