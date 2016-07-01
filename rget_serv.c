/**
 * server program
 * */
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include "rget.h"

void init_argu(struct argu_option *x) {
        x->recursive = false;
        x->force = false;
}

void sig_chld(int signo) {
	pid_t pid;
	int stat;

	while((pid = waitpid(-1,&stat,WNOHANG)) > 0)
		printf("child %d terminated\n",pid);
	return;
}

int main()
{
	int serv_sock;
	int res,fd;
	char request[BUFSIZ];
	int i=0;
	int n;
	char *arg[128];
	
	//extern int errno;	
	serv_sock = make_server_sock();	

	signal(SIGCHLD,sig_chld);
	while(1) {
		if((fd=accept(serv_sock,NULL,NULL)) < 0) {
			if(errno == EINTR)
				continue;
			else
				PrtErr("accept");
		}
		printf("accept a client....\n");
		if(fork() == 0) {  //子进程处理客户请求，关闭监听套接字
			close(serv_sock);	
			if(!process_request(fd))
				PrtErr("recv_argu err");
			exit(0);
		}
		close(fd);  //父进程关闭连接套接字
	}
}

bool process_request(int cfd) {
	struct argu_option s;
	struct stat sbuf;	
	char request[BUFSIZ];
        int len,res;
        char *arg[128];
        int arg_num;
        int n,i=0;

	init_argu(&s);

        recv(cfd,&arg_num,sizeof(arg_num),MSG_WAITALL);   /*接收参数argc的个数*/
         n = arg_num;
         while(--n) {
                 memset(request,0,sizeof(request));
                 res = recv(cfd,&len,sizeof(len),MSG_WAITALL);
                 if(res == -1)
                         return false;
                 res = recv(cfd,request,len,MSG_WAITALL);
                 if(res == -1)
                         return false;
                 if(res==0)
                 {
                         printf("client close\n");
                         break;
                 }

                 arg[i] = (char *)malloc(sizeof(char *));
                 strcpy(arg[i],request);
                 printf("get a call: request %d= %s\n",i,arg[i]); /*行缓冲，遇换行符输出*/
                 i++;
         }

	if(strncmp(arg[0],"-",1) == 0) {
		if(strstr(arg[0],"r") != NULL)
			s.recursive = true;
		if(strstr(arg[0],"f") != NULL)
			s.force = true;
	}

	if(access(n==3?arg[1]:arg[0],F_OK) != 0)  //检查用户权限
		return false;

	if(stat(n==3?arg[1]:arg[0],&sbuf) == -1) 
		return false;
	
	if(S_ISDIR(sbuf.st_mode)) {
		if(s.recursive) {
			is_dir(n==3?arg[1]:arg[0],cfd);
		}
		else
			send(cfd,"err",3,0);
	} else if(S_ISREG(sbuf.st_mode)) {
		send(cfd,"yes",3,0);   //client 没有-参数时，判断传入的是dir or file
		if(!is_regfile(n==3?arg[1]:arg[0],cfd))
			PrtErr("is_regfile err");
	}
	return true;
}

bool is_regfile(char *arg,int fd) {
	
	//send file name
	if(!send_filename(arg,fd))
		PrtErr("send_filename failed");

	//send file data
	if(!send_filedata(arg,fd))
		PrtErr("send filedata failed");
	printf("send file::%s\n",arg);
	
	return true;
}

bool send_filename(char *arg,int fd) {
 	char *filename;
	int len,res;

	if(strrchr(arg,'/') != NULL)
                filename = strrchr(arg,'/') + 1;
        else
                filename = arg;

        len = strlen(filename);
        res = send(fd,&len,sizeof(len),0);
        if(res == -1) {
                PrtErr("send name len failed");
                return false;
        }
        res = send(fd,filename,len,0);
        if(res == -1) {
                PrtErr("send filename failed");
		return false;
        }
	return true;
}

bool send_filedata(char *filename,int fd) {
	int ffd;
	int size,res;
	char buf[BUFSIZ];	

	ffd = open(filename,O_RDONLY);
        if(ffd == -1) {
                PrtErr("fopen");
		return false;
	}

	while(1) {
                size = read(ffd,buf,BUFSIZ);
                if(size == -1) break;
                if(size == 0) break;
                if(size > 0) {
                        res = send(fd,&size,sizeof(size),0);
                        if(res == -1) break;
                        res = send(fd,buf,size,0);
                        if(res == -1) break;
                }
        }
        size = 0;
        res = send(fd,&size,sizeof(size),0);
        close(ffd);

	return true;
}

bool is_dir(char *filename,int fd) {
	DIR *dir_ptr;
	struct dirent *direntp;
	struct stat filestat;
	char current_dir[BUFSIZ];
	int chdir_success = 1;	
	int res;

	if(lstat(filename,&filestat)  == -1)
		PrtErr("lstat");

	if(S_ISDIR(filestat.st_mode)) {
		res = send(fd,"r",1,0);  //send dir flag "r"
		if(res != 1)
			PrtErr("send char r");
		
		getcwd(current_dir,BUFSIZ-1);
		current_dir[BUFSIZ-1] = '\0';	

		if(chdir(filename) != 0)
			PrtErr("chdir");

		if(!send_filename(filename,fd))
			PrtErr("dir send filename failed");
		printf("send dir::%s\n",filename);

		if((dir_ptr = opendir(".")) == NULL) {
			PrtErr("can't open dir");
		} else {
			while((direntp = readdir(dir_ptr)) != NULL) {
				if(strcmp(direntp->d_name,".") && strcmp(direntp->d_name,"..")) {
					is_dir(direntp->d_name,fd);
				}			
			}
			closedir(dir_ptr);
			if(chdir(current_dir) != 0)
				PrtErr("chdir 2");
		}
	} else {
		res = send(fd,"f",1,0);  //send file flag "f"
		if(res != 1)
			PrtErr("send char f");
		is_regfile(filename,fd);
	}
	return true;
}

