//学习程序架构的层次，自下而上，倒着写。以及学习各种“磁盘文件操作”函数 √

//【头文件：】
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//【宏定义：】
//宏定义通讯录结构体中"name"和"phone"的字符串长度：
#define NAME_LENGTH 32
#define PHNOE_LENGTH 32

//宏定义重命名"printf"函数：
#define INFO printf

//宏定义缓冲区长度
#define BUFFER_LENGTH 128

//宏定义“插入”和“删除”通讯录结构体元素！！！
//【要注意，宏定义函数 ≠ 一般函数，宏本质上是对当前代码里宏定义的位置进行“文本替换”，所以访问的是“实参”】
//"item"是要操作（插入、删除）的元素节点（需要解引用以修改结构体内容）！！！
//"list"是通讯录结构体的头指针的实参（可以直接修改头指针的值）！！！
//👇
//(1)宏定义“插入-头插法”链表：
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
//(2)宏定义“删除”链表：
//双向链表的删除，一般就是："前指后后、后指前前"
//而在一般情况下，头节点和尾节点分别是两个特殊情况
//其中“前指后后”如果是头节点就不行，因为头节点的前是NULL
//其中“后指前前”如果是尾节点就不行，因为尾节点的后是NULL
//最后删除头节点的情况需要对头节点进行更新√
//"item"是要操作（插入、删除）的元素节点（需要解引用以修改结构体内容）！！！
//"list"是通讯录结构体的头指针的实参（可以直接修改头指针的值）！！！
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

//----------------------------预处理👆------------------------------
//-----------------------------代码👇-------------------------------

//【底层数据结构层】
//单个联系人节点的结构体的定义（双向链表）：
struct person//【节点结构体】
{
    char name[NAME_LENGTH];//联系人名字字符串数组
    char phone[PHNOE_LENGTH];//联系人电话字符串数组

    struct person *pre;//前指针
    struct person *next;//后指针
};
//把上面👆这个结构体进一步 “封装” 入下面这个结构体：
//访问通讯录节点的结构体的封装定义：
struct contacts//【管理结构体】
                 //管理结构体按照数据结构的具体格式，对节点结构体进行管理及访问其内的具体内容 √
{
    struct person *people;//头指针(链表里第一个节点的地址)
    int quantity;//通讯录内人员总数
};

//end----------------------------------------------------------------

//【interface接口交互层：API，下面是代码核心区】
//"ps"是要操作的元素，"people"是头指针
//👇
//(1)人员插入接口：
//头指针用二级指针接收，因为要改变头指针本身的值（即头指针的指向！）
/*而非头指针只需要传一级指针，因为只是要改变这个指针所指向的结构体内容
（即通讯录底层结构体里的前后指针的值（即前后指针的指向！））*/
int person_insert(struct person **ppeople, struct person *ps)
{
    if(ps == NULL) return -1;//检测传参
    LIST_INSERT(ps, *ppeople);
    //@对传入的头指针的二级指针解引用以此修改一级指针的值！！！
    return 0;
}
//(2)人员删除接口：
int person_delete(struct person **ppeople, struct person *ps)
{
    if(ps == NULL) return -1;
    LIST_REMOVE(ps, *ppeople);
    //@对传入的头指针的二级指针解引用以此修改一级指针的值！！！
    return 0;
}
//(3)人员查找接口：
struct person* person_search(struct person *people, char *name)
{
    struct person *item = NULL;
    for(item = people; item != NULL; item = item->next)
    {
        if(!strcmp(name, item->name))
        break;
    }
    return item;
}
//(4)人员遍历接口：
int person_traversal(struct person *people)
{
    struct person *item = NULL;
    for(item = people; item != NULL; item = item->next)
    {
        INFO("name:%s,phone:%s\n", item->name, item->phone);
    }
    return 0;
}
//(5)存储操作块(不需要接口层了!!!)：
//people(头指针)，filename(储存目的文件名)：
int save_file(struct person *people, const char *filename)
{
    FILE *fp = fopen(filename, "w");
    if(fp == NULL) return -1;
    //循环写入保存：
    struct person *item = NULL;
    for(item = people; item != NULL; item = item->next)
        {
            fprintf(fp, "name:%s, phone:%s\n",item->name, item->phone);
            fflush(fp);//刷新缓冲区并使其强制刷新写入外部磁盘
        }
    fclose(fp);//关闭fopen()
    
    return 0;
}
//(6)加载操作块(不需要接口层了！！！)：
//a.将缓冲区的数据解析：
int parser_token(char *buffer, int length, char *name, char *phone)
{
    //"name:%s, phone:%s\n",以下先存前半段：
    if(buffer == NULL) return -1;//检查应传入的非NULL指针
    int i = 0;//循环遍历载体
    int j = 0;//同上
    int status = 0;//状体机模式的状态表示值
    for(i = 0; i<length && buffer[i] != ','; i++)//外面的while和这里的循环判定是分开的
    {
        if(buffer[i] == ':')
        {
            status = 1;
            continue;//执行':'的下一个参数去
        }
        if(1 == status)
        {
            name[j++] = buffer[i]; 
        }
    }
    name[j] = '\0';//字符串封底
    //"name:%s, phone:%s\n",再先存后半段：
    j = 0;//再置0，复用内存(注意不用再申明类型、重定义了！！！)
    status = 0;//同上
    for(i; i < length; i++)
    /* 注意这里传入的长度不包括\0！！但BUFFER_LENGTH有128位，好像也没关系
    了，在那之前肯定已经读到'\n'了，所以这里关于length的判定不需要太严肃 */
    {
        if(buffer[i] == ':')
        {
            status = 1;
            continue;//执行':'的下一个参数去
        }
        if(1 == status)
        {    
            phone[j++] = buffer[i];
        }
        phone[j] = '\0';//字符串封底
    }

    return 0;
}

//b.将数据载入缓冲区（逐行）：
int load_file(struct person **ppeople,int *quantity, const char *filename)
{
    FILE *fp = fopen(filename, "r");//读取磁盘中的文件，创建文件流
    if(fp == NULL) return -1;
    //(方法一)会多判定一次，，，
    // while(!feof(fp))//检测具体文件执行函数是否已结束
    // /*feof()函数通过fp指针来检测FILE结构体中的内容，而这个内容随着这一
    // 代码循环体里的具体对文件磁盘有操作的函数（fgets,fgetc）等的操作而变化
    // 当这些函数把文件操作到结尾的时候，feof()函数就会返回非0，否则返回0*/
    
    // /*注意！feof()函数并不会再刚读到EOF的时候立刻返回非0，而是函数如：
    // fgetc()、fgets()已经返回EOF的下一次，再尝试操作文件才会返回非0*/

    char buffer[BUFFER_LENGTH] = {0};//每次最高读128位
    while(fgets(buffer, BUFFER_LENGTH, fp) != NULL)//用fget()返回的指针来判定循环
    {   /* char* fgets(char *str, int n, FILE *stream)（逐行读取）
        参数1：写入目的地；参数2：读取上限；参数3：FILE结构体指针；
        每读到'\n'就会把之前的数据一起读入缓冲区,并在末尾加上'\0',
        并且立即结束fgets()函数的执行，而第二个参数n是读取上限(读到
        n-1个字节)，而'\n'的优先级高于n!!!
        所以fgets()函数有三个终止条件：'\n',n(第二个传参)，EOF */

        buffer[strcspn(buffer, "\n")] = '\0';//!!!把读取入缓冲区的'\n'去掉
        //string character span,返回所查数组元素之前所有元素的数量之和，刚好等价于数组下标

        //定义两个数组分别用来存从缓存读取数据的数组：
        char name[NAME_LENGTH] = {0};
        char phone[PHNOE_LENGTH] = {0};
        
        if(0 != parser_token(buffer, strlen(buffer), name,phone))
        {
            continue;
        /* 相当于每次读“一行”，当解析失败时继续返回while判定，继续读取
        文件数据，直到fgets()函数执行到EOF */
        }

        memset(buffer, 0, sizeof(buffer));
        //每次复制完缓存区数据，清空缓存区，此函数用于把数组统一设置为同一个元素
        
        //每次读取的一行内容，申请一块struct person结构体来存储：
        struct person *p = (struct person*)malloc(sizeof(struct person));
        if(p == NULL) return -2;
        
        //分别把读取的"name"和"phone"存储到（p指针）结构体里：
        //void *memcpy(void *dest, const void *src, size_t)
        //            (目标内存地址，  源内存地址，    字节数)
        memcpy(p->name, name, NAME_LENGTH);
        memcpy(p->phone, phone, PHNOE_LENGTH);

        //调取person_insert函数来执行把这个（p指针）结构体插入链表：
        person_insert(ppeople, p);
        (*quantity)++;     //每插入链表一个新的结构体，并实现计数++
    }
    fclose(fp);

    return 0;
}    

//end----------------------------------------------------------------

//【portal界面层端口，接收从端口传入的终端数据，以此下一步传给API，进入核心代码区】
//(1)插入端：
int insert_entry(struct contacts *cts)
{
    //检测传参
    if(cts == NULL) return -1;
    //分配一块"person"结构体内存（并由p指向）
    struct person *p = (struct person*)malloc(sizeof(struct person));
    if(p == NULL) return -2;

    //需要插入的name输入：
    INFO("Please input name:\n");
    scanf("%s", p->name);
    //需要插入的phone输入：
    INFO("Please input phone:\n");
    scanf("%s", p->phone);
    //插入新成员：
    //"people"是通讯录结构体的头指针，"p"是刚才创建的结构体指针
    //这里要修改的是people（这个头指针）本身的值，所以传二级指针
    if(0 != person_insert(&cts->people, p))
    {
        free(p);
        return -3;
    }
    cts->quantity++;
    INFO("name:%s,phone:%s\nIsertion Success~\n", p->name, p->phone);

    return 0;
}
//(2)打印端：
int print_entry(struct contacts *cts)
{
    //检测传参：
    if(cts == NULL) return -1;
    //传入头指针：
    person_traversal(cts->people);

    return 0;
}
//(3)删除端：
int delete_entry(struct contacts *cts)
{
    //检测传参：
    if(cts == NULL) return -1;
    INFO("Please input name:\n");
    char name[NAME_LENGTH];
    scanf("%s", name);
    //先用查找函数查找一下要删除的人员是否存在（若存在，指针是多少？）：
    struct person* ps = person_search(cts->people, name);
    if(ps == NULL)
    {
        INFO("Don't exist!");
        return -2;
    }
    //利用"头指针"和"查找函数返回的需要删除的人员的指针"进行下一步：
    person_delete(&cts->people, ps);
    free(ps);
    cts->quantity--;
    INFO("deletion succeed~\n");

    return 0;
}
//(4)查找端：
int search_entry(struct contacts *cts)
{
    //检测传参：
    if(cts == NULL) return -1;
     INFO("Please input name:\n");
    char name[NAME_LENGTH];
    scanf("%s", name);
    //先用查找函数查找一下要删除的人员是否存在（若存在，指针是多少？）：
    struct person* ps = person_search(cts->people, name);
    if(ps == NULL)
    {
        INFO("Don't exist!");
        return -2;
    }
    INFO("name:%s,phone:%s\n", ps->name,ps->phone);

    return 0;
}
//(5)写入端：
int save_entry(struct contacts *cts)
{
    if(cts == NULL) return -1;
    INFO("Please input the filename to save:\n");
    
    char filename[NAME_LENGTH] = {0};//创建一个字符串数组来接收写入地址
    scanf("%s", filename);

    int a = save_file(cts->people, filename);//给执行接口传参
    if(-1 == a) INFO("Save fail!");
    else INFO("Save success~");

    return 0;
}

//(6)加载端：
int load_entry(struct contacts *cts)
{
    if(cts == NULL) return -1;
    INFO("Please input the filename to load from:\n");
    
    char filename[NAME_LENGTH] = {0};//创建一个字符串数组来接收加载源地址
    scanf("%s", filename);

    load_file(&cts->people, &cts->quantity, filename);//给执行接口传参

    return 0;
}

//end----------------------------------------------------------------

//【主界面层：主函数组织起所有端口，并显示业务选项在终端】
//（1）对于业务层不同选择的“重命名”（枚举数据类型重定义）：
enum
{
    OPER_INSERT = 1,
    OPER_PRINT,
    OPER_DELETE,
    OPER_SEARCH,
    OPER_SAVE,
    OPER_LOAD,
};
//（2）终端交互界面函数：
void menu_info(void)
{
    INFO("\n\n****************************************************\n");
    INFO("*****1.Add person\t\t2.Print people******\n");
    INFO("*****3.Del person\t\t4.Ser person***********\n");
    INFO("*****5.Save person\t\t6.Load person*******\n");
    INFO("*****Other key for exiting program******************\n");
    INFO("****************************************************\n\n");
}

//（3）主函数：
int main()
{
    //创建封装好（头指针和头指针指向的数据节点）的结构体：
    struct contacts *cts = (struct contacts*)malloc(sizeof(struct contacts));
    if(cts == NULL) return -1;

    memset(cts, 0, sizeof(struct contacts));
    /*这里要初始化因为constructs里的quantity可能被污染
    因为头指针people虽然会被后续操作覆盖，但是quantity后续只是++或--，
    并且注意！结构体声明时并不能进行赋值或者初始化！！！*/
    while(1)
    {
        menu_info();//不同的业务选项显示菜单：

        int select = 0;
        scanf("%d", &select);

        //不同的业务选项进入端口：
        switch(select)
        {
            case OPER_INSERT:
                insert_entry(cts);
                break;
            case OPER_PRINT:
                print_entry(cts);
                break;
            case OPER_DELETE:
                delete_entry(cts);
                break;
            case OPER_SEARCH:
                search_entry(cts);
                break;
            case OPER_SAVE:
                save_entry(cts);
                break;
            case OPER_LOAD:
                load_entry(cts);
                break;

            default:
                //goto只能用于同一个函数内，跳过中间代码，直接跳转
                goto exit;
                
        }
    }
    //释放已malloc的内存，并且置空指针：
    exit:
        free(cts);
        cts = NULL;

    return 0;
}

//end----------------------------------------------------------------


