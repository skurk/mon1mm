#include "udp.h"
#include "log.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

int udp_open(int port)
{
	int fd;
	int reuse = 1;
	struct sockaddr_in addr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		log_error("socket() failed: %s", strerror(errno));
		return -1;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
		log_warn("setsockopt(SO_REUSEADDR) failed: %s", strerror(errno));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons((unsigned short)port);

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		log_error("bind() to 0.0.0.0:%d failed: %s", port, strerror(errno));
		close(fd);
		return -1;
	}

	return fd;
}

void udp_close(int fd)
{
	if (fd >= 0)
		close(fd);
}
