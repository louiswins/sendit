/*
#ifndef RECV_BUF_H
#define RECV_BUF_H
*/

#include <streambuf>
#include <cstdlib> /* std::size_t */
#include <sys/socket.h>

class recv_buf : public std::streambuf {
public:
	explicit recv_buf(int sockfd, std::size_t buff_sz = 1024, std::size_t put_back = 8);
	~recv_buf() { delete[] buffer_; }

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

	const std::size_t size_;
	char *buffer_;
};

/*
#endif /* RECV_BUF_H */
