all: client server rdma

hw.h: hw.x
	rpcgen hw.x

hw_svc.c hw_clnt.c main.c hw_xdr.c: hw.h

client: main.o hw_clnt.o hw_xdr.o
	cc -O3 -Wall -L/usr/lib -libverbs -lpthread -o client main.o hw_clnt.o hw_xdr.o -lnsl

server: hw_server.o hw_svc.o hw_xdr.o rdma_queue2.o
	cc -O3 -Wall -L/usr/lib -libverbs -o server hw_server.o hw_svc.o hw_xdr.o rdma_queue2.o -lnsl

rdma: rdma_queue.o
	cc -O3 -Wall -L/usr/lib rdma_queue.c -libverbs -o rdma_server
	

.PHONY: clean

clean:
	-rm *.o
	-rm client
	-rm server
	-rm hw.h
	-rm hw_clnt.c
	-rm hw_svc.c