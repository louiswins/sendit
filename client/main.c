#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PORT "12345"
#define RECVBUFLEN 2048
#define HTTPHEADEREND "\r\n\r\n"
#define TEE

#define RESPHEADER \
"HTTP/1.1 200 OK\r\n" \
"Content-Type: text/html\r\n\r\n"
#define RESPBODY \
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n" \
"<html>\n" \
"<head>\n" \
"<meta http-equiv=\"Content-type\" content=\"text/html;charset=UTF-8\">\n" \
"<title>Upload</title>\n" \
"<body>\n" \
"<form enctype=\"multipart/form-data\" action=\".\" method=\"POST\">\n" \
"<p><label for=\"upfile\">Choose a file to upload: </label><input id=\"upfile\" name=\"upfile\" type=\"file\">\n" \
"<p><input type=\"submit\" value=\"Upload it\">\n" \
"</form>\n"



typedef int socketfd;
typedef enum {
	HTTP_UNKNOWN,
	HTTP_GET,
	HTTP_POST
} http_type;

socketfd setup_sockets(struct addrinfo *theirs, socklen_t *addr_size);
/* Let's return the content length */
int parse_headers(socketfd sock, char *recvbuf, int recvbuflen);


int main(int argc, char *argv[]) {

	socketfd accepted;
	struct addrinfo theirs;
	socklen_t addr_size;

	char recvbuf[RECVBUFLEN];
	int bufpos;

#if 0
	char headerbuf[sizeof(RESPHEADER)+10]; /* up to 10 digits for our body size */
	int headersize;
#endif

	/* READ DATA */
	accepted = setup_sockets(&theirs, &addr_size);

	bufpos = parse_headers(accepted, recvbuf, RECVBUFLEN);
	printf("Parsed %d bytes of %d.\n", bufpos, RECVBUFLEN);
	if (recvbuf[bufpos]) {
		printf("Rest of recv()'d data: %s\n", recvbuf+bufpos);
	} else {
		printf("Finished parsing network data.\n");
	}

#if 0
#ifdef NEEDLENGTH
	headersize = snprintf(headerbuf, sizeof(headerbuf), RESPHEADER, sizeof(RESPBODY)-1);
#else
	headersize = snprintf(headerbuf, sizeof(headerbuf), RESPHEADER);
#endif
	send(accepted, headerbuf, headersize, 0);
#endif
	send(accepted, RESPHEADER, sizeof(RESPHEADER), 0);
	send(accepted, RESPBODY, sizeof(RESPBODY), 0);

	close(accepted);

	exit(EXIT_SUCCESS);
}



socketfd setup_sockets(struct addrinfo *theirs, socklen_t *addr_size) {
	int status;
	struct addrinfo hints,
			*res,
			*p;
	socketfd sockfd,
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

	for (p = res; p != NULL; p = p->ai_next) {

		/* Get the server socket */
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("socket");
			continue;
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			exit(EXIT_FAILURE);
		}

		/* Bind to the port */
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("bind");
			continue;
		}

		break;
	}
	if (p == NULL) {
		fprintf(stderr, "Failed to bind.\n");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(res);

	/* Listen for incoming connections */
	if (listen(sockfd, 5) == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	/* Accept a connection */
	*addr_size = sizeof(*theirs);
	if ((ret = accept(sockfd, (struct sockaddr *)theirs, addr_size)) == -1) {
		perror("accept");
		exit(EXIT_FAILURE);
	}

	/* Free up the server data */
	close(sockfd);

	return ret;
}


int parse_headers(socketfd sock, char *recvbuf, int recvbuflen) {
	int bytesread;
	int heended = 0;
	int i = 0;
/*	int need_type = 1;*/

	while ((bytesread = recv(sock, recvbuf, recvbuflen, 0))) {
		if (bytesread == -1) {
			perror("recv");
			exit(EXIT_FAILURE);
		}
#if 0
		if (need_type) {
			if (!strncmp(recvbuf, "GET", 3)) {
				ret = HTTP_GET;
			} else if (!strncmp(recvbuf, "POST", 4)) {
				ret = HTTP_POST;
			} else {
				ret = HTTP_UNKNOWN;
			}
			need_type = 0;
		}
#endif
		for (i=0; i<bytesread; ++i) {
#ifdef TEE
			putchar(recvbuf[i]);
#endif
			if (recvbuf[i] == HTTPHEADEREND[heended]) {
				++heended;
				if (heended == sizeof(HTTPHEADEREND)-1) {
					break;
				}
			} else {
				heended = 0;
			}
		}
		if (heended == sizeof(HTTPHEADEREND)-1) {
			/* We've received \r\n\r\n */
			break;
		}
	}

	return i;
}
