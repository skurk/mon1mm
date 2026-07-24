#include "db.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mysql/mysql.h>

#define DB_MAX_RETRIES   5
#define DB_BACKOFF_BASE  1   /* seconds */

struct db_ctx {
	MYSQL   *conn;
	config_t cfg;
};

static int db_do_connect(db_ctx_t *db)
{
//	bool reconnect = 1;

	if (db->conn) {
		mysql_close(db->conn);
		db->conn = NULL;
	}

	db->conn = mysql_init(NULL);
	if (!db->conn) {
		log_error("mysql_init failed");
		return -1;
	}

//  MYSQL_OPT_RECONNECT is deprecated
//	mysql_options(db->conn, MYSQL_OPT_RECONNECT, &reconnect);

	if (!mysql_real_connect(db->conn, db->cfg.db_host, db->cfg.db_user,
							db->cfg.db_password, db->cfg.db_name,
							(unsigned int)db->cfg.db_port, NULL, 0)) {
		log_error("mysql_real_connect failed: %s", mysql_error(db->conn));
		mysql_close(db->conn);
		db->conn = NULL;
		return -1;
	}

	return 0;
}

/* Ensure a live connection, retrying with backoff. Returns 0 on success. */
static int db_ensure(db_ctx_t *db)
{
	int attempt;

	if (db->conn && mysql_ping(db->conn) == 0)
		return 0;

	for (attempt = 0; attempt < DB_MAX_RETRIES; attempt++) {
		int backoff = DB_BACKOFF_BASE << attempt;
		log_warn("DB connection down, reconnecting (attempt %d/%d)...",
				 attempt + 1, DB_MAX_RETRIES);
		if (db_do_connect(db) == 0) {
			log_info("DB reconnected");
			return 0;
		}
		sleep(backoff);
	}

	log_error("DB reconnect failed after %d attempts", DB_MAX_RETRIES);
	return -1;
}

db_ctx_t *db_connect(const config_t *cfg)
{
	db_ctx_t *db = calloc(1, sizeof(*db));
	if (!db)
		return NULL;

	db->cfg = *cfg;

	if (db_do_connect(db) != 0) {
		free(db);
		return NULL;
	}

	log_info("Connected to MySQL %s:%d db=%s", cfg->db_host, cfg->db_port,
			 cfg->db_name);
	return db;
}

void db_close(db_ctx_t *db)
{
	if (!db)
		return;
	if (db->conn)
		mysql_close(db->conn);
	free(db);
}

/* Build the upsert SQL statement text once. */
static const char *upsert_sql(void)
{
	static char sql[8192];
	static int  built = 0;
	int i;
	size_t n;

	if (built)
		return sql;

	n = 0;
	n += snprintf(sql + n, sizeof(sql) - n, "INSERT INTO contacts (");
	for (i = 0; i < FIELD_COUNT; i++)
		n += snprintf(sql + n, sizeof(sql) - n, "%s`%s`",
					  (i ? "," : ""), field_names[i]);
	n += snprintf(sql + n, sizeof(sql) - n, ") VALUES (");
	for (i = 0; i < FIELD_COUNT; i++)
		n += snprintf(sql + n, sizeof(sql) - n, "%s?", (i ? "," : ""));
	n += snprintf(sql + n, sizeof(sql) - n, ") ON DUPLICATE KEY UPDATE ");
	{
		int first = 1;
		for (i = 0; i < FIELD_COUNT; i++) {
			if (i == F_ID)
				continue; /* never update the key */
			n += snprintf(sql + n, sizeof(sql) - n, "%s`%s`=VALUES(`%s`)",
						  (first ? "" : ","), field_names[i], field_names[i]);
			first = 0;
		}
	}

	built = 1;
	return sql;
}

int db_upsert_contact(db_ctx_t *db, const contact_t *c)
{
	MYSQL_STMT   *stmt;
	MYSQL_BIND    bind[FIELD_COUNT];
	unsigned long lengths[FIELD_COUNT];
	bool       is_null[FIELD_COUNT];
	int           i, rc = -1;

	if (db_ensure(db) != 0)
		return -1;

	stmt = mysql_stmt_init(db->conn);
	if (!stmt) {
		log_error("mysql_stmt_init failed: %s", mysql_error(db->conn));
		return -1;
	}

	if (mysql_stmt_prepare(stmt, upsert_sql(), (unsigned long)strlen(upsert_sql()))) {
		log_error("upsert prepare failed: %s", mysql_stmt_error(stmt));
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind, 0, sizeof(bind));
	for (i = 0; i < FIELD_COUNT; i++) {
		is_null[i] = c->set[i] ? 0 : 1;
		lengths[i] = c->set[i] ? (unsigned long)strlen(c->value[i]) : 0;
		bind[i].buffer_type = MYSQL_TYPE_STRING;
		bind[i].buffer = (void *)c->value[i];
		bind[i].buffer_length = FIELD_MAX;
		bind[i].length = &lengths[i];
		bind[i].is_null = &is_null[i];
	}

	if (mysql_stmt_bind_param(stmt, bind)) {
		log_error("upsert bind failed: %s", mysql_stmt_error(stmt));
		goto done;
	}

	if (mysql_stmt_execute(stmt)) {
		log_error("upsert execute failed: %s", mysql_stmt_error(stmt));
		goto done;
	}

	rc = 0;
	log_info("Stored contact callsign=%s ID=%s",
			 c->set[F_callsign] ? c->value[F_callsign] : "?",
			 c->set[F_ID] ? c->value[F_ID] : "?");

done:
	mysql_stmt_close(stmt);
	return rc;
}

int db_delete_contact(db_ctx_t *db, const contact_t *c)
{
	MYSQL_STMT   *stmt;
	MYSQL_BIND    bind;
	unsigned long length;
	bool       is_null;
	static const char *sql = "DELETE FROM contacts WHERE `ID` = ?";
	int rc = -1;

	if (!c->set[F_ID]) {
		log_warn("contactdelete without ID, ignoring");
		return -1;
	}

	if (db_ensure(db) != 0)
		return -1;

	stmt = mysql_stmt_init(db->conn);
	if (!stmt) {
		log_error("mysql_stmt_init failed: %s", mysql_error(db->conn));
		return -1;
	}

	if (mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql))) {
		log_error("delete prepare failed: %s", mysql_stmt_error(stmt));
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(&bind, 0, sizeof(bind));
	is_null = 0;
	length = (unsigned long)strlen(c->value[F_ID]);
	bind.buffer_type = MYSQL_TYPE_STRING;
	bind.buffer = (void *)c->value[F_ID];
	bind.buffer_length = FIELD_MAX;
	bind.length = &length;
	bind.is_null = &is_null;

	if (mysql_stmt_bind_param(stmt, &bind)) {
		log_error("delete bind failed: %s", mysql_stmt_error(stmt));
		goto done;
	}

	if (mysql_stmt_execute(stmt)) {
		log_error("delete execute failed: %s", mysql_stmt_error(stmt));
		goto done;
	}

	rc = 0;
	log_info("Deleted contact callsign=%s ID=%s",
			 c->set[F_callsign] ? c->value[F_callsign] : "?", c->value[F_ID]);

done:
	mysql_stmt_close(stmt);
	return rc;
}
