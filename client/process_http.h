#ifndef PROCESS_HTTP_H
#define PROCESS_HTTP_H

#include <utility>
#include <string>
#include <map>
#include <iostream>

struct http_info {
	std::string method,
		request_uri,
	        http_version;
	std::map<std::string, std::string> headers;
};

/* Given a Content-Length header, this extracts the body of an
 * HTTP request from the socket mysock
 */
std::string get_body(std::istream& mysock, http_info& headers);

/* This is the same as getline() except that it finds a \r\n or \n
 * and discards the leading \r in that case.
 */
std::istream& gethttpline(std::istream& in, std::string& str);

/* This parses a line into a header pair<key, val>
 */
std::pair<std::string, std::string> extract_header(const std::string& line);

/* Parses the HTTP headers in the stream in.
 */
http_info parse_headers(std::istream& in);

/* A basic utility function for percent-decoding
 */
std::string percent_decode(const std::string& uri);

#endif /* PROCESS_HTTP_H */
