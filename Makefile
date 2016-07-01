all:serv clnt
serv:rget_serv.c sock.c
	gcc rget_serv.c sock.c -o serv

clnt:rget_clnt.c sock.c
	gcc rget_clnt.c sock.c -o clnt

clean:
	rm -fr serv clnt *.o
