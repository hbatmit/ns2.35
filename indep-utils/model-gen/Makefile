######
CC = gcc
.c.o:
	$(CC) -g  -c $*.c

all: http_connect http_active
	$(CC) -g -o http_connect http_connect.o 

http_connect: http_connect.o 
	$(CC) -g -o http_connect http_connect.o 

http_active: http_active.o 
	$(CC) -g -o http_active http_active.o 

clean:  
	rm *.o http_connect http_active *.dat *.cdf core* inbound.* outbound.*

