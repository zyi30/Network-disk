//客户端代码
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define  OK  "ok#"
#include <netinet/in.h>
#include <fcntl.h>
#define CMD_ERR "err arg"
#define FILE_ERR "err file"
#define  OK  "ok#"
char *get_cmd(char buff[],char *myargv[])
{
    if(buff==NULL||myargv==NULL)
    {
        return NULL;
    }
    char *s=strtok(buff," ");//单线程用strtok就可以了;
    int i=0;
    while(s!=NULL)
    {
        myargv[i++]=s;
        s=strtok(NULL," ");
    }

    return myargv[0];//返回命令;
}
//客户端的代码:
void up_file(int c,char *cmd_buff,char *filename)
 {
   if(cmd_buff==NULL || filename==NULL)
   {
       send(c,CMD_ERR,strlen(CMD_ERR),0);
       printf("文件名为空，请输入要上传的文件名!\n");
       return ;
   }
   send(c,cmd_buff,strlen(cmd_buff),0);

   int fd=open(filename,O_RDONLY);
   if(fd==-1)
   {
       send(c,FILE_ERR,strlen(FILE_ERR),0);
       printf("要上传的文件打开失败!\n");
       return ;
   }

   int filesize=lseek(fd,0,SEEK_END);
   lseek(fd,0,SEEK_SET);

   char buff_size[64]={0};
   sprintf(buff_size,"ok#%d",filesize);
   send(c,buff_size,strlen(buff_size),0);

   memset(buff_size,0,64);
   int n=recv(c,buff_size,63,0);
   if(n<0)
   {
       printf("err\n");
       return ;
   }
   if(strcmp(buff_size,"err")==0)
   {
       printf("err\n");
       return ;
   }

   char data[1024];
   int num=0;
   printf("开始发送文件：!\n");
   while((num=read(fd,data,1024))>0)
   {
       send(c,data,num,0);
   }
   printf("文件发送完毕！\n");
   close(fd);
   return ;

 }
void recv_file(int c, char cmd_buff[], char* filename)
{
   if ( cmd_buff == NULL || filename == NULL )
       //前面是cmd为空,后面是文件名为空即只写了一个get
   {
      return;
   }
    send(c,cmd_buff,strlen(cmd_buff),0);//客户端给服务器端发送get a.c

    //接下来就是收服务器发送过来的Ok或者error
    char buff[64] = {0};
    int num = recv(c,buff,63,0);
    if ( num <= 0 )//num<=0,说明服务器端异常崩掉了;
   {
      printf("ser close or err\n");
      return;
   }
    if ( strncmp(buff,"err",3) == 0 )
        //服务器回复我们err,就证明出错了,我们打印ser err
   {
      printf("ser :%s\n",buff);
      return;
   }

    //执行到这里,就是服务器回复的是:ok#size
   int filesize = 0;
   sscanf(buff+3,"%d",&filesize);//获得大小也可以用atoi
   //或者使用atoi方法也可以:
  // int filesize=atoi(buff+3);
     printf("文件:%s, 大小:%d\n",filename,filesize);
     if ( filesize < 0 ) //有时候协议内容有问题,
         //那么就有可能文件大小小于0,那么我们就打印错误;
    {
      send(c,"err",3,0);
      return;
    }
    int fd = open(filename,O_CREAT|O_WRONLY,0600);
    //以读写的方式打开filename;注意要引用头文件  <fcntl.h>
    if ( fd == -1 )
        {
      printf("创建文件失败\n");
      send(c,"err",3,0);
      return;
   }

     int downsize=0;
      downsize=lseek(fd,0,SEEK_END);
      //先求出客户端的已有的文件的大小,即已经下载的文件的大小;
      char buff_size[64]={0};
      sprintf(buff_size,"ok#%d",downsize);
      printf("该文件已经下载了%d个字节！\n",downsize);
       send(c,buff_size,strlen(buff_size),0);
    char data[1024];

    int curr_size = 0;
    while( 1 )
   {
      int n = recv(c,data,1024,0);
      if ( n <= 0 )
      {
        printf("down file err\n");
          //可能对方关闭文件了,下载错误了;
          //之后我们也可以调用rm删除这个文件,也可以保留,就是不做处理;
        break;
      }
     write(fd,data,n);//刚才读取了n个数据,现在往文件中写入n个数据
     curr_size += n;
     float f = curr_size * 100.0 / filesize;
     printf("当前下载：%.2f%%\r",f);
        //写两个%号才能打出一个%号;先把百分比打印出来,
        //万一要退出就打印不了百分比了,所以在这里先打印百分比;

//并且我们需要它在一个地方原地去打印,所以加一个\r;
//前面的不变,后面一直覆盖打印,
//这样我们看到的结果就是数字在变;
//\r的意思就是光标到行尾了,然后又到行头重新打印;
// \n是另起一行，\r的话回到本行的开头，
 //        如果继续输入的话会把先前的覆盖掉
//比如printf("asd flkj\r111")输出的是111 flkj
     fflush(stdout);//打印不出去,我们还必须要刷新一下;
     if ( curr_size >= filesize )//为什么写>,
         //因为有时候代码有问题,>的时候还在操作,所以>也先退出再说;
    {
       break;
    }
}

    close(fd);
    printf("\n");
    printf("文件下载完成\n");
    return ;
}

int main()
{
    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd==-1)
    {
        exit(0);
    }

    struct sockaddr_in saddr;
    memset(&saddr,0,sizeof(saddr));
    saddr.sin_family=AF_INET;

    saddr.sin_port=htons(6000);
    saddr.sin_addr.s_addr=inet_addr("127.0.0.1");

    int res=connect(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
    if(res==-1)
    {
        close(sockfd);
        exit(0);
    }
    while(1)
    {
        printf("connect>>");
        fflush(stdout);
        char buff[128]={0};
        fgets(buff,128,stdin);
        //if(strncmp(buff,"end",3)==0)
        //不要这几句了,统一交给get_cmd这个方法处理
        //{
           // break;
        //}

        //从这里开始加入以下代码,把原来下面所有的代码都替换
        buff[strlen(buff)-1]=0;
        //因为用fgets会读到\n,那么先去掉\n;
        char  cmd_buff[128]={0};
        strcpy(cmd_buff,buff);
        //备份buff,因为字符串分割函数strtok会改变buff的值,后面要用buff;

        //解析命令,参数,异常检测
        char  *myargv[10]={0};
        char *cmd=get_cmd(buff,myargv);
        if(cmd==NULL)//没有输入命令,为空,继续下一轮循环
        {
            continue;
        }
        else if(strcmp(cmd,"exit")==0)//exit客户端要退出
        {
            break;
            //写到这里,其实就是上面写的end退出的那三句,
            //就是统一交给get_cmd去解决;
        }
        else if(strcmp(cmd,"get")==0)
        {
             //get  下载
             //服务器是发送文件,客户端这里是接收文件
      recv_file(sockfd,cmd_buff,myargv[1]);//
      //cmd_buff里面放的就是我们刚才备份的命令+参数;
      //myargv[1]里面放的就是文件名;
        }

        else if(strcmp(cmd,"up")==0)
        {

            up_file(sockfd,cmd_buff,myargv[1]);
        }
        else
        {
             //执行通用命令
             send(sockfd,cmd_buff,strlen(cmd_buff),0);
            //发送刚才备份的命令
             char  read_buff[1024]={0};
            //读取服务器返回的结果;
            int  num=recv(sockfd,read_buff,1023,0);
            if(num<=0)//要么服务器关闭,要么服务器出错
            {
                printf("ser close or err\n");
                break;
                //出错了要么退出,要么重新connect链接服务器
                //这里直接退出,也就是子进程结束了;
            }
            if(strncmp(read_buff,OK,strlen(OK))==0)
                //如果成功了,就是读取前面的几个字符是ok#
            {
                printf("%s\n",read_buff+strlen(OK));
                //打印内容,不打印ok#
            }
            else//否则就出错了,打印出错的各种结果就是以err开头的那些宏
            {
                printf("%s\n",read_buff);
            }
        }

    }
}






