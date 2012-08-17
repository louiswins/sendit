#ifndef PROCESS_HTTP_H
#define PROCESS_HTTP_H

#include <utility>
#include <string>
#include <map>
#include <iostream>

typedef std::map<std::string, std::string> http_headers;
struct http_requestinfo {
	std::string method,
		request_uri,
	        http_version;
};

/* Given a Content-Length header, this extracts the body of an
 * HTTP request from the socket mysock
 */
std::string get_body(std::istream& mysock, http_headers& headers);

/* This is the same as getline() except that it finds a \r\n or \n
 * and discards the leading \r in that case.
 */
std::istream& gethttpline(std::istream& in, std::string& str);

/* This parses a line into a header pair<key, val>
 */
std::pair<std::string, std::string> extract_header(const std::string& line);

/* Parses the HTTP request in the stream in.
 */
http_requestinfo parse_request(std::istream& in);
/* Parses the HTTP headers in the stream in.
 */
http_headers parse_headers(std::istream& in);

/* A basic utility function for percent-decoding
 */
std::string percent_decode(const std::string& uri);
/* Extract boundary from header
 */
std::string get_boundary(const std::string& header);

#endif /* PROCESS_HTTP_H */
