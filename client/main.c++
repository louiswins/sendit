#include "recv_buf.h"
#include <map>
#include <string>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#define PORT "12345"
#define RECVBUFLEN 2048
#define HTTPHEADEREND "\r\n\r\n"
#define TEE

#define RESPHEADER \
"HTTP/1.1 200 OK\r\n" \
"Content-Type: text/html\r\n\r\n"
#define RESPBODYGET \
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
#define RESPBODYPOST \
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n" \
"<html>\n" \
"<head>\n" \
"<meta http-equiv=\"Content-type\" content=\"text/html;charset=UTF-8\">\n" \
"<title>Done</title>\n" \
"<body>\n" \
"<p>Done. You've uploaded it.\n"


using namespace std;

struct http_info {
	string method,
	       request_uri,
	       http_version;
	map<string, string> headers;
};

int setup_sockets(struct addrinfo *theirs, socklen_t *addr_size);
/* Let's return the content length */
http_info parse_headers(int mysock);


int main(int argc, char *argv[]) {

	int accepted;
	struct addrinfo theirs;
	socklen_t addr_size;


	/* READ DATA */
	accepted = setup_sockets(&theirs, &addr_size);

	http_info headers = parse_headers(accepted);
	if (headers.http_version == "HTTP/1.1" && !headers.headers.count("Host")) {
		cerr << "Error: No \"Host\" header using HTTP v1.1. The user's sending bad data." << endl <<
			"http://www.w3.org/Protocols/rfc2616/rfc2616-sec19.html#sec19.6.1.1" << endl;
	} else if (headers.headers.count("Host")) {
		cout << "The user wanted the host " << headers.headers["Host"] << endl;
	}
#if 0
	printf("Parsed %d bytes of %d.\n", bufpos, RECVBUFLEN);
	if (recvbuf[bufpos]) {
		printf("Rest of recv()'d data: %s\n", recvbuf+bufpos);
	} else {
		printf("Finished parsing network data.\n");
	}
#endif

	send(accepted, RESPHEADER, sizeof(RESPHEADER), 0);
	send(accepted, RESPBODYGET, sizeof(RESPBODYGET), 0);

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


istream &gethttpline(istream &in, string &str) {
	getline(in, str);
	size_t sz = str.size();
	// if the client uses proper HTTP (and ends lines with \r\n) we take
	// that into account
	if (sz > 0 && str[sz-1] == '\r') {
		str.resize(sz-1);
	}
	return in;
}


http_info parse_headers(int sock) {
	recv_buf buf(sock);
	istream rvstream(&buf);

	http_info ret;

	string curhead;

	gethttpline(rvstream, curhead);
	istringstream httpstatus(curhead);
	httpstatus >> ret.method >> ret.request_uri >> ret.http_version;

#ifdef TEE
	cout << "Request Line: " << ret.method << ' ' << ret.request_uri << ' ' << ret.http_version << endl << endl << "General Headers:" << endl;
#endif

	// we do this to avoid messed up parsing on the request line or on an empty line.
	// if we receive messed-up headers, I don't feel bad about dying because someone
	// is doing something nefarious.
	gethttpline(rvstream, curhead);
	while (!curhead.empty()) {
		string key;
		istringstream tmp(curhead);
		getline(tmp, key, ':');
		if (curhead.size() == key.size()) {
			// there wasn't a ':', so there's something wrong.
			gethttpline(rvstream, curhead);
			continue;
		}
		string val = curhead.substr(key.size() + 1);
		while (val[0] == ' ')
			val.erase(0, 1);
		
		ret.headers[key] = val;
#ifdef TEE
		cout << key << ": " << ret.headers[key] << endl;
#endif
		gethttpline(rvstream, curhead);
	}
	return ret;
}
