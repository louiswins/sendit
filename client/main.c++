#include "recv_buf.h"
#include "process_http.h"
#include <map>
#include <string>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <cstdio> /* std::perror */
#include <cstdlib>
#include <unistd.h>

#define PORT "12345"

#define RESPHEADERGOOD \
"HTTP/1.1 200 OK\r\n" \
"Content-Type: text/html\r\n\r\n"
#define RESPHEADERBAD \
"HTTP/1.1 404 Not Found\r\n" \
"Content-Type: text/html\r\n\r\n" \
"Page does not exist.\r\n"
#define RESPBODYGET \
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n" \
"<html>\n" \
"<head>\n" \
"<meta http-equiv=\"Content-type\" content=\"text/html;charset=UTF-8\">\n" \
"<title>Upload</title>\n" \
"<body>\n" \
"<form enctype=\"multipart/form-data\" action=\".\" method=\"POST\">\n" \
"<p><label for=\"up\">Choose a file to upload: </label><input id=\"up\" name=\"up\" type=\"file\">\n" \
"<p><input type=\"submit\" value=\"Upload it\">\n" \
"</form>\n"
#define RESPBODYPOST \
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n" \
"<html>\n" \
"<head>\n" \
"<meta http-equiv=\"Content-type\" content=\"text/html;charset=UTF-8\">\n" \
"<title>Done</title>\n" \
"<body>\n" \
"<p>Done. You've uploaded it.\n"

#define DEFAULT_MAGIC "/"

using namespace std;

int setup_sockets(struct addrinfo *theirs, socklen_t *addr_size);


int main(int argc, char *argv[]) {

	string magic;
	if (argc <= 1) {
		magic = DEFAULT_MAGIC;
	} else {
		magic = argv[1];
	}
	if (magic[0] != '/') {
		magic = "/" + magic;
	}

	int accepted;
	struct addrinfo theirs;
	socklen_t addr_size;


	/* READ DATA */
	accepted = setup_sockets(&theirs, &addr_size);

	http_info headers = parse_headers(accepted);
	if (headers.http_version == "HTTP/1.1" && !headers.headers.count("Host")) {
		cerr << "Error: No \"Host\" header using HTTP v1.1. The user's sending bad data.\n"
			"http://www.w3.org/Protocols/rfc2616/rfc2616-sec19.html#sec19.6.1.1\n";
	} else if (headers.headers.count("Host")) {
#ifdef TEE
		cout << "The user wanted the host " << headers.headers["Host"] << "\n\n";
#endif
	}

	if (percent_decode(headers.request_uri) == magic) {
		send(accepted, RESPHEADERGOOD, sizeof(RESPHEADERGOOD), 0);
		if (headers.method == "GET") {
			send(accepted, RESPBODYGET, sizeof(RESPBODYGET), 0);
		} else if (headers.method == "POST") {
			string body = get_body(accepted, headers);
#ifdef TEE
			cout << "Body (true length " << body.size() << "):\n" << body;
#endif
			send(accepted, RESPBODYPOST, sizeof(RESPBODYPOST), 0);
		}
	} else {
		send(accepted, RESPHEADERBAD, sizeof(RESPHEADERBAD), 0);
	}

	close(accepted);

	exit(EXIT_SUCCESS);
}



int setup_sockets(struct addrinfo *theirs, socklen_t *addr_size) {
	int status;
	struct addrinfo hints,
			*res,
			*p;
	int sockfd,
	    ret;
	int yes=1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; /* IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* TCP */
	hints.ai_flags = AI_PASSIVE;

	/* getaddrinfo() to ser up the socket data */
	if ((status = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
		cerr << "getaddrinfo: " << gai_strerror(status) << '\n';
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
		cerr << "Failed to bind.\n";
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
