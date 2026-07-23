#define _POSIX_C_SOURCE 200809L
#include "xmlparse.h"
#include "log.h"

#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <expat.h>

const char *const field_names[FIELD_COUNT] = {
	"app",
	"contestname",
	"contestnr",
	"timestamp",
	"mycall",
	"band",
	"rxfreq",
	"txfreq",
	"operator",
	"mode",
	"call",
	"countryprefix",
	"wpxprefix",
	"stationprefix",
	"continent",
	"snt",
	"sntnr",
	"rcv",
	"rcvnr",
	"gridsquare",
	"exchange1",
	"section",
	"comment",
	"qth",
	"name",
	"power",
	"misc",
	"zone",
	"prec",
	"ck",
	"ismultiplier1",
	"ismultiplier2",
	"ismultiplier3",
	"points",
	"radionr",
	"run1run2",
	"RoverLocation",
	"RadioInterfaced",
	"NetworkedCompNr",
	"IsOriginal",
	"NetBiosName",
	"IsRunQSO",
	"Frequency",
	"ID",
	"IsClaimedQso"
};

typedef struct {
	contact_t   *out;
	int          depth;        /* element nesting depth */
	int          cur_field;    /* index of the field currently being read, or -1 */
	size_t       cur_len;      /* accumulated length for the current field */
} parse_ctx_t;

static int field_lookup(const char *name)
{
	int i;
	for (i = 0; i < FIELD_COUNT; i++) {
		if (strcasecmp(name, field_names[i]) == 0)
			return i;
	}
	return -1;
}

static void XMLCALL start_element(void *userdata, const XML_Char *name,
								  const XML_Char **attr)
{
	parse_ctx_t *ctx = (parse_ctx_t *)userdata;
	(void)attr;

	ctx->depth++;

	if (ctx->depth == 1) {
		/* root element determines message type */
		if (strcasecmp(name, "contactinfo") == 0)
			ctx->out->type = MSG_CONTACTINFO;
		else if (strcasecmp(name, "contactreplace") == 0)
			ctx->out->type = MSG_CONTACTREPLACE;
		else if (strcasecmp(name, "contactdelete") == 0)
			ctx->out->type = MSG_CONTACTDELETE;
		else
			ctx->out->type = MSG_NONE;
		return;
	}

	if (ctx->depth == 2 && ctx->out->type != MSG_NONE) {
		int idx = field_lookup(name);
		ctx->cur_field = idx;
		ctx->cur_len = 0;
		if (idx >= 0) {
			ctx->out->value[idx][0] = '\0';
			ctx->out->set[idx] = 1;
		}
	}
}

static void XMLCALL end_element(void *userdata, const XML_Char *name)
{
	parse_ctx_t *ctx = (parse_ctx_t *)userdata;
	(void)name;

	if (ctx->depth == 2)
		ctx->cur_field = -1;

	ctx->depth--;
}

static void XMLCALL char_data(void *userdata, const XML_Char *s, int len)
{
	parse_ctx_t *ctx = (parse_ctx_t *)userdata;
	int idx = ctx->cur_field;
	size_t avail, copy;

	if (idx < 0)
		return;

	avail = FIELD_MAX - 1 - ctx->cur_len;
	if (avail == 0)
		return;

	copy = ((size_t)len < avail) ? (size_t)len : avail;
	memcpy(ctx->out->value[idx] + ctx->cur_len, s, copy);
	ctx->cur_len += copy;
	ctx->out->value[idx][ctx->cur_len] = '\0';
}

int xml_parse(const char *buf, size_t len, contact_t *out)
{
	XML_Parser parser;
	parse_ctx_t ctx;
	int rc = 0;

	memset(out, 0, sizeof(*out));
	out->type = MSG_NONE;

	ctx.out = out;
	ctx.depth = 0;
	ctx.cur_field = -1;
	ctx.cur_len = 0;

	parser = XML_ParserCreate(NULL);
	if (!parser) {
		log_error("XML_ParserCreate failed");
		return -1;
	}

	XML_SetUserData(parser, &ctx);
	XML_SetElementHandler(parser, start_element, end_element);
	XML_SetCharacterDataHandler(parser, char_data);

	if (XML_Parse(parser, buf, (int)len, 1) == XML_STATUS_ERROR) {
		log_error("XML parse error at line %lu: %s",
				  (unsigned long)XML_GetCurrentLineNumber(parser),
				  XML_ErrorString(XML_GetErrorCode(parser)));
		rc = -1;
	}

	XML_ParserFree(parser);
	return rc;
}
