#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
	int  udp_port;
	char db_host[256];
	int  db_port;
	char db_user[128];
	char db_password[256];
	char db_name[128];
} config_t;

/* Load configuration from an INI-style file.
 * Missing keys keep their default values. Returns 0 on success,
 * -1 if the file could not be opened (defaults are still applied). */
int config_load(config_t *cfg, const char *path);

/* Fill cfg with sensible defaults. */
void config_defaults(config_t *cfg);

#endif /* CONFIG_H */
