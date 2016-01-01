#
# Makefile -- for user threads library with synchronization
#
 
CC = g++

CFLAGS = -g -lpthread
 
all: p4.cpp
	$(CC) $(CFLAGS) -o p4 p4.cpp
	rm -rf endpoints
	rm -rf list*

clean: 
	rm -rf p4



#
# End of Makefile
#