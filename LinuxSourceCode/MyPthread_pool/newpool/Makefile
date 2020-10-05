CC = gcc
TARGET = pthread_pool2
SRC = pool.c pool_fuction.c
OBJECT = pool.o  pool_fuction.o
INCLUDES = -I./
LDFLAGS = -lpthread

all:$(TARGET)

$(OBJECT):$(SRC)
	$(CC) -c $(INCLUDES) ${SRC} 

$(TARGET):$(OBJECT)
	$(CC) -o $@ $(OBJECT) $(LDFLAGS) 

.PHONY:clean

clean:
	@rm -rf $(OBJECT) $(TARGET) *~
