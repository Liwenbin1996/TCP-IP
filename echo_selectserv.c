/******************************************************************************
*Copyright(c) 2018,wenbin. All right reserved.
*Author:   wenbin
*Date:   2018-5-8
*Description:  并发回声服务器端---基于I/O复用实现
*
*Function List:
*    int select(int maxfd, fd_set* readset, fd_set* writeset, \
*			    		fd_set* exceptset, const struct timeval* timeout);
*    Parameters:
*        maxfd: 监视对象描述符的数量
*        readset: 将所有关注“是否存在待读取数据”的文件描述符注册到fd_set型变量
*        writeset: 将所有关注“是否可传输无阻塞数据”的文件描述符注册到fd_set型变量
*        exceptset: 将所有关注“是否发生异常”的文件描述符注册到fd_set型变量
*        timeout: 调用select函数后，为防止陷入无限阻塞的状态，传递超时信息
*    Retrun: 发生错误是返回-1,超时返回0,返回值大于0时表示发生事件的个数
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>

#define BUF_SIZE 100
void error_handling(char *message);

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	struct timeval timeout;
	fd_set reads, cpy_reads;

	socklen_t adr_sz;
	int fd_max, str_len, fd_num, i;
	char buf[BUF_SIZE];
	if(argc != 2)
	{
		printf("Usage: %s <port>", argv[0]);
		exit(1);
	}

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");

	if(listen(serv_sock, 5) == -1)
		error_handling("listen() error");

	FD_ZERO(&reads);   /*将reads全部清0*/
	FD_SET(serv_sock, &reads);   /*向reads注册serv_sock*/
	fd_max = serv_sock;

	while(1)
	{

		//select()调用后，除发生变化的文件描述符对应位外，
		//其余位都将清0,故每次都需进行初始化
		//select()调用后，timeout会被替换为超时前剩余时间，
		//故每次都需重新设置超时时间
		cpy_reads = reads;
		timeout.tv_sec = 5;
		timeout.tv_usec = 5000;

		if((fd_num = select(fd_max+1, &cpy_reads, 0, 0, &timeout)) == -1)
			break;

		if(fd_num == 0)
			continue;

		for(int i=0; i<fd_max+1; i++)
		{
			if(FD_ISSET(i, &cpy_reads))
			{
				if(i == serv_sock)   /*处理连接请求*/
				{
					adr_sz = sizeof(clnt_adr);
					clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
					FD_SET(clnt_sock, &reads);
					if(clnt_sock > fd_max)
						fd_max = clnt_sock;
					printf("connected client: %d \n", clnt_sock);
				}
				else   /*进行数据传输*/
				{
					str_len = read(i, buf, BUF_SIZE);
					if(str_len == 0)
					{
						FD_CLR(i, &reads);
						close(i);
						printf("closed client: %d \n", i);
					}
					else
						write(i, buf, str_len);
				}
			}
		}
	}
	close(serv_sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
