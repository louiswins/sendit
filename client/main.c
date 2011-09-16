#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define PORT "12345"
#define RECVBUFLEN 10
#define HTTPHEADEREND "\r\n\r\n"

#ifdef NEEDLENGTH
#define RESPHEADER \
"HTTP/1.1 200 OK\r\n" \
"Content-Type: text/html\r\n" \
"Content-Length: %d\r\n" \
"\r\n"
#else
#define RESPHEADER \
"HTTP/1.1 200 OK\r\n" \
"Content-Type: text/html\r\n\r\n"
#endif
#define RESPBODY \
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n" \
"<html>\n" \
"<head>\n" \
"<meta http-equiv=\"Content-type\" content=\"text/html;charset=UTF-8\">\n" \
"<title>Louis Wins</title>\n" \
"<body>\n" \
"<h1>Louis Wins!</h1>\n" \
"<p>Get my <a href=\"louis.asc\">public key</a> to send me private stuff!\n"


typedef int socketfd;

socketfd setup_sockets(struct addrinfo *theirs, socklen_t *addr_size);


int main(int argc, char *argv[]) {

	socketfd accepted;
	struct addrinfo theirs;
	socklen_t addr_size;

	char recvbuf[RECVBUFLEN];
	int bytesread;
	int hestatus = 0;
	char headerbuf[sizeof(RESPHEADER)+10]; // up to 10 digits for our body size
	int headersize;

	accepted = setup_sockets(&theirs, &addr_size);

	while ((bytesread = recv(accepted, recvbuf, RECVBUFLEN, 0))) {
		int i=0;
		if (bytesread == -1) {
			perror("recv");
			exit(EXIT_FAILURE);
		}
		while (i < bytesread) {
			if (recvbuf[i] == HTTPHEADEREND[hestatus]) {
				++hestatus;
				if (hestatus == sizeof(HTTPHEADEREND)-1) {
					putchar(recvbuf[i]);
					break;
				}
			} else {
				hestatus = 0;
			}
			putchar(recvbuf[i++]);
		}
		if (hestatus == sizeof(HTTPHEADEREND)-1)
			break;
	}
#ifdef NEEDLENGTH
	headersize = snprintf(headerbuf, sizeof(headerbuf), RESPHEADER, sizeof(RESPBODY)-1);
#else
	headersize = snprintf(headerbuf, sizeof(headerbuf), RESPHEADER);
#endif
	send(accepted, headerbuf, headersize, 0);
	send(accepted, RESPBODY, sizeof(RESPBODY), 0);

	close(accepted);

	exit(EXIT_SUCCESS);
}



socketfd setup_sockets(struct addrinfo *theirs, socklen_t *addr_size) {
	int status;
	struct addrinfo hints,
			*res;
	socketfd serversock,
		 ret;
	int yes=1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; /* IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* TCP */
	hints.ai_flags = AI_PASSIVE;

	/* getaddrinfo() to ser up the socket data */
	if ((status = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		exit(EXIT_FAILURE);
	}

	/* Get the server socket */
	serversock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (setsockopt(serversock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	/* Bind to the port */
	if (bind(serversock, res->ai_addr, res->ai_addrlen) == -1) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	/* Listen for incoming connections */
	if (listen(serversock, 1) == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	/* Accept a connection */
	*addr_size = sizeof(*theirs);
	if ((ret = accept(serversock, (struct sockaddr *)theirs, addr_size)) == -1) {
		perror("accept");
		exit(EXIT_FAILURE);
	}

	/* Free up the server data */
	close(serversock);
	freeaddrinfo(res);

	return ret;
}
