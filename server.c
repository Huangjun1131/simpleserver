#include <stdio.h>
#include <stdio.h>
#include "debug.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/epoll.h>

typedef void (*a)(int);
typedef struct abc{
	a p;
	void *ptr;
	int sockfd;
}ABC;
void sock_handler(int i)
{
	static unsigned int count;

	printf("my own socket handler %d, count is %d\n", i, count++);
	system("date");
}

int global_count;

int main()
{
	ABC ll;
	ll.p = sock_handler;
	ll.ptr = NULL;

	
	char buff[2500]={0};
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == listenfd)
		errsys("socket");

	struct sockaddr_in myaddr = {0};
	struct sockaddr_in clientaddr = {0};
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons(8888);
	myaddr.sin_addr.s_addr = inet_addr("0.0.0.0");//INADDR_ANY
	int len = sizeof myaddr;

	if(-1 == bind(listenfd, (struct sockaddr*)&myaddr, len))
		errsys("bind");

	if(-1 == listen(listenfd, 10))
		errsys("listen");

	int epoll_fd = epoll_create(1024);
	if(-1 == epoll_fd)
		errsys("epoll");

	struct epoll_event event = {0};
	event.events = EPOLLIN;
	ll.sockfd = listenfd;
	event.data.ptr = (void*)&ll;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listenfd, &event);

#define BUFSIZE 1000
#define MAXNFD  1024 
	
	struct epoll_event revents[MAXNFD] = {0};
	int nready;
	char buf[MAXNFD][BUFSIZE] = {0};
	while(1)
	{
		if(-1 == (nready = epoll_wait(epoll_fd, revents, MAXNFD, -1)) )
			errsys("poll");
		
		
		int i = 0;
		for(;i<nready; i++)
		{
			if(revents[i].events & EPOLLIN)
			{
//		
			
				ABC *cc = (ABC *)revents[i].data.ptr;
	
				if(cc->sockfd == listenfd)
				{
					int sockfd = accept(listenfd, (struct sockaddr*)&clientaddr, &len);
					if(-1 == sockfd)
						errsys("accept");
					debug("incoming: %d,%s,%d\n",sockfd, inet_ntoa( clientaddr.sin_addr),ntohs(clientaddr.sin_port) );
					
					struct epoll_event event = {0};
					event.events = EPOLLIN;
					ABC *ll = malloc(20);
					ll->p = sock_handler;
					ll->sockfd = sockfd;
					event.data.ptr = (void*)ll;
					epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &event);
				}
				else
				{
					int ret = read(cc->sockfd, buf[cc->sockfd], sizeof buf[0]);
					if(0 >= ret)
					{
						close(cc->sockfd);
						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cc->sockfd, &revents[i]);
						printf("clear sockfd %d\r\n", cc->sockfd);
						continue;
					}
					global_count = ret;
					revents[i].events = EPOLLOUT;
//					ABC *ll = (ABC *)revents[i].data.ptr;
					epoll_ctl(epoll_fd, EPOLL_CTL_MOD, cc->sockfd, &revents[i]);
				}

			}
			else if(revents[i].events & EPOLLOUT)
			{
				ABC *ll = (ABC*)(revents[i].data.ptr);
				ll->p(ll->sockfd);
				FILE *ffp = fopen("index.html","w+");
//				if(-1 == fread(buff,1,2500,ffp))
//					errsys("read\n");
				int ret = write(ll->sockfd, buf[ll->sockfd], global_count);
				fwrite(buf[ll->sockfd],1,global_count, ffp);
				fclose(ffp);
				printf("ret %d: %d\n", ll->sockfd, ret);
				int idx;
				for (idx = 0; idx < global_count; idx++)
				{
					printf("0x%02X ", (unsigned char)buf[ll->sockfd][idx]); 
				}
				printf("\n");
				revents[i].events = EPOLLIN;
				epoll_ctl(epoll_fd, EPOLL_CTL_MOD, ll->sockfd, &revents[i]);
	//			close(ll->sockfd);
			}
		}
	}

	close(listenfd);
}
