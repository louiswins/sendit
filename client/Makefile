# Enable warnings! Warnings good!
CFLAGS += -Wall -pedantic

all : client

client: main.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -f client main.o
