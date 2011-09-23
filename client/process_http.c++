#include "recv_buf.h"
#include "process_http.h"
#include <utility>
#include <map>
#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstdlib>

using namespace std;

string get_body(int mysock, http_info headers) {
	if (!headers.headers.count("Content-Length")) return "";
	size_t len;
	istringstream(headers.headers["Content-Length"]) >> len;

	recv_buf buf(mysock);
	istream is(&buf);

	string body;
	body.reserve(len);
	for (size_t i = 0; i < len; ++i) {
		body.append(1, is.get());
	}

	return body;
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

pair<string,string> extract_header(const string &line) {
	size_t colon = line.find_first_of(':');
	if (colon == string::npos) {
		throw runtime_error("Bad request data");
	}
	string key = line.substr(0, colon);
	string val = line.substr(colon + 1);
	while (val[0] == ' ')
		val.erase(0,1);

	return make_pair(key, val);
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
	cout << "Request Line: " << ret.method << ' ' << ret.request_uri << ' ' << ret.http_version << "\n\n" << "General Headers:" << '\n';
#endif

	// we do this to avoid messed up parsing on the request line or on an empty line.
	// if we receive messed-up headers, I don't feel bad about dying because someone
	// is doing something nefarious.
	gethttpline(rvstream, curhead);
	while (!curhead.empty()) {
		try {
			ret.headers.insert(extract_header(curhead));
		} catch(runtime_error e) {
			continue;
		}
		
		gethttpline(rvstream, curhead);
	}
#ifdef TEE
	for (map<string,string>::const_iterator it = ret.headers.begin(); it != ret.headers.end(); ++it) {
		cout << it->first << ": " << it->second << '\n';
	}
#endif
	return ret;
}

string percent_decode(const string &uri) {
	string ret;
	for (string::const_iterator p = uri.begin(); p < uri.end(); ++p) {
		if (*p == '%') {
			char code[3] = { *(p+1), *(p+2) };
			int val;
			stringstream(code) >> hex >> val;
			ret += val;
			p += 2;
		} else {
			ret += *p;
		}
	}
	return ret;
}
