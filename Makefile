all: client server sha_test

hw.h: hw.x
	rpcgen hw.x

hw_svc.c hw_clnt.c main.c hw_xdr.c: hw.h

client: main.o hw_clnt.o hw_xdr.o
	cc -O3 -Wall -L/usr/lib -libverbs -lpthread -o client main.o hw_clnt.o hw_xdr.o -lnsl

example64.o: 
	gcc -O3 -D_LARGEFILE64_SOURCE=1 -DHAVE_HIDDEN -I. -D_FILE_OFFSET_BITS=64 -c -o example64.o example.c

server: hw_server.o hw_svc.o hw_xdr.o rdma_queue2.o read_dev.o example64.o sha_funcs.o
	cc -O3 -Wall -mtune=cortex-a57 -mcpu=cortex-a57+crypto -L/usr/lib -libverbs -o server example64.o hw_server.o hw_svc.o hw_xdr.o rdma_queue2.o  read_dev.o armv8_hash_asm.S  sha_funcs.o -lnsl -L. libz.a

sha_test: sha_test.o sha_funcs.o
	cc -O3 -Wall -mtune=cortex-a57 -mcpu=cortex-a57+crypto -o sha_test sha_test.o armv8_hash_asm.S sha_funcs.o -lnsl

.PHONY: clean

clean:
	-rm *.o
	-rm client
	-rm server
	-rm hw.h
	-rm hw_clnt.c
	-rm hw_svc.c
	-rm sha_test
