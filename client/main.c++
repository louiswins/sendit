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

#define RESPHEADOK \
"HTTP/1.1 200 OK\r\n" \
"Server: sendit/0.11\r\n" \
"Connection: close\r\n" \
"Content-Type: text/html\r\n\r\n"

#define RESPHEADNFOUND \
"HTTP/1.1 404 Not Found\r\n" \
"Server: sendit/0.11\r\n" \
"Connection: close\r\n" \
"Content-Type: text/plain\r\n" \
"Content-Length: 21\r\n\r\n" \
"Page does not exist.\n"

#define RESPHEADCONTINUE \
"HTTP/1.1 100 Continue\r\n\r\n"

#define RESPHEADNOHOST \
"HTTP/1.1 400 Bad Request\r\n" \
"Server: sendit/0.11\r\n" \
"Connection: close\r\n" \
"Content-Type: text/html\r\n" \
"Content-Length: 111\r\n\r\n" \
"<html><body>\n" \
"<h2>No Host: header received</h2>\n" \
"HTTP 1.1 requests must include the Host: header.\n" \
"</body></html>\n"

#define RESPHEADNIMPL \
"HTTP/1.1 501 Not Implemented\r\n" \
"Server: sendit/0.11\r\n" \
"Connection: close\r\n" \
"Content-Type: text/plain\r\n" \
"Content-Length: 17\r\n\r\n" \
"Not implemented.\n"

#define RESPBODYGET_PREMAGIC \
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n" \
"<html>\n" \
"<head>\n" \
"<meta http-equiv=\"Content-type\" content=\"text/html;charset=UTF-8\">\n" \
"<title>Upload</title>\n" \
"<body>\n" \
"<form enctype=\"multipart/form-data\" action=\""
#define RESPBODYGET_POSTMAGIC \
"\" method=\"POST\">\n" \
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

int setup_server();
int receive_connection(int sockfd);


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
#ifdef DEBUG
	cout << "Listening for " << magic << "\n";
#endif

	int server,
	    accepted;

	string body;


	/* READ DATA */
	server = setup_server();
	accepted = receive_connection(server);

	recv_buf rvbuf(accepted);
	istream ist(&rvbuf);

	http_requestinfo req = parse_request(ist);
	http_headers headers = parse_headers(ist);
	if (req.http_version == "HTTP/1.1" && !headers.count("Host")) {
		send(accepted, RESPHEADNOHOST, sizeof(RESPHEADNOHOST)-1, 0);
		close(accepted);
		cerr << "Error: No \"Host\" header using HTTP v1.1. The user's sending bad data.\n"
			"See http://www.w3.org/Protocols/rfc2616/rfc2616-sec19.html#sec19.6.1.1\n";
		exit(EXIT_FAILURE);
#ifdef DEBUG
	} else if (headers.count("Host")) {
		cout << "The user wanted the host " << headers["Host"] << "\n\n";
#endif
	}

	if (req.method != "GET" && req.method != "POST") {
		send(accepted, RESPHEADNIMPL, sizeof(RESPHEADNIMPL)-1, 0);
	} else if (percent_decode(req.request_uri) == magic) {
		if (req.method == "GET") {
			send(accepted, RESPHEADOK, sizeof(RESPHEADOK)-1, 0);
			send(accepted, RESPBODYGET_PREMAGIC, sizeof(RESPBODYGET_PREMAGIC)-1, 0);
			send(accepted, magic.data(), magic.size(), 0);
			send(accepted, RESPBODYGET_POSTMAGIC, sizeof(RESPBODYGET_POSTMAGIC)-1, 0);
		} else { // POST
			if (req.http_version == "HTTP/1.1" && headers.count("Expect") && headers["Expect"] == "100-continue") {
				send(accepted, RESPHEADCONTINUE, sizeof(RESPHEADCONTINUE)-1, 0);
			}
#ifdef DEBUG
			if (headers.count("Content-Type")) {
				string bdy = get_boundary(headers["Content-Type"]);
				if (bdy.size()) {
					cout << "Boundary: \"" << bdy << "\"\n";
				}
			}
#endif
			body = get_body(ist, headers);
			send(accepted, RESPHEADOK, sizeof(RESPHEADOK)-1, 0);
			send(accepted, RESPBODYPOST, sizeof(RESPBODYPOST)-1, 0);
		}
	} else {
		send(accepted, RESPHEADNFOUND, sizeof(RESPHEADNFOUND)-1, 0);
	}

	close(server);
	close(accepted);

	cout << body;

	return 0;
}



int setup_server() {
	struct addrinfo hints,
			*res,
			*p;
	int sockfd;
	int yes=1;
	int status;

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

	return sockfd;
}


int receive_connection(int sockfd) {
	int ret;
	/* Accept a connection */
	if ((ret = accept(sockfd, NULL, NULL)) == -1) {
		perror("accept");
		exit(EXIT_FAILURE);
	}

	return ret;
}
