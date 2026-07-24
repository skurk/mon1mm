#ifndef XMLPARSE_H
#define XMLPARSE_H

#include <stddef.h>

typedef enum {
	MSG_NONE = 0,
	MSG_CONTACTINFO,
	MSG_CONTACTREPLACE,
	MSG_CONTACTDELETE
} msg_type_t;

/* Maximum length stored for any single field value. */
#define FIELD_MAX 512

/* Field indices. Keep in sync with the field_names[] table in xmlparse.c
 * and with the column list used by db.c. */
typedef enum {
	F_app = 0,
	F_contestname,
	F_contestnr,
	F_timestamp,
	F_mycall,
	F_band,
	F_rxfreq,
	F_txfreq,
	F_operator,
	F_mode,
	F_callsign,
	F_countryprefix,
	F_wpxprefix,
	F_stationprefix,
	F_continent,
	F_snt,
	F_sntnr,
	F_rcv,
	F_rcvnr,
	F_gridsquare,
	F_exchange1,
	F_section,
	F_comment,
	F_qth,
	F_name,
	F_power,
	F_misc,
	F_zone,
	F_prec,
	F_ck,
	F_ismultiplier1,
	F_ismultiplier2,
	F_ismultiplier3,
	F_points,
	F_radionr,
	F_run1run2,
	F_RoverLocation,
	F_RadioInterfaced,
	F_NetworkedCompNr,
	F_IsOriginal,
	F_NetBiosName,
	F_IsRunQSO,
	F_Frequency,
	F_ID,
	F_IsClaimedQso,
	FIELD_COUNT
} field_index_t;

typedef struct {
	msg_type_t type;
	/* value[i] holds the text for field i; set[i] indicates presence. */
	char value[FIELD_COUNT][FIELD_MAX];
	int  set[FIELD_COUNT];
} contact_t;

/* Names of the XML child elements / DB columns, indexed by field_index_t. */
extern const char *const field_names[FIELD_COUNT];

/* Parse one datagram (XML document) into out.
 * Returns 0 on success (out->type set to the detected message type),
 * -1 on parse error. Unknown root elements yield type MSG_NONE and return 0. */
int xml_parse(const char *buf, size_t len, contact_t *out);

#endif /* XMLPARSE_H */
