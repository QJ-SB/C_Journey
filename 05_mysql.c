// mysql

# include <stdio.h>
# include <string.h>
# include <mysql.h>
/* MySQL头文件，需要在相应的系统上安装"libmysqlclient-dev"，MySQL客户端库-
   开发包，安装后mysql.h等头文件会被复制到系统标准头文件路径里，才能成功编译 */

# define QJ_DB_SERVER_IP       "192.168.134.101"//数据库服务器的IP
# define QJ_DB_USERNAME        "admin"//要登陆的用户名
# define QJ_DB_PASSWORD        "123456"//密码
# define QJ_DB_DEFAULTDB       "QJ_DB"//要连接的数据库名
# define QJ_DB_PORT            3306//服务器端口号

//宏定义Mysql语句：
//插入：
# define SQL_INSERT_TBL_USER   "INSERT TBL_USER(U_NAME, U_GENDER) VALUES('QJ', 'MAN');" 
//查询：
# define SQL_SELECT_TBL_USER   "SELECT * FROM TBL_USER;"
//删除：
# define SQL_DELETE_TBL_USER   "CALL PROC_DELETE_USER('QJ');"
/* 这里要配合在mysql服务器端用上 CREATE PROCEDURE ... ()函数！
   然后只需要在代码端CALL这个函数就行，这样只需要一句执行语句！！！ */

//插入图片：
# define SQL_INSERT_IMG_USER  "INSERT TBL_USER(U_NAME, U_GENDER, U_IMG) VALUES('HJ', 'WOMAN', ?);"//这里的'?'是占位符
//查询图片：
# define SQL_SELECT_IMG_USER  "SELECT U_IMG FROM TBL_USER WHERE U_NAME='HJ';"

//
# define FILE_IMAGE_LENGTH    300*1024 //300Kb
//【select封装成函数】
int QJ_MYSQL_SELECT(MYSQL *handle)
{
   //(1)向mysql服务器发送select语句：
   if(mysql_real_query(
        handle,
        SQL_SELECT_TBL_USER,
        strlen(SQL_SELECT_TBL_USER)//“不包括'\0'的动态有效字符数”的字节数
    ))
    {
        printf("mysql_real_query:%s\n", mysql_error(handle));
        return -1;
    }

    //(2)储存执行select后返回的结果集：
    MYSQL_RES *res = mysql_store_result(handle);
    /* MYSQL_RES* mysql_store_result(1个参数)  
       成功则返回MYSQL_RES类型的结构体的指针，失败或无结果则返回NULL
       
       作用：用于获取SQL各种查询语句所返回的完整结果集，并在客户端（执行此程序
            的一端）缓存全部数据，方便下一步读取处理。同时生成MYSQL_RES类型的
            结果集对象（包含查询语句执行结果的各种元数据，包含列信息和列数量等）
       参数1：MYSQL*，指向MYSQL结构体（遥控器）的指针，代表已建立连接的数据库。
       */
    if(res == NULL) 
    {
      printf("mysql_store_result:%s\n", mysql_error(handle));
      return -2;
    }

    //(3)处理返回的结果集：
    //(3.1)存储返回结果的“行”、“列”总数
    int rows = mysql_num_rows(res);
    /* unsigned_long mysql_num_rows(1个参数)  返回结果集中的“行数” 
       
       作用：用于获取已查询结果集中的行总数。
       参数1：MYSQL_RES*，指向MYSQL_RES已存储结果集的指针。  */
   printf("rows:%d\n", rows);

   int fields = mysql_num_fields(res);
   /* unsigned int mysql_num_fileds(1个参数)  返回结果集中的“列数”  
      作用：用于获取已查询结果集中的列总数。
      参数1：MYSQL_RES*，指向MYSQL_RES已存储结果集的指针。  */
   printf("fields:%d\n", fields);

   //(3.2)打印每行的数据
   MYSQL_ROW row;
   /* MYSQL_ROW是一个字符串数组的指针，所指向的数组中，其中的每个元素对应该
               行中某一列的字段值：
      例如：row[0]、row[1]、row[2]可分别获取该行的第一、二、三列的字段。  */

   while(row = (mysql_fetch_row(res)))//返回NULL后停止=读到末尾停止
   /* MYSQL_ROW* mysql_fetch_row(1个参数)  
      返回MYSQL_ROW类型的指针，读到末尾则返回NULL

      作用：用于从结果集中逐行读取数据，每次调用只返回一行记录!
      参数1：MYSQL_RES*，MYSQL_RES结果集结构体的指针
   */
   {
      int i = 0;
      for(i = 0; i < fields; i++)//按列数来遍历，逐行读取的结果
      {
         printf("%s\t", row[i]);
      }
      printf("\n");//每打印完一行后换行！
   }

   //(4)释放结果集：
   mysql_free_result(res);
   /* void mysql_free_result(1个参数)  无返回值
      作用：释放MYSQL_RES结构体占用的内存
      参数1：MYSQL_RES*，MYSQL_RES结构体结果集的指针。  */

   return 0;
}



//【从磁盘读图片到缓冲区】
//filename: path + file's name(文件目录+名字)
//buffer: to store image(缓冲区)
int read_image(char *filename, char *buffer)
{
   if(filename == NULL || buffer == NULL) return -1;//首先检测参数
   
   FILE *fp = fopen(filename, "rb");
   //"rb(read binary)"以二进制方式打开并读取
   if(fp == NULL)
   {
      printf("fopen failed\n");
      return -2;
   }

   //将文件操作指针移动到文件末尾：
   fseek(fp, 0, SEEK_END);
   /* int fseek(3个参数)  返回0成功，非0失败
      作用：用来移动文件指针，核心作用是改变文件内部的读写位置

      参数1：FILE*，FILE类型结构体的指针，代表要操作的文件遥控器
      参数2：long int，表示偏移量，即从起始位置（由参数3指定）开始移动的字节数
            可以为正数（向文件末尾移动），也可以为负数（向文件开头移动）
      参数3：int，指定偏移量的起始位置，是一个整数类型，有三个预定义的常量可以
            使用：（1）SEEK_SET，表示从文件开头第一个有效字节开始（值为0）；
                 （2）SEEK_CUR，表示从文件指针当前位置开始（值为1）；
                 （3）SEEK_END，表示从文件末尾最后一个有效字节之后开始（值为2）。
   */

   //用一个int类型的length来接收文件的总字节数：
   int length = ftell(fp);
   /* long int ftell(1个参数)  
   返回当前文件指针(存储在FILE遥控器中的信息)，相对于文件开头的字节偏移量
   此偏移量 = “从文件第一个有效字节 到 当前指针位置 前的所有字节数”
   
   作用：用于获取文件指针当前的位置与文件开头的偏移量
   参数1：FILE*，FILE结构体类型文件遥控器的指针
   */

   //再把文件指针指回文件开头：
   fseek(fp, 0, SEEK_SET);

   //以1个字节为单位读取文件并存到缓冲区buffer：
   int size = fread(buffer, 1, length, fp);
   /* size_t fread(3个参数)  返回的是成功读取到的“数据项数量”而不一定是字节数！
      作用：将文件中的数据按单位量，连续读取并存储到缓冲区中

      参数1：void*，指向缓冲区buffer的指针，buffer的类型不确定所以void*
      参数2：size_t，一次要读取的单位数据项的字节大小（步长）
      参数3：size_t，要读取的数据项数量（步数）
      参数4：FILE*，FILE结构体类型文件遥控器的指针
   */
  if(size != length)
  {
      printf("fread failed:%d\n", size);
      return -3; 
  }
  fclose(fp);//关闭文件遥控器

  return size;//返回读取到的字节数
}

//【把图片从缓冲区写入磁盘】
//filenam：外部磁盘文件的目录地址
//buffer：缓冲区的地址
//length：要写入的字节长度
int write_image(char *filename, char *buffer, int length)
{
   //例行检测
   if(filename == NULL || buffer == NULL || length <= 0) return  -1;
   //以“写二进制并增加读写权限（w包含了若不存在则创建）”的形式打开磁盘文件
   FILE *fp = fopen(filename, "wb+");//'w+'表示对于打开的磁盘文件可写+读
   if(fp == NULL)
   {
      printf("fopen failed\n");
      return -2;
   }

   //把缓冲区中的数据写入外部文件磁盘,并接收返回值
   int size = fwrite(buffer, 1, length, fp);
   /* size_t fwrite(4个参数)  
   成功时返回实际写入的'数据项'，若失败返回值<第三个参数'要写入数据项的数量'

     作用：用于向文件中写入数据
     参数1：const void*，指向要写入的数据所在的内存地址（const防止函数修改地址）
     参数2：size_t，一次要写入的数据项的字节大小（步长）
     参数3：size_t，要写入的数据项数量（步数）
     参数4：FILE*，FILE结构体类型的文件遥控器指针
   */
  if(size != length)
  {
   printf("fwrite failed:%d\n", size);//失败打印
   return -3;
  }
  fclose(fp);//释放遥控器

  return size;//返回写入的'数据项'数量
}

//【把图片从缓冲区写入MYSQL服务器】
int mysql_write(MYSQL *handle, char *buffer, int length)
{
   if(handle == NULL || buffer == NULL || length <= 0) return -1;//检测

   //向'管道端口'申请一个初始化的MYSQL_STMT（预处理语句包）
   MYSQL_STMT *stmt = mysql_stmt_init(handle);
   /* MYSQL_STMT，包含预处理语句的结构体（就是把“要执行的语句”打包）
   
      mysql_stmt_init(1个参数) 
      成功返回一个MYSQL_STMT结构体的指针，失败返NULL
      作用：初始化预处理语句结构体
      参数1：MYSQL*，已建立连接的MYSQL结构体指针（mysql管道）
   */

   //把'语句'放入预处理语句包，并让MYSQL服务器进行逻辑预处理
   int ret = mysql_stmt_prepare(
      stmt, 
      SQL_INSERT_IMG_USER,
      strlen(SQL_INSERT_IMG_USER));
   /* mysql_stmt_prepare(3个参数)  返回0成功，非0失败
      作用：让数据库预解析带有'?'的SQL语句，但不执行，为后续动态传参做准备

      参数1：MYSQL_STMT*，'预处理语句包'的指针（已初始化）
      参数2：const char*，要预处理的SQL语句字符串（里面可包含'?'作为参数占位符）
      参数3：unsigned long，SQL语句的长度（字节数），一般用strlen()计算
   */
   if(ret)//判定mysql_stmt_prepare()函数执行结果
   {
      printf("mysql_stmt_prepare:%s\n", mysql_error(handle));
      return -2;
   }

   //初始化此结构体（形参绑定说明书 - 对于'小参数'直接写里面就行！！）
   MYSQL_BIND param = {0};//'parameter -> 形参'
   /* MYSQL_BIND结构体专门配合MYSQL_STMT(预处理语句包)结构体使用，作用是在
      C程序变量和SQL语句的参数or结果之间'搭桥'
      
      MYSQL_BIND结构体中包含的关键字段：
      （1）void *buffer,缓冲区指针'buffer'
      （2）enum_filed_types buffer_type，枚举变量'buffer_type',用于告诉
                                        MYSQL服务器，buffer缓冲区里的字段
                                        是什么MYSQL类型【告诉MYSQL传参类型】
      （3）unsigned long *length，无符号长整数类型的指针，传递or获取数据的实
                                 际动态长度（字节数）【更灵活】
      （4）my_bool *is_null，布尔类型变量的指针，用于标记所绑定的参数或结果集
                             中的字段是否为NULL：【为了避免NULL混淆导致错误】
                             用于'参数'：true -> 表示当前绑定的参数值为NULL
                                        false -> 表示当前绑定的参数值有效
                             用于'结果集'：true -> 表示当前字段的值为NULL
                                        false -> 表示当前字段值有效
      */
   param.buffer_type = MYSQL_TYPE_LONG_BLOB;
                       //告诉MYSQL传参类型为'大二进制对象'（预留好位置）
   param.buffer = NULL;//先将缓冲区信息指向NULL（不传小参数）
   param.is_null = 0;//把is_null布尔值置为假（传参内容不是NULL）
   param.length = NULL;//先把动态长度置为0（不传小参数这里自然也为NULL）

   //把形参也给STMT传过去（stmt bind param，语句包绑定形参）
   //（把'形参绑定说明书'传入'预处理语句包'）
   ret = mysql_stmt_bind_param(stmt, &param);
   /* mysql_stmt_bind_param(2个参数)  成功返回0，非0失败
      作用：建立参数关联，用于让“MYSQL_BIND'形参绑定说明书'中指向的buffer缓冲
      区中的'形参'”
         与 
      “之前让MYSQL服务器先预处理了代码逻辑的MYSQL_STMT'预处
      理语句包'中的'?'”一一对应，让MYSQL服务器知道每个'?'到底该填什么。

      参数1：MYSQL_STMT*，指向已初始化过的'预处理语句包'
      参数2：MYSQL_BIND*，指向一个'形参绑定说明书'   */
   if(ret)
   {
      printf("mysql_stmt_bind_param:%s\n", mysql_error(handle));
      return -3;
   }
   
   //给MYSQL具体传参 - 传大参数！！才多次调用这个函数，不然直接写BIND说明书里了
   ret = mysql_stmt_send_long_data(stmt, 0, buffer, length);
   /* mysql_stmt_send_long_data(4个参数)  成功返回0，失败非0
      作用：向MYSQL服务器分批地发送超长数据的参数【把一个参数分批传参给MYSQL】

      参数1：MYSQL_STMT*，指向已与处理好的'预处理语句包'
      参数2：unsigned int，要发送的长数据的参数序号（从0开始计数），对应着预处
                          理好的那些'?'的位置的参数
      参数3：const char*，指向要发送的长数据所存储在的缓冲区
      参数4：unsigned long，本次发送的数据总长度（字节数）
   */
   if(ret)
   {
      printf("mysql_stmt_send_long_data failed:%s\n", mysql_error(handle));
      return -4;
   }

   //（执行预处理好的SQL语句-执行函数）
   ret = mysql_stmt_execute(stmt);
   /* int mysql_stmt_excute(1个参数)  成功返回0，失败返回非0
      作用：用于执行已经预处理好的SQL语句
      参数1：MYSQL_STMT*，预处理语句包的指针
   */
   if(ret)
   {
      printf("mysql_stmt_excute failed:%s\n", mysql_error(handle));
      return -5;
   }

   //释放MYSQL_STMT结构体 - 堆内存
   ret = mysql_stmt_close(stmt);//MYSQL_BIND结构体通常是用户在栈上分配的，不需释放
   /* mysql_stmt_close(1个参数)  成功返回0，失败返回非0 
      作用：关闭'预处理语句包'
      参数1：MYSQL_STMT*，指向要关闭的'预处理语句包'  */
   if(ret)
   {
      printf("mysql_stmt_close failed:%s\n", mysql_error(handle));
      return -6;
   }

   return ret;
}

//【把图片从MYSQL服务器里取到缓冲区】
int mysql_read(MYSQL* handle, char* buffer, int length)
{
   if(handle == NULL || buffer == NULL || length <= 0) return -1;//检测

   //一样创建并初始化'预处理语句包' - 查询图片
   MYSQL_STMT *stmt = mysql_stmt_init(handle);
   int ret = mysql_stmt_prepare(
      stmt, 
      SQL_SELECT_IMG_USER,
      strlen(SQL_SELECT_IMG_USER));
   if(ret)//判定mysql_stmt_prepare()函数执行结果
   {
      printf("mysql_stmt_prepare:%s\n", mysql_error(handle));
      return -2;
   }

   //再定义一个'结果集绑定说明书'
   MYSQL_BIND result = {0};
   result.buffer_type = MYSQL_TYPE_LONG_BLOB;
                       //接收的结果集类型为'大二进制对象'
   result.buffer = NULL;//先将缓冲区信息指向NULL
   result.is_null = 0;//把is_null布尔值置为假（结果集内容不是NULL）
   unsigned long total_length = 0;//定义一个无符号长整数
   result.length = &total_length;//执行后的结果集实际总字节数

   //将'预处理语句包'的处理结果绑定到'结果集绑定说明书'中
   //【这里不同于绑定'形参'，这里只是定义，把 结果集说明书 和 预处理语句包
   //  联系在一起】
   mysql_stmt_bind_result(stmt, &result);
   /* mysql_stmt_bind_result(2个参数)  成功返回0，失败返回非0
      作用：用于将预处理语句的查询结果绑定到变量

      参数1：MYSQL_STMT*，指向'预处理语句包'
      参数2：MYSQL_BIND*，指向'结果集绑定说明书'   */
      if(ret)//判定mysql_stmt_bind_result()函数执行结果
   {
      printf("mysql_stmt_bind_result:%s\n", mysql_error(handle));
      return -3;
   }

   //（执行预处理好的SQL语句-执行函数）
   ret = mysql_stmt_execute(stmt);
   if(ret)//判定
   {
      printf("mysql_stmt_excute:%s\n", mysql_error(handle));
      return -4;
   }
   
   //把预处理语句的执行结果集一次性拉取 ↓（从MYSQL服务器到本地内存中）
   //这里的stmt已经完全定义好了，BIND也绑定好了 √
   ret = mysql_stmt_store_result(stmt);
   /* mysql_stmt_store_result(1个参数)  返回0成功，非0失败
      作用：用于将预处理语句的执行结果集存储到客户端内存中
      参数1：MYSQL_STMT，预处理语句包的指针   */
   if(ret)//判定
   {
      printf("mysql_stmt_store_result:%s\n", mysql_error(handle));
      return -5;
   }

   //用循环把结果集打印出来
   while(1)
   {
      //【先按照'整行/次'拉取- 执行后返回的结果集】
      //把本地内存中的结果集逐行拉取进已定义好的缓冲区 √
      ret = mysql_stmt_fetch(stmt);
      /* mysql_stmt_fetch(1个参数)  
      返回0则成功拉取一行数据，并写入绑定缓冲区
      返回-1（MYSQL_NO_DATA）宏定义值，所有数据已读取完毕
      返回1（MYSQL_DATA_TRUNCATED）宏定义值，数据被截断
      返回其他数，则执行失败

      作用：逐行读取预处理语句执行后的结果集数据
      参数1：MYSQL_STMT*，预处理语句包的指针   */
      if(ret != 0 && ret != MYSQL_DATA_TRUNCATED) break;
      //允许"执行成功"和"读取数据被截断"继续执行循环，直到"读取成功"或"读取失败"
      //👇👇👇
      //【再按'第几列/次'进行分片覆盖！！！】
      //【只能"行 + 列分片"，因为MYSQL结果集就是遵循“行优先！！！”】
      int start = 0;//设置一个 “起始位的偏移量（一开始为0）”
      while(start < (int)total_length)//从0开始，刚好循环'total_length'次
      /* MYSQL_BIND.length是执行预处理语句包后的结果集实际总字节数！！ */
      {
         result.buffer = buffer + start;
         /*缓存区指针从传入的buffer开始，每次后移'起始位偏移量'  */
         result.buffer_length = 1;
         /* unsigned buffer_length是MYSQL_BIND结构体中的缓存区最大
            存储字节数，用于限制存储的最大数据量，无论实际Length是多少
            
            这里相当于每次只缓存1字节  */

         //拉取当前行的第0、也就是第1列，进行缓存区数据覆盖
         mysql_stmt_fetch_column(
            stmt,
            &result,
            0,//拉取'当前行'第0、也就是第一列'
            start//同上，起始位偏移量
         );
         /* mysql_stmt_fetch_column(4个参数)  返回0成功，非0失败
            作用：从预处理语句的执行结果集中读取指定'列数据'到缓冲区

            参数1：MYSQL_STMT，预处理语句包的指针
            参数2：MYSQL_BIND，结果集绑定说明说的指针
            参数3：unsigned int，要读取的列（从0开始表示第一列）
            参数4：unsigned long，读取数据起始点的偏移量
                     （计数从0开始，从该列的第'参数4'个字节开始拉取）
         */
        start += result.buffer_length;
        //最后让 “起始位偏移量” 每次+ “最大缓存字节数(分片量)”（这里是+1）
      }
   }
   mysql_stmt_close(stmt);//关闭句柄

   return total_length;//返回结果集实际总字节数
}

//【主函数】
int main()
{
    //------------------------------------------------------------------
    //【一、操作前准备】
    //创建mysql服务器的'管道端口' √
    MYSQL mysql;
    /* 创建一个MYSQL类型的结构体变量mysql,这个结构体就像一个交互容器，所有和
       mysql服务器的通信（连接、执行SQL语句等）都需要通过它来完成。 */
    //👇   
    //初始化MYSQL结构体'管道端口' √
    if(NULL == mysql_init(&mysql))
    /* mysql_init(1个参数) 成功后返回MYSQL结构体的指针，失败则返回NULL
       作用：初始化MYSQL结构体
       参数1：刚才创建的MYSQL结构体的指针(地址) */
    {
        printf("mysql_init:%s\n", mysql_error(&mysql));
        /* mysql_error(1个参数) 返回const char*类型的错误信息字符串
           作用：在libmysqlclient库中用于获取最近一次数据库操作错误信息
           参数1：必须传入已执行初始化命令的MYSQL类型结构体的指针(地址)*/
           goto Exit;
    }
    //👇
    //连接目标mysql服务器 √
    if(!mysql_real_connect(
        &mysql,
        QJ_DB_SERVER_IP,
        QJ_DB_USERNAME,
        QJ_DB_PASSWORD,
        QJ_DB_DEFAULTDB,
        QJ_DB_PORT,
        NULL,
        0
      ))
      {
        printf("mysql_real_connect:%s\n", mysql_error(&mysql));
        goto Exit;
      }
    /* mysql_real_connect(8个参数)  
       成功则返回指向MYSQL结构体的指针（与参数1相同），失败则返回NULL

       作用：（1）与指定的Mysql服务器建立网络连接，并验证用户身份、密码；
            （2）指定连接的相关参数。
       参数1：MYSQL*，已初始化的MYSQL结构体的指针
       参数2：const char*，'服务器主机名'or'IP地址'（NULL表示'本地'）
       参数3：const char*，登录用户名
       参数4：const char*，登录密码
       参数5：const char*，要连接的数据库名（NULL表示'不指定'）
       参数6：unsigned int，服务器端口号（0 表示默认端口号3306）
       参数7：const char*，要连接的套接字路径（NULL表示'不使用'）
                            (套接字用于进程间通信)
       参数8：unsigned long，客户端标志（0 表示默认）
                    (用于设置客户端连接mysql服务器时的特定功能选项) 
    */
    //-----------------------------------------------------------------

    //【二、操作mysql数据库】

    #if 1
    //@插入操作：
    printf("case: mysql --> insert\n");
    if(mysql_real_query(
        &mysql,
        SQL_INSERT_TBL_USER,
        strlen(SQL_INSERT_TBL_USER)//“不包括'\0'的动态有效字符数”的字节数
    ))
    {
        printf("mysql_real_query:%s\n", mysql_error(&mysql));
        goto Exit;
    }
    /* mysql_real_query(3个参数) 成功返回0，失败返回非0
       作用：向Mysql服务器发送并执行SQL语句
       
       参数1：MYSQL*，已初始化的MYSQL结构体(遥控器)的指针；
       参数2：const char*，'要执行的SQL语句的字符串的'指针；
       参数3：unsigned long，SQL语句的长度(以字节为单位)。
    */
    
    #endif
    //@查询操作：
    QJ_MYSQL_SELECT(&mysql);//select查询函数

    printf("case: mysql --> delete\n");

    #if 1
    //@删除操作：
     if(mysql_real_query(
        &mysql,
        SQL_DELETE_TBL_USER,
        strlen(SQL_DELETE_TBL_USER)//“不包括'\0'的动态有效字符数”的字节数
    ))
    {
        printf("mysql_real_query:%s\n", mysql_error(&mysql));
        goto Exit;
    }

    #endif
    //@查询操作：
    QJ_MYSQL_SELECT(&mysql);//select查询函数

    //@【1】读取图片：
    printf("case: mysql --> read image from disk and write into MysqlSever\n");
    //创建一块缓冲区，字符串数组，长度为64kb(64*1024个元素，每个元素1字节)
    char buffer[FILE_IMAGE_LENGTH] = {0};
    int length = read_image("count.png", buffer);
    /* 这里也可以不写'绝对路径'，因为编译后的程序和这个PNG文件在同一目录下*/
    if(length < 0) goto Exit;//判定一下

    //【2】写入图片(到数据库)
    mysql_write(&mysql, buffer, length);

    //【3】读取图片
    printf("case: mysql --> read image from MysqlSever and write into disk\n");
    memset(buffer, 0, FILE_IMAGE_LENGTH);//把用过的缓冲区重新置空
    length = mysql_read(&mysql, buffer, FILE_IMAGE_LENGTH);

    //【4】写入图片(到硬盘)
    write_image("a.png", buffer, length);
    //-----------------------------------------------------------------

    //【三、操作后善后】
    Exit: 
      mysql_close(&mysql);
    /* mysql_close(1个参数)  无返回值
       作用：用于关闭与mysql服务器的网络连接，并释放MYSQL结构体的内存

       参数1：指向MYSQL结构体的指针(遥控器)。*/

    return 0;
}


    