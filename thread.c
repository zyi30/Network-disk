
//thread.c全部代码如下:
#include "thread.h"
#include <sys/wait.h>
#include <fcntl.h>
#define FILE_ERR "err file"
#define ARG_MAX 10
#define CMD_ERR "err arg"
#define PIPE_ERR "err pipe"
#define FORK_ERR "err fork"
 //定义成这样的宏的意思是我们其实可以把错误分类,
#define OK "ok#"
    void send_file(int c,char *filename)
{
   if(filename==NULL)
   {
     send(c,CMD_ERR,strlen(CMD_ERR),0);//如果get后面没有文件名,
       //那么命令参数错误,还用以前的宏;
     return ;
   }
    
   int fd=open(filename,O_RDONLY);
   if(fd==-1)
   {
      send(c,FILE_ERR,strlen(FILE_ERR),0);//在上面再定义一种错误类型:
       //文件(不存在)错误类型
      return;
   } 
    
    //到这里就是成功的打开这个文件了;
    //告诉客户端成功的同时我们还需要一个文件大小; 
    //lseek或者lstat方法都可以,里面的结果体就有文件的大小;
   
    int filesize=lseek(fd,0,SEEK_END);
    lseek(fd,0,SEEK_SET);
    
   //给客户端回复ok#文件大小
   char buff_size[64]={0};
   sprintf(buff_size,"ok#%d",filesize);
   send(c,buff_size,strlen(buff_size),0);

    //等着客户端回复是否要下载;
   memset(buff_size,0,64);
   int n=recv(c,buff_size,63,0);
   if(n<=0) //客户端关闭了
   {
    return ;
   } 
    if(strcmp(buff_size,"err")==0)//客户端回复err,就是不下载的意思
   {
     return ;
   }
    
    //到这里了,客户端就是要下载的意思;
    //从这里开始加入以下代码,就可以做到断点续传;
     //因为运行到这里就是客户端发送ok#downsize的地方;
     //解析downsize;
     int downsize=0;
     sscanf(buff_size+3,"%d",&downsize);
     printf("该文件已经下载了%d个字节!!!!!!!!\n",downsize);
      downsize=lseek(fd,downsize,SEEK_SET);
      if(downsize==-1)
     {
        printf("文件打开失败!!\n");
         return ;
     }
     char data[1024];
    int num=0;
      int sendnum=0;
    while((num=read(fd,data,1024))>0)
     {
        sendnum=send(c,data,num,0);
        if(sendnum==-1)
        {
             printf("发送数据失败！\n");
             close(fd);
              printf("client close\n");
             return ;
        }
     }
    close(fd);
    return ;
 } 

    void recv_file(int c,char cmd_buff[],char *filename)
{
    if(cmd_buff==NULL || filename==NULL)
    {
        printf("上传的文件为空!\n");
        return ;
    }
   // send(c,cmd_buff,strlen(cmd_buff),0);

    char buff[64]={0};
    int num=recv(c,buff,63,0);
    if(num<0)
    {
        printf("cli close or err\n");
        return ;
    }
    if(strncmp(buff,"err",3)==0)
    {
        printf("cli :%s\n",buff);
        return ;
    }

    int filesize=0;
    sscanf(buff+3,"%d",&filesize);

    printf("上传文件：%s,文件大小:%d\n",filename,filesize);
    if(filesize<0)
    {
        send(c,"err",3,0);
        return ;
    }

    int fd=open(filename,O_CREAT|O_WRONLY,0600);
    if(fd==-1)
    {
        printf("上传文件时创建文件失败！\n");
        send(c,"err",3,0);
        return ;
    }
    send(c,"ok",2,0);
    char data[1024];
    int curr_size=0;

    while(1)
    {
        int n=recv(c,data,1024,0);
        if(n<0)
        {
            printf("up file err\n");
            break;
        }
        write(fd,data,n);
        curr_size+=n;
        float f=curr_size*100.0/filesize;
        printf("当前下载:%.2f%%\r",f);

        fflush(stdout);
        if(curr_size>=filesize)
        {
            break;
        }
    }
    close(fd);
    printf("\n");
    printf("文件上传完成！\n");
    return ;

}
char *get_cmd(char buff[],char *myargv[])
{
    if(buff==NULL||myargv==NULL)

    {   
        return NULL;
    }   

    char *s=NULL;//每次返回的字段
    char *p=NULL;//记录当前分割字符串的位置
    
    int i=0;
    s=strtok_r(buff," ",&p);
    while(s!=NULL)
    {   
        myargv[i++]=s;
        s=strtok_r(NULL," ",&p);
    }   

    return myargv[0];//命令
}
void * work_thread(void *arg)
{
    
    int c=(int)arg;

    while(1)
    {   
        char buff[128]={0};
        int n=recv(c,buff,127,0);
        //接收客户端发过来的数据,比如ls -l,mv a.c b.c,rm file
        //然后将这些命令存入到参数中char *myargv[];
        if(n<=0)
        {
            break;
        }

         //以下代码替换原来的两句代码:
         // printf("buff=%s\n",buff);
        //send(c,"ok",2,0);
        
      //解析命令及参数
        char *myargv[ARG_MAX]={0};
        char *cmd=get_cmd(buff,myargv);

        if(cmd==NULL)//参数检查
        {
            send(c,CMD_ERR,strlen(CMD_ERR),0);
            continue;
        }

        //判断是不是命令，是，fork+exec();
        //get:代表下载，实现函数downfile();
        //up:代表上传，实现函数recvfile();

        if(strcmp(cmd,"get")==0)
        {
            //下载
            send_file(c,myargv[1]);
        }
        else if(strcmp(cmd,"up")==0)
        {
            recv_file(c, cmd,myargv[1]);


        }
        else
        {
            //创建无名管道
            //fork复制子进程
            //在子进程中，替换标准输出，使用dup2函数，然后exec替换命令
            //在父进程中，读取管道内容；
    
            int pipefd[2];
            if(pipe(pipefd)==-1)
            {
                send(c,PIPE_ERR,strlen(PIPE_ERR),0);
                continue;
            }
            pid_t pid=fork();
            if(pid==-1)
            {
                send(c,FORK_ERR,strlen(FORK_ERR),0);
                continue;
            }
            if(pid==0)//子进程
            {
                close(pipefd[0]);
                //关闭子进程的读端,当然这个不写也可以,
                //反正替换成功或者失败都会关闭,见管道详解;
                  dup2(pipefd[1],1);
                //标准输出为1;管道的写端覆盖标准输出;
                dup2(pipefd[1],2);
                //标准错误输出为2;管道的写端覆盖标准错误输出;

                execvp(cmd,myargv);
                perror("exec err");
                close(pipefd[1]);//关闭标准输出;
                exit(0);
                //其实退出进程后标准输入和标准输出也就会关闭了,
                //可以省略两个close,但是不好;
            }
            //能执行到这里的必定是父进程; //1-6为父进程所执行的代码  
             //fork父进程 读管道中的数据
        close(pipefd[1]);//1  //关闭父进程的写端;
        wait(NULL);//2   
            //先等待子进程结束,防止僵死进程产生;要加头文件<sys/wait.h>
    
//接下来读取管道中的内容;1024个字节,
//如果不够,可以循环读取;后面再改;我们这里就只读取一次就好了;
//一种是管道中有数据可以读取出来,一种是管道中没有数据,有零个字节;
//因为管道的写端被关闭了,也不会出现read阻塞住的情况,
//当然,我们在父进程中也要关闭写端;
            
//char read_buff[1024]={"OK#"};
//也就是无论管道中有没有数据,都给客户端回复一个ok#;或者处理成宏,如下:
    
        char read_buff[1024]={OK};//3
        read(pipefd[0],read_buff+strlen(OK),1021);//4
        close(pipefd[0]);//5
        send(c,read_buff,strlen(read_buff),0);//6
            
        }
    }

    close(c);
    printf("client close\n");
}


void start_thread(int c)
{
    pthread_t id;
    pthread_create(&id,NULL,work_thread,(void *)c);
}


