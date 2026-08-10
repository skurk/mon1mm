#ifndef DB_H
#define DB_H

#include "config.h"
#include "xmlparse.h"

typedef struct db_ctx db_ctx_t;
db_ctx_t *db_connect(const config_t *cfg);

void db_close(db_ctx_t *db);
int db_upsert_contact(db_ctx_t *db, const contact_t *c);
int db_delete_contact(db_ctx_t *db, const contact_t *c);
int db_select_unsynced(db_ctx_t *db);

#endif /* DB_H */
