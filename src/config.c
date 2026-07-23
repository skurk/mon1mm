#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void config_defaults(config_t *cfg)
{
	cfg->udp_port = 12060;
	snprintf(cfg->db_host, sizeof(cfg->db_host), "%s", "localhost");
	cfg->db_port = 3306;
	snprintf(cfg->db_user, sizeof(cfg->db_user), "%s", "root");
	cfg->db_password[0] = '\0';
	snprintf(cfg->db_name, sizeof(cfg->db_name), "%s", "n1mm");
}

static char *trim(char *s)
{
	char *end;
	while (*s && isspace((unsigned char)*s)) s++;
	if (*s == '\0') return s;
	end = s + strlen(s) - 1;
	while (end > s && isspace((unsigned char)*end)) *end-- = '\0';
	return s;
}

int config_load(config_t *cfg, const char *path)
{
	FILE *fp;
	char line[512];

	config_defaults(cfg);

	fp = fopen(path, "r");
	if (!fp) return -1;

	while (fgets(line, sizeof(line), fp)) {
		char *p = trim(line);
		char *eq, *key, *val;

		if (*p == '\0' || *p == '#' || *p == ';' || *p == '[') continue;

		eq = strchr(p, '=');
		if (!eq) continue;
		*eq = '\0';
		key = trim(p);
		val = trim(eq + 1);

		/* strip surrounding quotes on value */
		if (val[0] == '"' || val[0] == '\'') {
			size_t len = strlen(val);
			if (len >= 2 && val[len - 1] == val[0]) {
				val[len - 1] = '\0';
				val++;
			}
		}

		if (strcmp(key, "udp_port") == 0)
			cfg->udp_port = atoi(val);
		else if (strcmp(key, "db_host") == 0)
			snprintf(cfg->db_host, sizeof(cfg->db_host), "%s", val);
		else if (strcmp(key, "db_port") == 0)
			cfg->db_port = atoi(val);
		else if (strcmp(key, "db_user") == 0)
			snprintf(cfg->db_user, sizeof(cfg->db_user), "%s", val);
		else if (strcmp(key, "db_password") == 0)
			snprintf(cfg->db_password, sizeof(cfg->db_password), "%s", val);
		else if (strcmp(key, "db_name") == 0)
			snprintf(cfg->db_name, sizeof(cfg->db_name), "%s", val);
	}

	fclose(fp);
	return 0;
}
