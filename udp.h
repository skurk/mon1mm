#ifndef UDP_H
#define UDP_H

/* Open a UDP socket bound to 0.0.0.0:port.
 * Returns the socket fd on success, -1 on error. */
int udp_open(int port);

/* Close the UDP socket. */
void udp_close(int fd);

#endif /* UDP_H */
