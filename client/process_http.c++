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


/* TODO:
 * support chunked format requests
 * http://jmarshall.com/easy/http/#http1.1s3
 */
string get_body(istream& is, http_info& headers) {
	if (!headers.headers.count("Content-Length")) return "";
	size_t len;
	istringstream(headers.headers["Content-Length"]) >> len;

	string body;
	body.reserve(len);
	for (size_t i = 0; i < len; ++i) {
		body.append(1, is.get());
	}

	return body;
}


istream& gethttpline(istream& in, string& str) {
	getline(in, str);
	size_t sz = str.size();
	// if the client uses proper HTTP (and ends lines with \r\n) we take
	// that into account
	if (sz > 0 && str[sz-1] == '\r') {
		str.resize(sz-1);
	}
	return in;
}

pair<string,string> extract_header(const string& line) {
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

http_info parse_headers(istream& in) {
	http_info ret;
	string curhead;

	gethttpline(in, curhead);
	istringstream httpstatus(curhead);
	httpstatus >> ret.method >> ret.request_uri >> ret.http_version;

#ifdef DEBUG
	cout << "Request Line: " << ret.method << ' ' << ret.request_uri << ' ' << ret.http_version << "\n\n" << "General Headers:" << '\n';
#endif

	// we do this to avoid messed up parsing on the request line or on an empty line.
	// if we receive messed-up headers, I don't feel bad about dying because someone
	// is doing something nefarious.
	gethttpline(in, curhead);
	while (!curhead.empty()) {
		try {
			ret.headers.insert(extract_header(curhead));
		} catch(runtime_error e) {
			cerr << "Error in header \"" << curhead << "\".\n";
		}
		
		gethttpline(in, curhead);
	}

#ifdef DEBUG
	for (map<string, string>::const_iterator b = ret.headers.begin(), e = ret.headers.end();
			b != e; ++b) {
		cout << b->first << ": " << b->second << '\n';
	}
#endif
	return ret;
}

string percent_decode(const string& uri) {
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

string get_boundary(const string& header) {
	size_t bpos = header.find("boundary=");
	if (bpos == string::npos)
		return "";
	return header.substr(bpos + sizeof("boundary=")-1);
}
