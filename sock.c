#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define LISTEN 1

int make_server_sock(){
	int res,serv_sock;
	struct sockaddr_in saddr;

	serv_sock = socket(AF_INET,SOCK_STREAM,0);
        if(serv_sock == -1)
        	return -1;

        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(11000);
        inet_aton("127.0.0.1",&saddr.sin_addr);

        res = bind(serv_sock,(struct sockaddr *)&saddr,sizeof(saddr));
        if(res == -1) return -1;

        if(listen(serv_sock,LISTEN) == -1)
                return -1;

	return serv_sock;
}

int connect_to_server(){
	int sock,res;
	struct sockaddr_in saddr;
	
	sock = socket(AF_INET,SOCK_STREAM,0);
        if(sock == -1)
		return -1;

        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(11000);
        inet_aton("127.0.0.1",&saddr.sin_addr);

        res = connect(sock,(struct sockaddr*)&saddr,sizeof(saddr));
        if(res == -1) return -1;

	return sock;
}
