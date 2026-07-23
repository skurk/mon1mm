#define _POSIX_C_SOURCE 200809L
#include "config.h"
#include "log.h"
#include "udp.h"
#include "xmlparse.h"
#include "db.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define UDP_BUF_SIZE 65536

static volatile sig_atomic_t g_running = 1;

static void handle_signal(int sig)
{
	(void)sig;
	g_running = 0;
}

static void install_signal_handlers(void)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handle_signal;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	signal(SIGPIPE, SIG_IGN);
}

int main(int argc, char **argv)
{
	const char *conf_path = (argc > 1) ? argv[1] : "./mon1mm.conf";
	config_t cfg;
	db_ctx_t *db;
	int sock;
	char *buf;

	if (config_load(&cfg, conf_path) != 0)
		log_warn("Could not open config '%s', using defaults", conf_path);
	else
		log_info("Loaded config from '%s'", conf_path);

	log_info("Starting mon1mm (udp_port=%d, db=%s@%s:%d/%s)",
			 cfg.udp_port, cfg.db_user, cfg.db_host, cfg.db_port, cfg.db_name);

	install_signal_handlers();

	db = db_connect(&cfg);
	if (!db) {
		log_error("Initial DB connection failed, exiting");
		return EXIT_FAILURE;
	}

	sock = udp_open(cfg.udp_port);
	if (sock < 0) {
		db_close(db);
		return EXIT_FAILURE;
	}
	log_info("Listening for N1MM UDP broadcasts on 0.0.0.0:%d", cfg.udp_port);

	buf = malloc(UDP_BUF_SIZE);
	if (!buf) {
		log_error("Out of memory");
		udp_close(sock);
		db_close(db);
		return EXIT_FAILURE;
	}

	while (g_running) {
		struct sockaddr_in src;
		socklen_t srclen = sizeof(src);
		ssize_t n;
		contact_t contact;

		n = recvfrom(sock, buf, UDP_BUF_SIZE - 1, 0,
					 (struct sockaddr *)&src, &srclen);
		if (n < 0) {
			if (errno == EINTR)
				continue; /* interrupted by signal */
			log_error("recvfrom failed: %s", strerror(errno));
			continue;
		}
		buf[n] = '\0';

		if (xml_parse(buf, (size_t)n, &contact) != 0)
			continue; /* parse error already logged */

		switch (contact.type) {
		case MSG_CONTACTINFO:
		case MSG_CONTACTREPLACE:
			if (!contact.set[F_ID])
				log_warn("contactinfo/replace without ID, skipping");
			else
				db_upsert_contact(db, &contact);
			break;
		case MSG_CONTACTDELETE:
			db_delete_contact(db, &contact);
			break;
		case MSG_NONE:
		default:
			/* ignore score, radio, spots, etc. */
			break;
		}
	}

	log_info("Shutting down...");
	free(buf);
	udp_close(sock);
	db_close(db);
	log_info("Stopped cleanly");
	return EXIT_SUCCESS;
}
