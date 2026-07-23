#ifndef DB_H
#define DB_H

#include "config.h"
#include "xmlparse.h"

typedef struct db_ctx db_ctx_t;

/* Create a DB context and connect. Returns NULL on failure. */
db_ctx_t *db_connect(const config_t *cfg);

/* Close and free a DB context. */
void db_close(db_ctx_t *db);

/* Upsert a contactinfo/contactreplace record. Returns 0 on success. */
int db_upsert_contact(db_ctx_t *db, const contact_t *c);

/* Delete a contact by ID. Returns 0 on success. */
int db_delete_contact(db_ctx_t *db, const contact_t *c);

#endif /* DB_H */
