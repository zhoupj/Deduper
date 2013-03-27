CC = gcc
CFLAGS = -Wall -g
TARGET = deduper_server 

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

SOURCES = $(wildcard *.c libs/*.c storage/*.c index/*.c job/*.c)
OBJS = $(patsubst %.c,%.o,$(SOURCES))

$(TARGET) : $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) -lpthread -lcrypto
	chmod a+x $(TARGET)

clean:
	rm -rf *.o libs/*.o storage/*.o index/*.o job/*.o
cleanall:
	rm $(TARGET)
	make clean
