CC = g++
# Enable warnings! Warnings good!
CFLAGS += -Wall -pedantic

all : client

client: main.o recv_buf.o process_http.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

main.o: recv_buf.h process_http.h
recv_buf.o: recv_buf.h
process_http.o: recv_buf.h process_http.h

%.o: %.c++
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f client *.o
