/*保存消息的回声服务器端*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 30
void error_handling(char *message);
void read_childproc(int sig);

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;

	int fds[2];   /*fds[0] 管道输出、fds[1] 管道输入*/

	pid_t pid;
	struct sigaction act;
	socklen_t adr_sz;
	int str_len, state;
	char buf[BUF_SIZE];
	if(argc != 2)
	{
		printf("Usage: %s <port> \n", argv[0]);
		exit(1);
	}

	act.sa_handler = read_childproc;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	//处理僵尸进程
	//注册信号，子进程结束后系统会调用act中的处理函数
	state = sigaction(SIGCHLD, &act, 0);

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");

	if(listen(serv_sock, 5) == -1)
		error_handling("listen() error");

	pipe(fds);
	pid = fork();
	if(pid == 0)   /*保存消息*/
	{
		FILE *fp = fopen("store_file.txt", "wt");
		char msgbuf[BUF_SIZE];
		int i, len;

		for(int i=0; i<10; i++)
		{
			len = read(fds[0], msgbuf, BUF_SIZE);
			fwrite((void*)msgbuf, 1, len, fp);
		}
		fclose(fp);
		return 0;
	}

	while(1)
	{
		adr_sz = sizeof(clnt_adr);
		clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
		if(clnt_sock == -1)
			continue;
		else
			puts("new client connected...");

		pid = fork();
		if(pid == 0)   /*创建子进程进行数据传输*/
		{
			close(serv_sock);   /*关闭无关的套接字文件描述符*/
			while((str_len = read(clnt_sock, buf, BUF_SIZE)) != 0)
			{
				write(clnt_sock, buf, str_len);
				write(fds[1], buf, str_len);
			}

			close(clnt_sock);
			puts("client disconnected...");
			return 0;
		}
		else  /*主进程处理连接请求*/
			close(clnt_sock);
	}
	close(serv_sock);
	return 0;
}

void read_childproc(int sig)
{
	pid_t pid;
	int status;
	/*
	等待终止子的进程ID，-1表示任意子进程
	保存子进程返回值
	可选项，WNOHANG 无子进程终止返回0,不进入阻塞状态
	*/
	pid = waitpid(-1, &status, WNOHANG);
	printf("remove proc id: %d \n", pid);
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}









