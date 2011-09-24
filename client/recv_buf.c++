#include "recv_buf.h"
#include <cstdlib> /* std::size_t */
#include <cstdio> /* perror */
#include <algorithm> /* std::max */
#include <vector>
#include <cstring> /* std::memmove */
#include <sys/socket.h> /* recv */

recv_buf::recv_buf(int sockfd, std::size_t buff_sz, std::size_t put_back) :
sockfd_(sockfd), put_back_(std::max(put_back, std::size_t(1))), buffer_(std::max(buff_sz, put_back_) + put_back_) {
	char *end = &buffer_.front() + buffer_.size();

	// we start setting eback(), gptr(), and egptr() to the same
	// location, to signal that we need to underflow().
	setg(end, end, end);
}

std::streambuf::int_type recv_buf::underflow() {
	if (gptr() < egptr()) // buffer not exhausted
		return traits_type::to_int_type(*gptr());

	char *base = &buffer_.front();
	char *start = base;

	if (eback() == base) { // true when this isn't the first fill
		// make arrangements for putback characters
		std::memmove(base, egptr() - put_back_, put_back_);
		start += put_back_;
	}

	// start is now the start of the buffer proper.
	// read from sockfd_ in to the provided buffer
	ssize_t n = recv(sockfd_, start, buffer_.size() - (start - base), 0);
	if (n == -1) {
		perror("recv");
		return traits_type::eof();
	}
	if (n == 0)
		return traits_type::eof();

	// set buffer pointers
	setg(base, start, start + n);
	return traits_type::to_int_type(*gptr());
}
