SRC = main.c
OBJ = $(addsuffix .o, $(basename $(SRC)))
TARGET = $(basename $(OBJ))
CC = gcc
CCFLAGS =  

all: $(TARGET)

main: main.o util.o
	${CC} ${CCFLAGS} -o $@ main.o util.o

util.o: util.c util.h
	${CC} ${CCFLAGS} -c $<


.c.o:
	${CC} ${CCFLAGS} -c $<

clean:
	rm -f *.o $(TARGET) 
