//---------------------------------------------------------------------------------------------
//-----------------------------------------头文件👇--------------------------------------------
//---------------------------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <errno.h>
#include <unistd.h>

#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>


//---------------------------------------------------------------------------------------------
//-----------------------------------------宏定义👇--------------------------------------------
//---------------------------------------------------------------------------------------------
#define MAX_BUFFER		128          
#define MAX_EPOLLSIZE	(384*1024)   //384kb
#define MAX_PORT		100

#define TIME_SUB_MS(tv1, tv2)  ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000)


//---------------------------------------------------------------------------------------------
//-----------------------------------------全局变量👇-------------------------------------------
//---------------------------------------------------------------------------------------------
int isContinue = 0;


//---------------------------------------------------------------------------------------------
//-----------------------------------------其他函数👇-------------------------------------------
//---------------------------------------------------------------------------------------------
//追加套接字描述符设置状态为，非阻塞：
static int ntySetNonblock(int fd) {
	int flags;

	flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) return flags;
	flags |= O_NONBLOCK;//"|=" -> 按位或标志，加一个或情况（或值）
	if (fcntl(fd, F_SETFL, flags) < 0) return -1;
	return 0;
}


//避免旧套接字释放后，因地址暂时'被占用'而导致的新套接字绑定失败：
static int ntySetReUseAddr(int fd) {
	int reuse = 1;//开启

	//将已释放的套接字所绑定过的'ip地址'和'端口'设置为"可复用"状态（一般会处于TIME_WAIT状态）
	return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
	/* int setsockopt(5个参数)   成功返回0，失败返回-1
	   作用：用于动态修改套接字的属性
	   参数1：int，需要设置选项的套接字描述符
	   参数2：int，选项所在的协议层
	   参数3：int，要设置的具体选项名称
	   参数4：const void*，指向这个选项对应的传参的缓存区
	   参数5：socklen_t，存放这个参数的缓存区长度    */
}


//---------------------------------------------------------------------------------------------
//-----------------------------------------主函数👇-------------------------------------------
//---------------------------------------------------------------------------------------------
int main(int argc, char **argv) {
	if (argc <= 2) {
		printf("Usage: %s ip port\n", argv[0]);
		exit(0);
	}

//----------------------------------------终端传参转换👇-----------------------------------------
	const char *ip = argv[1]; //IP字符串隐式退化为首地址，后赋值给指针ip
	int port = atoi(argv[2]); //ASCII to int, port端口字符串转化为int


//------------------------------------一些局部变量初始化👇---------------------------------------
	int connections = 0;      //总连接数置0
	char buffer[128] = {0};   //缓存区128个字符
	int i = 0, index = 0;     //循环用数、端口偏移量


//----------------------------------创建events事件发生簿👇---------------------------------------
	struct epoll_event events[MAX_EPOLLSIZE];//创建epoll事件组 -> 可容纳384*1024=393216个epoll事件


//----------------------------------创建epoll监控事件表👇---------------------------------------
	int epoll_fd = epoll_create(MAX_EPOLLSIZE);//创建epoll监控表，返回其描述符
	
	strcpy(buffer, " Data From MulClient\n");//把这玩意复制到buffer中

	
//------------------------------创建服务器套接字地址并初始化👇-------------------------------------
	//为后面connect做准备（因为循环创建的fd都要用同一个协议、连接同一个ip，所以这些定义放外面）
	//                                                               （port递增放循环里）
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;                     //定义地址族
	addr.sin_addr.s_addr = inet_addr(ip);          //赋值ip


//---------------------------------------创建初始时间容器👇---------------------------------------
	//存储1970.1.1到现在的s秒数+us微秒数：
	//后面计算时间差用得到
	struct timeval tv_begin;
	gettimeofday(&tv_begin, NULL);
	/* int gettimeofday(2个参数)   成功返回0，失败放回-1
	   作用：获取当前时间
	   参数1：struct timeval*，时间间隔结构体体的指针，用于存储获取到的时间（微秒级）
	   参数2：struct timezone*，用于存储时区信息，现代系统中通常设为NULL（忽略，已被废弃）  */


//-----------------------开始循环：创建fd、连接、发送、注册入epoll👇-------------------------------
	while (1) {


    //-----------------------------------准备工作👇-------------------------------------------
		if (++index >= MAX_PORT) index = 0;    //如果index没加到100（从1开始）：
		                                       //端口号的拓展数index加到100后再次置0

		//创建1个epoll事件结构体和fd置空 -> 为后面循环注册epoll用：
		struct epoll_event ev;    
		int sockfd = 0;


    //----------------------------------当连接数 < 34w👇---------------------------------------
		//int connectiongs = 0, isContinue = 0
		//连接数<340000,为的是3台客户端加一起能到100w：
		if (connections < 340000 && !isContinue) {


        //--------------------------------创建本地TCP-fd👇------------------------------------
			sockfd = socket(AF_INET, SOCK_STREAM, 0);   //创建套接字
			if (sockfd == -1) {
				perror("socket");
				goto err;
			}

			//每创建一个fd，请求的服务器端口号累加：
			addr.sin_port = htons(port+index);//host to networks,
                                              //服务器套接字地址中,传入的服务器port+1


            //---------------------------建立TCP连接-并配置fd模式👇-------------------------------
			//连接服务器"ip：相应的累加port"：
			if (connect(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0) {
				perror("connect");
				goto err;
			}
			//connect成功后↓【加快连接的速度 -> 实现“高并发非阻塞 + 异常后地址复用”】
			//设置：本地套接字-非阻塞 && 复用（套接字异常后已释放的套接字）所绑定过的地址（ip+port）：
			ntySetNonblock(sockfd);
			ntySetReUseAddr(sockfd);


            //------------------------------向服务器发送数据👇----------------------------------
			//按照以下格式打印"已连接数"，并把这个打印字符串覆盖存到buffer里：
			sprintf(buffer, "Hello Server: client --> %d\n", connections);
			//然后把buffer里的内容发给服务器：
			send(sockfd, buffer, strlen(buffer), 0);


            //-------------------------------fd注册入epoll👇--------------------------------------
			//把该已建立连接并且已发送数据的套接字（以及对应要监控的epoll事件），注册入epoll中：
			ev.data.fd = sockfd;//已建立连接的fd记录进epoll事件中
			ev.events = EPOLLIN | EPOLLOUT;//监控'可读'和'可发送'
			epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev);
			connections ++;//已建立连接数 +1
		}
		

		//------------------------'每1000次连接' 或 '连接 > 34w'👇--------------------------------
		//【999、1999、...、338999、339999，每隔'1000'次连接 || 总连接数已超过340000】
		if (connections % 1000 == 999 || connections >= 340000) {


		//--------------打印：总连接数、本次创建的fd、1000次连接用时（ms）、👇-----------------------
			//创建一个新的时间容器，并装入原来记录过的时间：
			struct timeval tv_cur;
			memcpy(&tv_cur, &tv_begin, sizeof(struct timeval));
			//原来记录过的时间复制后，记录新的"此刻"：
			gettimeofday(&tv_begin, NULL);

			//记录'1000'次执行所用时间(ms毫秒为单位)：
			//秒s *1000=ms，微秒um /1000=ms:
			int time_used = TIME_SUB_MS(tv_begin, tv_cur);
			printf("connections: %d, sockfd:%d, time_used:%d\n", connections, sockfd, time_used);


			//-----------------------每1000次连接-wait检测监控事件👇-----------------------------
			//等待阻塞100ms，并检测一遍所有已连接的connections，让有事件发生的都可返回结果进events：
			int nfds = epoll_wait(epoll_fd, events, connections, 100);//返回发生事件的fd数量
			for (i = 0;i < nfds;i ++) {


				//----------------把有事件发生的fd每次取出一个（分成两种情况）👇-----------------------
				int clientfd = events[i].data.fd;

				/*'&'按位与：对比二者底层二进制的bit位是否同时为1，同时为1则该位置为1，其他位都为0
				j      组成一个新的值  【在判断标志位时，核心就是检测“包含关系”】👇  */
				//      ↓ ↓ ↓
				/*EPOLLIN底层bit位：0001
				  EPOLLOUT底层bit位：0100
				 （struct epoll_event）ev.events里是'按位或'逻辑，一个标志位的集合0101
				  如果events是0001，则按位与为'真'，如果events是0100：则按位与也为'真'：*/
				//      ↓ ↓ ↓
				

       			//----------（1）EPOLLOUT -> 有内核发送缓冲区空闲，都用对应fd发一次数据👇------------------------
				//【本质上是检测本位events中是否包含EPOLLOUT】:
				if (events[i].events & EPOLLOUT) {
					sprintf(buffer, "data from %d\n", clientfd);
					send(clientfd, buffer, strlen(buffer), 0);
				} 


				//---------（2）EPOLLIN -> 内核接收缓冲区中有数据，用当前fd接收-并打印👇-----------------				
				//【本质上是检测本位events中是否包含EPOLLIN】:
				else if (events[i].events & EPOLLIN) {
					char rBuffer[MAX_BUFFER] = {0};	//初始化一个128字符的缓冲区	
					ssize_t length = recv(clientfd, rBuffer, MAX_BUFFER, 0);//接收数据
					//如果收到数据 -> 则打印：
					if (length > 0) {
						printf(" RecvBuffer:%s\n", rBuffer);
						//如果收到的是"quit" -> 则isContinue = 0:
						if (!strcmp(rBuffer, "quit")) {
							isContinue = 0;
						}
					}
					//如果对方已断开连接 -> 总连接数-1、并关闭该fd：
					else if (length == 0) {
						printf(" Disconnect clientfd:%d\n", clientfd);
						connections --;
						close(clientfd);
					} 
					//在（length < 0）的情况下：
					else {
						//失败原因是ENTER则继续循环（ENTER不是错误，只是系统调用被临时打断）
						if (errno == EINTR) continue;

						printf(" Error clientfd:%d, errno:%d\n", clientfd, errno);
						close(clientfd);
					}
				}
				//--------------------（3）既不是EPOLLIN又不是EPOLLOUT👇--------------------------
				else {
					printf(" clientfd:%d, errno:%d\n", clientfd, errno);
					close(clientfd);
				}
			}
		}

		usleep(1 * 1000);    //每次while前让系统休息1ms
	}
	return 0;

err:
	printf("error : %s\n", strerror(errno));
	/* char* strerror(1个参数)   返回一个指向人类可读描述符错误码的字符串指针
	   作用：将错误发转换为人类可读、易读的错误信息
	   参数1：int，通常输入全局变量errno。  */
	return 0;
}



