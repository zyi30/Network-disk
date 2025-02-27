
//创建套接字并初始化(绑定与监听),处理接收客户端链接,并启动线程
#include "socket.h"
#include "thread.h"

int main()
{
    int sockfd=socket_init();
    if(sockfd==-1)
    {
        printf("create socket err\n");
        exit(0);
    }

    struct sockaddr_in caddr;
    int len;
    while(1)
    {
        len=sizeof(caddr);
        int c=accept(sockfd,(struct sockaddr*)&caddr,&len);
        if(c<0)
        {
            printf("accept failed\n");
            continue;
        }
        printf("client connected\n");
        printf("accept c=%d\n",c);


        start_thread(c);
    }

}
