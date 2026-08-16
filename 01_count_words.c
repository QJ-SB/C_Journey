//学习“状态机”的代码编辑理念 √

#include <stdio.h>

#define OUT 0
#define IN 1
#define INIT OUT

int is_delimiter(int c)
{
    if ((' ' == c) || ('\n' == c) || ('\t' == c) || ('\'' == c))//判定的分隔符
    {
        return 1;
    }
    else return 0;
}

int count_words(char *filename)
{
    int status = INIT;
    int words = 0;
    FILE *fp = fopen(filename,"r");//只读形式创建文件流
    if (fp == NULL)
    {
        return -1;
    }

    int c = 0;
    while ((c = fgetc(fp)) != EOF)//fgetc()函数读取单个字符返回的是int值
    {
        if (is_delimiter(c))
        {
            status = OUT;//状态机切换
        }
        else if (OUT == status)
        {
            status = IN;//状态机切换
            words++;//统计切换状态次数，以此统计单词数
        }
    }
    fclose(fp);
    
    return words;
}

int main(int argc, char *argv[])//argc终端输入的命令量(按空格划分)
{                               //argv[]把终端的命令看作字符串数组，传入终端读取的具体命令
    if (argc < 2)
    {
        return -1;
    }

    printf("the total of words in it is %d\n", count_words(argv[1]));//传入文件目录，返回单词数量

    return 0;
}