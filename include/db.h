#ifndef DB_H
#define DB_H

#include "config.h"
#include "xmlparse.h"

typedef struct db_ctx db_ctx_t;
db_ctx_t *db_connect(const config_t *);

void db_close(db_ctx_t *db);
int db_upsert_contact(db_ctx_t *, const contact_t *);
int db_delete_contact(db_ctx_t *, const contact_t *);
int db_select_unsynced(db_ctx_t *);
int db_buffer_store_contact(db_ctx_t *);
int db_tag_contact_as_stored(db_ctx_t *, char *);

#endif /* DB_H */
