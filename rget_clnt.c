/**
 * client program
 * */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "rget.h"

int main(int argc, char *argv[])
{
	int clnt_sock;
	int res,i;
	int len;
	int opt;
	char retval[5];
	
	if(argc < 2) {
		fprintf(stderr,"%s argument is invalid\n",argv[0]);
		usage(argv);
		exit(1);
	}	

	clnt_sock = connect_to_server();
	if(clnt_sock == -1) {
		PrtErr("socket");	
	}
	
	send(clnt_sock,&argc,sizeof(argc),0);  /*参数长度*/
	
	for(i=1; i < argc; i++) {  //send 命令行参数
		len = strlen(argv[i]);
		res = send(clnt_sock,&len,sizeof(len),0);
		res = send(clnt_sock,argv[i],len,0);
	}

	if(strncmp(argv[1],"-",1) == 0) {  //使用"-rf"短选项
		if(strchr(argv[1],'r') != NULL) { //指定r
			if(whether_exist(argv[2])) { //dir exist, exit
				if(strchr(argv[1],'f') != NULL) {
					printf("-rf \n");
					delete_dir(nopath(argv[2]));
				}
				else {
					fprintf(stderr,"%s is already exist\n",nopath(argv[2]));
					goto exit;
				}
			}
			recv_dir(clnt_sock);
			goto exit;
		} else {
			printf("file \n");
			if(whether_exist(argv[2])) {
				if(strchr(argv[1],'f') == NULL) { 
					fprintf(stderr,"file exist,need -f(force)\n");
					goto exit;
				} else 
					if(unlink(nopath(argv[2])) == -1) //file unlink failed
						goto exit;
			}
			recv(clnt_sock,retval,3,MSG_WAITALL);
			recv_file(clnt_sock);
			goto exit;
		}
	} else {
		printf("not -\n");
		recv(clnt_sock,retval,3,MSG_WAITALL); //recv 3 bytes
		if(strncmp(retval,"err",3) == 0) {  //dest is a dir
			fprintf(stderr,"is a dir, not -r\n");
			exit(1);
		} else if (strncmp(retval,"yes",3) == 0) { //dest is a file
				if(whether_exist(argv[1])) {
					fprintf(stderr,"file exist,need -f(force)\n");
					goto exit;
				} 
		recv_file(clnt_sock);
		}
	}
exit:
	close(clnt_sock);
	return 0;
}

int delete_dir(char *filename) {
	DIR *ptr;
	struct dirent *direntp;
	struct stat sbuf;
	char current_dir[256]={0};

	if(getcwd(current_dir,255) ==NULL)
		PrtErr("getcwd");
	current_dir[255] = '\0';
	
	if(stat(filename,&sbuf) == -1)
		PrtErr("stat dir");
	
	printf("delete dir %s\n",filename);
	if(S_ISDIR(sbuf.st_mode)) {
		if((ptr = opendir(filename)) != NULL) {
			chdir(filename);
			while((direntp = readdir(ptr)) != NULL)
				if(strncmp(direntp->d_name,".",1) != 0 && strncmp(direntp->d_name,"..",2) != 0)
					if(delete_dir(direntp->d_name) != 0)
						printf("delete failed %m\n"),exit(-1);
			
			chdir(current_dir);
			if(rmdir(filename) == -1) {
				fprintf(stderr,"not permission rmdir %s\n",filename);
				exit(EXIT_FAILURE);
			}
		}
	}else if(S_ISREG(sbuf.st_mode)){
		if(unlink(filename) == -1) {
			fprintf(stderr,"not permission unlink file %s\n",filename);
				exit(EXIT_FAILURE);
		}
	} else {
		fprintf(stderr,"%s is not a dir or regularfile\n",filename);
		exit(EXIT_FAILURE);
	}
}

bool whether_exist(char *filename){
	DIR *dir_ptr;
	struct dirent *direntp;
	char *onlyfile;

	onlyfile = nopath(filename);

	if((dir_ptr = opendir(".")) == NULL) {
                        PrtErr("can't open dir");
        } else {
        	while((direntp = readdir(dir_ptr)) != NULL) {
                	if(strcmp(direntp->d_name,onlyfile) == 0)
				return true;
                        }
                closedir(dir_ptr);	
	}
	return false;	
}

//去掉文件名中的路径'/'
char *nopath(char *filename) {
	char *onlyfile;	

	if(strrchr(filename,'/') != NULL)
                onlyfile = strrchr(filename,'/') + 1;
        else
                onlyfile = filename;
	
	return onlyfile;
}

int recv_file(int cfd) {
	int ffd;
	int res,len;
	char *filename;

	filename = recv_filename(cfd);
        printf("recv file::%s\n",filename);

	//create file
	ffd = open(filename,O_RDWR | O_CREAT,0666);
	if(ffd == -1)
		PrtErr("open filename");

	//recv file data
	recv_filedata(cfd,ffd);

	return 0;
}

int recv_dir(int fd) {
	char *rf_name;
	char res;
	int retval;

	while(1) {
		memset(&res,0,1);
		retval = recv(fd,&res,sizeof(res),MSG_WAITALL); //serv while,here zuse
		if(retval == 0 || retval == -1 ) break;
		
		if(strncmp(&res,"r",1) == 0) {		
			//recv dir name
			rf_name = recv_filename(fd);
			printf("recv dir::%s\n",rf_name);
			
			//creat dir
			if(mkdir(rf_name,S_IRWXU | S_IRWXG |S_IROTH | S_IXOTH) == -1)
				PrtErr("mkdir");
				
			if(chdir(rf_name))
				fprintf(stderr,"chdir error");
			recv_dir(fd);
		} else if(strncmp(&res,"f",1) == 0){
			recv_file(fd);
		}
	}
}

char *recv_filename(int cfd) {
	int res,len;
	static char fname[BUFSIZ];

	memset(fname,0,BUFSIZ);
	//接收文件信息
        res = recv(cfd,&len,sizeof(len),MSG_WAITALL);
        res = recv(cfd,&fname,len,MSG_WAITALL);
        fname[len] = 0;

	return fname;
}

void recv_filedata(int cfd,int ffd) {
	int res,len;
	char buf[BUFSIZ];

	while(1) {
                res = recv(cfd,&len,sizeof(len),MSG_WAITALL);
                if(len == 0) break;
                res = recv(cfd,buf,len,MSG_WAITALL);
                write(ffd,buf,len);
        }
}

void usage(char **argv) {
	printf("用法：%s [rf] source dest\n",argv[0]);
	printf("-r 递归拷贝整个目录\n");
	printf("-f 本地有相同文件或目录强行覆盖，不提示\n");
}
