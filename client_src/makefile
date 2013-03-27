CC = gcc
CFLAGS = -Wall -O -g
TARGET = deduper_client 

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

SOURCES = $(wildcard *.c libs/*.c)
OBJS = $(patsubst %.c,%.o,$(SOURCES))

$(TARGET) : $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) -lpthread -lcrypto
	chmod a+x $(TARGET)

clean:
	rm -rf *.o libs/*.o
cleanall:
	rm -rf *.o libs/*.o  $(TARGET)
