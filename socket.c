#include "socket.h"

#define IPS "ipstr="
#define PORT "port="
#define LISMAX "lismax="
struct sock_info

{
    char ips[32];
    short port;
    short lismax;
};
//方法内部去填充这个结构体
int read_conf(struct sock_info *pa)
{
    if(pa==NULL)
    {
        return -1;
    }

    FILE *fp=fopen("my.conf","r");//没有写明文件路径表示在当前路径
    if(fp==NULL)
    {
        printf("my.conf is not exists!\n");
        return -1;
    }

    //从文件中读取一行内容用什么方法?
//fgets.查看帮助手册是如果等于NULL就到文件末尾了,所以循环条件是!=NULL
    int index=0;//判断出错在第几行
    char buff[128]={0};
    while(fgets(buff,128,fp)!=NULL)//fgets是读取一行内容,
        //每次最多读取128个字符;遇到\n停下来,如果用read读取,
         //还要自己去控制,遇到\n识别是一行,麻烦,有封装好的库函数就用;
    {
        index++;//每读取一行,行数++;
        if(strncmp(buff,"#",1)==0)
            //跳过注释,//当然也可以写if(buff[0]=='#')
        {
            continue;
        }
        if(strncmp(buff,"\n",1)==0)
            //跳过空行,空行里面只有一个'\n,//当然也可以写if(buff[0]=='\n')
        {
            continue;
        }
        //当然,也可以printf("%s",buff);先打印一下信息;
        //接下来就是填充结构体了;
        buff[strlen(buff)-1]='\0';//把最后面的\n换成'\0'
        if(strncmp(buff,IPS,strlen(IPS))==0)//写成宏,方便后期修改
        {
            strcpy(pa->ips,buff+strlen(IPS));
            //buff+strlen就定位到=后面的位置了;
        }
        else if(strncmp(buff,PORT,strlen(PORT))==0)
        {
            pa->port=atoi(buff+strlen(PORT));
            //buff+strlen就定位到=后面的位置了;
        }
        else if(strncmp(buff,LISMAX,strlen(LISMAX))==0)
        {
            pa->lismax=atoi(buff+strlen(LISMAX));
            //buff+strlen就定位到=后面的位置了;
        }
        else
        {
            printf("未识别的配置项在第%d行：%s\n",index,buff);
        }
        //当然,这里的健壮性还需要根据配置文件再细分;
    }

    fclose(fp);
}
int socket_init()
{
    struct sock_info a;
    if(read_conf(&a)==-1)
    {
        printf("read conf err\n");
        return -1;
    }

    printf("ip:%s\n",a.ips);
    printf("port:%d\n",a.port);
    printf("lismax:%d\n",a.lismax);


    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd==-1)
    {
        return sockfd;
    }

    struct sockaddr_in saddr;
    memset(&saddr,0,sizeof(saddr));
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(a.port);

    int res=bind(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
    if(res==-1)
    {
        return -1;
    }

    res=listen(sockfd,a.lismax);
    if(res==-1)
    {
        return -1;
    }

    return sockfd;
}

