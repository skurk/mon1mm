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
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define UDP_BUF_SIZE 65536

#define TIMER_SIGNAL (SIGRTMIN +1)

static volatile sig_atomic_t g_running = 1;
timer_t timerId;
struct sigaction lotwa;
db_ctx_t *db;
config_t cfg;

static void handle_signal(int sig)
{
	(void)sig;
	g_running = 0;
}

static void handle_lotw_autosync(int sig, siginfo_t *si, void *uc)
{
	(void)sig;
	(void)si;
	(void)uc;
	db_select_unsynced(db);
}

static void install_signal_handlers(void)
{
	// Handle ^C event
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handle_signal;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	signal(SIGPIPE, SIG_IGN);

	if(cfg.lotw_sync_interval > 0)
	{

		// Establish interval timer
		memset(&lotwa, 0, sizeof(lotwa));
		lotwa.sa_flags = SA_SIGINFO;
		lotwa.sa_sigaction = handle_lotw_autosync;

		sigset_t mask;
		sigemptyset(&mask);
		sigaddset(&mask, TIMER_SIGNAL);
		pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
		if(sigaction(TIMER_SIGNAL, &lotwa, NULL) == -1)
		{
			log_warn("sigaction() failed, LoTW sync is disabled");
			return;
		}

		struct sigevent sev;
		sev.sigev_notify = SIGEV_SIGNAL;
		sev.sigev_signo = TIMER_SIGNAL;
		sev.sigev_value.sival_ptr = &timerId;
		if(timer_create(CLOCK_REALTIME, &sev, &timerId) == -1)
		{
			log_warn("timer_create() failed, LoTW sync is disabled");
			return;
		}

		struct itimerspec its;
		its.it_value.tv_sec = cfg.lotw_sync_interval;
		its.it_value.tv_nsec = 0;
		its.it_interval.tv_sec = cfg.lotw_sync_interval;
		its.it_interval.tv_nsec = 0;
		if(timer_settime(timerId, 0, &its, NULL) == -1)
		{
			log_warn("timer_settime() failed, LoTW sync is disabled");
			return;
		}

		log_info("LoTW auto sync enabled, interval = %d seconds", cfg.lotw_sync_interval);
	}
}

int main(int argc, char **argv)
{
	const char *conf_path = (argc > 1) ? argv[1] : "./mon1mm.conf";
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

	log_blankline();
	log_info("Stopping application...");
	log_info("Shutting down...");

	free(buf);
	udp_close(sock);
	db_close(db);

	if(timerId)
	{
		log_info("Removing timer...");
		struct itimerspec zeroIts = {0};
		timer_settime(timerId, 0, &zeroIts, NULL);
		timer_delete(timerId);
	}

	log_info("Application stopped cleanly.");
	return EXIT_SUCCESS;
}
