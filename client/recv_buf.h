#ifndef RECV_BUF_H
#define RECV_BUF_H

#include <streambuf>
#include <vector>
#include <cstdlib> /* std::size_t */
#include <sys/socket.h>

/**
 * Usage:
 * recv_buf buf(socket);
 * istream is(buf);
 * (Now you can use is as any other istream, e.g. is >> variable, is.get(), &c.
 */

class recv_buf : public std::streambuf {
public:
	explicit recv_buf(int sockfd, std::size_t buff_sz = 1024, std::size_t put_back = 8);

private:
	// overrides base class underflow()
	int_type underflow();

	// copy ctor and assignment are not implemented;
	// copying not allowed
	recv_buf(const recv_buf &);
	recv_buf &operator=(const recv_buf &);

private:
	int sockfd_;
	const std::size_t put_back_;

	std::vector<char> buffer_;
};

#endif /* RECV_BUF_H */
