CC=gcc
DFLAGS= -g
LIB=-lm

all : dosdbell dosdbellasim

dosdbell: dosdbell.o dostraffic.o models.o flowlist.o
	$(CC) $(DFLAGS) -o dosdbell dosdbell.o dostraffic.o models.o flowlist.o $(LIB)

dosdbellasim: dosdbellasim.o dostraffic.o models.o flowlist.o
	$(CC) $(DFLAGS) -o dosdbellasim dosdbellasim.o dostraffic.o models.o flowlist.o $(LIB)

dosdbelliasim.o: dosdbellasim.c dosdbell.h dostraffic.h flowlist.h
	$(CC) -c dosdbellasim.c $(CIDIR) $(DFLAGS) 
	
dosdbell.o: dosdbell.c dosdbell.h dostraffic.h flowlist.h
	$(CC) -c dosdbell.c $(CIDIR) $(DFLAGS) 

dostraffic.o: dostraffic.c dostraffic.h 
	$(CC) -c dostraffic.c $(CIDIR) $(DFLAGS)

models.o: models.c models.h 
	$(CC) -c models.c $(CIDIR) $(DFLAGS)

flowlist.o: flowlist.c flowlist.h 
	$(CC) -c flowlist.c $(CIDIR) $(DFLAGS)

clean:
	rm -f *.o
	rm -f dosdbell
