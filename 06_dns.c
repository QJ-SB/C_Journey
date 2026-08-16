/* UDP编程 --> 请求DNS --> 翻译域名为IP地址*/

/*所有发给DNS服务器的'>1字节的参数'都要用htons()函数转换端序，而域名-字符串
  不需要，是因为字符串的本质就是'单个字节的数组'，不存在端序问题，本身就按照
  字符串中的字符排列  */

//-----------------------------------------------------------------------------------------
//--------------------------------------头文件👇-------------------------------------------
//-----------------------------------------------------------------------------------------
# include <stdio.h>//标准输入输出
# include <string.h>//字符串处理
# include <stdlib.h>//标准库
# include <time.h>//time()函数

//UNIX和类UNIX系统中socket编程（定义了各种结构体和操作函数）：
# include <sys/socket.h>
//Advanced Research Projects Agency(高级研究计划局-网络)
//主要提供了地址转换函数：
# include <arpa/inet.h>
//UNIX标准头文件：
# include <unistd.h>


//-----------------------------------------------------------------------------------------
//----------------------------------------宏定义👇-----------------------------------------
//-----------------------------------------------------------------------------------------
# define DNS_SERVER_PORT    53 //53-DNS端口
# define DNS_SERVER_IP      "114.114.114.114"
//中国移动、电信、联通公用的公共域名解析服务器DNS地址


//-----------------------------------------------------------------------------------------
//-------------------------定义DNS报文header-头部域 和 query-查询域👇------------------------
//-----------------------------------------------------------------------------------------
//【DNS协议下的报文的'头部(header)'】-unsigned short(2字节-16bits)
//作用：描述DNS报文的基本属性
struct dns_header
{
    unsigned short id;//会话标识（用于识别）
    unsigned short flags;//标志位（标记报文是请求、响应、查询等类型）

    unsigned short questions;//问题数（要查询的问题数）
    unsigned short answers;//回答数（DNS服务器返回的回答数量）

    unsigned short authority;//授权（授权服务器数量）
    unsigned short additional;//附加信息（附加信息数量）
};
//【DNS协议下的报文的'查询域'】
/*作用：承载客户端向DNS服务器发送的查询请求
  封装了：（需要解析域名、想要获取的资源的记录类型、所属的网络协议类别）*/
struct dns_queries
{
    char *q_name;//域名指针（指向要查询的域名）
    int length;//域名长度-[这个是自己加的，为了方便后续填充，后续DNS报文中不需要！]

    unsigned short q_type;//查询类型（指定要查询的资源的记录类型，方便查询）
    unsigned short q_class;//查询类（用于指定查询时适用的网络协议环境
                           //，通常是'IN',internet）
};


//-----------------------------------------------------------------------------------------
//---------------------填充DNS报文header-头部域 和 query-查询域👇----------------------------
//-----------------------------------------------------------------------------------------
//【DNS头部填充函数】
int dns_create_header(struct dns_header *header)
{
    //函数传参必先判定！
    if(header == NULL) return -1;
    memset(header, 0, sizeof(struct dns_header));

    //设置'随机函数'的'随机种子'：
    srandom(time(NULL));
    /* void srandom(1个参数)  无返回值
       作用：初始化设置随机生成数函数rand()的种子(起始数)
       参数1：unsigned int，无符号整数的'seed-种子'
    
       time_t time(1个参数)  
       返回time_t类型的时间值（本质上是一串整数，从1970.1.1到此刻秒数）
       作用：配合srandom()函数使用
       参数1：time_t*，时间戳：（1）非NULL，指向时间类型变量
                              （2）NULL，指向当前的系统时间    */

    //随机命名'会话标识'：
    header->id = htons(random());
    /* long random(0个参数)  返回long类型的伪随机整数  */

    //标志位确定：（网络通信协议规定了统一的字节序 - 大端序）
    //规定好'客户端'和'DNS服务器'之间的沟通模式、规则
    header->flags = htons(0x0100);
    /* 【host to networks,主机序到网络序】
       unsigned short htons(1个参数)  返回网络字节序的16比特位无符号整数
       作用：将采用'小端序(倒记、倒读)'记录的数据转换为'大端序(正记、正读)'
             把主机能懂的'倒序'转换为网络通用的'正序'。
       参数1：unsigned short，主机字节序的16为无符号整数  */ 

    //每次查询1个问题（1项解析请求 - 通常对应1个域名）
    //只针对某1个特定对象或特定资源记录类型的请求
    header->questions = htons(1);

    return 0;
    /* 至于剩下的header中的其他子内容：
       （1）answer：DNS服务器响应查询后返回的结果，DNS处理完查询，获取到
                   对应的DNS资源记录后，才会填充这部分内容，客户端发送查
                   询报文时，这部分就是空值；
       （2）authority：当DNS服务器无法直接提供查询结果，但可以给出对该域名
                      的授权服务器信息（更上层DNS服务器）时，才会填充这一
                      部分内容；
       （3）additional：通常包含与回答部分相关的一些额外信息，特定情况下才
                        会填充。   
    【以上3部分都属于'被动填充'，等服务器响应后才会被动填充，不用主动设置！】*/   
}
//【DNS查询域填充函数】
int dns_create_queries(struct dns_queries *queries, const char* hostname)
{
    if(queries == NULL || hostname == NULL) return -1;//判定传参
    memset(queries, 0, sizeof(struct dns_queries));//初始化查询域

    //给查询域中的域名指针动态分配一块堆内存('由hostname指向的字符数+2'个字节)
    queries->q_name = (char*)malloc(strlen(hostname) + 2);
    if(queries->q_name == NULL) return -2;

    //把除了delim分割符以外的'头'和'尾'都算上（+2），后面要用上这个length！！！
    queries->length = strlen(hostname) + 2;

    queries->q_type = htons(1);//查询类型'1' -> 表示由域名获得其IPv4地址
    queries->q_class = htons(1);//查询类'1' -> 表示网络环境为IN,internet

    //把queries结构体里的q_name重命名一下，方便操作（准备给它赋值）：
    char *qname = queries->q_name;
    //复制一份域名（用来分割）：
    char *hostname_dup = strdup(hostname);
    /* char* strdup(1个参数)  成功返回新复制的内存的指针，失败返回NULL
       作用：复制字符串并返回备份的地址
       参数1：const char*，指向要被复制的字符串的指针  
       
    （strdup()函数内含malloc()函数，所以切记free()这块堆内存！！！）  */

    //创建待会要用的'分割符'的字符串数组：
    const char delim[2] = ".";
    //分割域名：
    char *token = strtok(hostname_dup, delim);
    /* char* strtok(2个参数) 
    返回一个指向'被分割出来的标记(token)'的指针 √
    如果没有更多标记可提取，字符串已被分割完毕，则返回NULL √
       作用：用于将一个字符串按照指定的'分隔符'拆分成多个'子字符串(token)'

       参数1：char*，指向要被分割的字符串的指针
       参数2；const char*，指向包含'分割符'的字符串，用于指定分割的依据，这
             个字符串中任意单位字符都作为分割的标志：
（如：delim = " ,\t\n" -> 表示'空格''逗号'、'制表符'、'换行符'都作为分割符）
    */

    while(token != NULL)
    {
        size_t len = strlen(token);//取第一个token的大小size(字符数)
        *qname = len;//先把第一个数字解引用后放进去：如“3www60voice3com0”
        qname++;//让这个指针指向下一个字符位置

        //把'www\0'给复制到qname中：
    //（因为前面strlen()把'\0'给截了，所以这里要+1，加回来，保证qname的完整）
        strncpy(qname, token, len+1);
        /* char* strncpy(3个参数)   返回一个指向目的地字符串的指针   
          作用：按照指定的字符数量将一个字符串的内容复制到另一个字符串中，
                相比于strcpy()函数更安全，前者一直复制到'\0'为止，后者
                则可以手动控制要复制的字符数总数。

           参数1：char*，指向要复制到的目的地字符串的指针(目的地位置要够)
           参数2：const char*，指向源头字符串的指针
           参数3：size_t，表示总共要复制的最大字符数  */
        qname += len;//再把指针向后移动'len'位（此时实际指向了'\0'）

        //继续分割：（直到返回NULL后 -> 跳出此while循环）
        token = strtok(NULL, delim);
        /* 后续再调用strtok()函数，同一字符串如果还未被处理结束，strtok()
           函数内部会保存一个静态指针（记住上一次分割的位置），所以当后续再
           调用此函数并且参数1 -> NULL时，函数会使用内部保存的这个静态指针，
           继续从上一次分割结束的地方开始找下一个token并分割出来。
           假设第二次调用传的是一个新的字符串地址，则strtok()函数会丢弃之前
           保存的状态。  */
    }

    //手动添加域名结束符'0'，如：“3www60voice3com0”
    *qname = 0;

    free(hostname_dup);//释放备份地址
    return 0;
}


//-----------------------------------------------------------------------------------------
//---------------------打包填充好的header和query ---> DNS报文👇-----------------------------
//-----------------------------------------------------------------------------------------
//【打包DNS协议报】
int dns_build_request(
    struct dns_header *header,//头部 
    struct dns_queries *queries,//查询域
    char *request,//打包好的发送请求
    int length//request的长度
)
{
    if(header == NULL || queries == NULL || request == NULL) return -1;
    memset(request, 0, length);//初始化

    //【复制头部】👇
    //把header先copy进去：
    memcpy(request, header, sizeof(struct dns_header));
    //计算出copy后的内存'偏移量'：
    int offset = sizeof(struct dns_header);

    //【复制查询域】👇
    //把'q_name'copy进去：
    //【这里前面填充查询域时候的length（+2那个），不就用上了吗！！！】
    memcpy(request + offset, queries->q_name, queries->length);
    //再累加上-再次copy后的内存'偏移量'：
    offset += queries->length;

    //把'q_type'copy进去：
    memcpy(request + offset, &queries->q_type, sizeof(queries->q_type));
    //再累加上-再次copy后的内存'偏移量'：
    offset += sizeof(queries->q_type);

    //把'q_class'copy进去：
    memcpy(request + offset, &queries->q_class, sizeof(queries->q_class));
    //再累加上-再次copy后的内存'偏移量'：
    offset += sizeof(queries->q_class);

    return offset;//返回整个request的长度
}


//-----------------------------------------------------------------------------------------
//----------------------创建UDP套接字 ---> 发送打包好的DNS报文👇-----------------------------
//-----------------------------------------------------------------------------------------
//【客户端向服务器发送DNS协议报-基于UDP协议】
int dns_client_commit(const char* domain)
{
    //【创建本地的网络通信出口-套接字】
    //sockfd(socket file descriptor - 套接字文件描述符-ID)
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    /* int socket(3个参数)  
    成功返回一个非负整数：套接字描述符-套接字的ID；
    失败则返回-1。
       作用：创建一个套接字对象(网络通信接口)，实现进程间网络通信 √

       参数1：int，指定'地址族' -> 指定了套接字使用的网络协议类型：
             (AF:adress family) （1）AF_INET: IPv4协议
                                （2）AF_INET6: IPv6协议
                                （3）AF_UNIX: 本地进程间通信
       参数2：int，指定'套接字类型' -> 决定数据传输的方式：
                                 （1）SOCK_STREAM: 流式套接字（TCP协议）
                                 （2）SOCK_DGRAM: 数据包套接字（UDP协议）
                                         （datagram-数据报）
       参数3：int，指定具体'协议' -> 通常设为0，自动匹配前面的设置。  */
    if(sockfd < 0) return -1;

    //【目标套接字(网络接口)地址说明书】
    struct sockaddr_in serv_addr = {0};//IPv6是sockaddr_in6
    /* sockaddr_in结构体类型 -> 
             专门用于描述 要连接的服务器 的IPv4地址和端口等信息
    （socket address internet -> 套接字地址-IPv4-定义结构体）  */

    serv_addr.sin_family = AF_INET;//地址族-IPv4
    serv_addr.sin_port = htons(DNS_SERVER_PORT);//目标端口号-DNS-53【转序】
    serv_addr.sin_addr.s_addr = inet_addr(DNS_SERVER_IP);//ip地址【转序】
                        /* in_addr_t inet_addr(1个参数)  
                         成功返回：转换后的32位网络字节序整数形式的IPv4地址
                         失败返回：INADDR_NONE（0xffffffff）
                         作用：将十进制IP地址 转换为 32位网络字节序整数
                         参数1：const char*，指向一个十进制的IP地址   */

    //先发一次信息给目标服务器，并绑定好IP和端口
    if (0 != connect(
        sockfd,
        (struct sockaddr*)&serv_addr,
        sizeof(struct sockaddr_in)
    )) return -2;
    /* int connect(3个参数)  成功返回0，失败返回-1 
       作用：
       （1）在TCP套接字中：客户端通过该函数向服务器发起连接请求，与服务器的
                          监听套接字建立TCP连接（通过TCP协议建立的可靠、有
                          序的双向通信链路）-（TCP 三次握手）
       （2）在UDP套接字中：客户端系统会给UDP套接字绑定一个目标IP和端口，但是
                          不会建立链接状态。
       参数1：int，本地套接字描述符
       参数2：const strut sockaddr*，指向目标套接字地址
       参数3：socklen_t，目标套接字地址长度
       */
    
    //【头部-header】
    struct dns_header header = {0};//初始化一个header
    dns_create_header(&header);//填充header

    //【查询域-queries】
    struct dns_queries queries = {0};//初始化一个queries
    dns_create_queries(&queries, domain);//填充queries

    //创建一个打包缓存区：
    char request[1024] = {0};//1024字节完全够放header+queries

    //【打包-header、queries】
    int length = dns_build_request(
        &header,//头部地址
        &queries,//查询域地址
        request,//打包地址
        sizeof(request) //打包地址长度
    );

    //发送到DNS协议报到DNS服务器
    int slen = sendto(
        sockfd,//本地套接字描述符
        request,//发送数据缓存区的指针
        length,//发送总字节数
        0,//发送标志
        (struct sockaddr*)&serv_addr,//(传大类)目标套接字地址的指针
        sizeof(struct sockaddr_in)//目标套接字地址的长度（字节数）
    );
    /*ssize_t sendto(6个参数)  成功返回实际发送的字节数，失败返回-1
       作用：用于发送数据报，主要用于无连接的UDP协议

       参数1：int，套接字描述符-套接字ID（已创建并绑定的sockt）
       参数2：const void*，指向要发送的数据的缓存区
       参数3：size_t，要发送数据的总字节数
       参数4：int，发送标志（flags），指定发送数据的方式，通常为0
       参数5：const struct sockaddr*，指向目标套接字地址
       参数6：socklen_t，表示目标套接字地址的长度(字节数)
            （struct sockaddr长度类型）
    */

    //创建一个接收的缓存区：
    char response[1024] = {0};
    //初始化一个套接字地址说明书-用来接收发送方的信息
    struct sockaddr_in addr = {0};
    //创建一个套接字地址类型的指针（为了让返回的数据进行填充）
    socklen_t addr_len = sizeof(addr);

    int n = recvfrom(
        sockfd,//本地套接字描述符
        response,//接收数据缓存区的指针
        sizeof(response),//接收缓存区的总字节数
        0,//接收标志
        (struct sockaddr*)&addr,//(传大类)目标套接字地址的指针
        &addr_len//目标套接字地址的长度（字节数）
    );
    /* ssize_t recvfrom(7个参数) 成功返回实际接收的字节数，失败返回-1
       作用：用于从指定的套接字接收数据，同时获取发送方的地址信息
       
       参数1：int，套接字描述符-套接字ID（已创建并绑定的sockt）
       参数2：void*，指向用于存储接收数据的缓存区
       参数3：size_t，该缓存区的大小（字节数）
       参数4：int，接收标志（flags），指定接收数据的方式，通常为0
       参数5：struct sockaddr*，指向数据发送方套接字地址
       参数6：socklen_t*，套接字地址长度类型指针（为了填充）
       */
      
    //打印接收字节数和内容
    printf("recvfrom: %d, %s\n", n, response);

    //创建一个for循环来逐字符地打印response缓存区的内容：
    int i = 0;
    for(i = 0; i < n; i++)
    {
        printf("%c", response[i]);//%c: 逐字符
    }
    printf("\n");//打印完后换行

    //创建一个for循环来逐字符地(以十六进制)打印response缓存区的内容：
    i = 0;
    for(i = 0; i < n; i++)
    {
        printf("%x", response[i]);//%x: 以十六进制
    }
    printf("\n");//打印完后换行

    return n;//返回接收字节数
}


//-----------------------------------------------------------------------------------------
//-------------------------------------入口-主函数👇----------------------------------------
//-----------------------------------------------------------------------------------------
//【主函数】
//argc: argument count-参数计数
//argv: argument vector-参数向量（数组）
int main(int argc, char *argv[])
{
    if(argc < 2) return -1;//至少传2个参数

    //【向DNS服务器发送DNS协议报（包含域名）-请求解析】
    dns_client_commit(argv[1]);//传执行程序后的第一个参数-域名
}