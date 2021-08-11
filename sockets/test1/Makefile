
all: client server

client: client.o common.o
	cc -o client client.o common.o

server: server.o common.o
	cc -o server server.o common.o
clean:
	rm -f client client.o common.o server server.o
