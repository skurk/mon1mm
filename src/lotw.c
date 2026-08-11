
#include "lotw.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <openssl/pkcs12.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#include <curl/curl.h>

/* LoTW signed-upload endpoint. */
#define LOTW_UPLOAD_URL "https://lotw.arrl.org/lotw/upload"

/* --------------------------------------------------------------------------
 * Small helpers
 * ------------------------------------------------------------------------ */

/* Return the field value or "" if unset. Never returns NULL. */
static const char *fv(const contact_t *c, field_index_t i)
{
	return c->set[i] ? c->value[i] : "";
}

/* Append "<NAME:len>value" ADIF element to buf (bounded). */
static void adif_add(char *buf, size_t cap, const char *name, const char *val)
{
	size_t used = strlen(buf);
	if (!val || !*val)
		return;
	snprintf(buf + used, cap - used, "<%s:%zu>%s", name, strlen(val), val);
}

/* Extract "YYYYMMDD" and "HHMMSS" from an N1MM timestamp
 * "YYYY-MM-DD HH:MM:SS". Returns 0 on success. */
static int split_timestamp(const char *ts, char date[9], char time[7])
{
	int Y, Mo, D, H, Mi, S;
	if (!ts || sscanf(ts, "%d-%d-%d %d:%d:%d", &Y, &Mo, &D, &H, &Mi, &S) != 6)
		return -1;
	snprintf(date, 9, "%04d%02d%02d", Y, Mo, D);
	snprintf(time, 7, "%02d%02d%02d", H, Mi, S);
	return 0;
}

/* --------------------------------------------------------------------------
 * ADIF record construction
 * ------------------------------------------------------------------------ */

/* Build a one-QSO ADIF record (the fields LoTW cares about) into out.
 * Returns 0 on success. */
static int build_adif_record(const contact_t *c, const char *station_call,
							 char *out, size_t cap)
{
	char date[9] = {0}, time[7] = {0};
	const char *call = station_call && *station_call ? station_call : fv(c, F_mycall);

	if (split_timestamp(fv(c, F_timestamp), date, time) != 0) {
		log_error("lotw: contact has no/invalid timestamp");
		return -1;
	}
	if (!*fv(c, F_callsign)) {
		log_error("lotw: contact has no worked callsign");
		return -1;
	}

	out[0] = '\0';
	adif_add(out, cap, "STATION_CALLSIGN", call);
	adif_add(out, cap, "CALL", fv(c, F_callsign));
	adif_add(out, cap, "QSO_DATE", date);
	adif_add(out, cap, "TIME_ON", time);
	adif_add(out, cap, "BAND", fv(c, F_band));
	adif_add(out, cap, "MODE", fv(c, F_mode));
	adif_add(out, cap, "FREQ", fv(c, F_txfreq));
	adif_add(out, cap, "GRIDSQUARE", fv(c, F_gridsquare));
	adif_add(out, cap, "RST_SENT", fv(c, F_snt));
	adif_add(out, cap, "RST_RCVD", fv(c, F_rcv));
	{
		size_t used = strlen(out);
		snprintf(out + used, cap - used, "<EOR>");
	}
	return 0;
}

/* --------------------------------------------------------------------------
 * Certificate loading and signing
 * ------------------------------------------------------------------------ */

static void log_ssl_errors(const char *what)
{
	unsigned long e;
	char msg[256];
	while ((e = ERR_get_error()) != 0) {
		ERR_error_string_n(e, msg, sizeof msg);
		log_error("lotw: %s: %s", what, msg);
	}
}

/* Load private key and certificate from a PKCS#12 (.p12) file. */
static int load_p12(const char *path, const char *password,
					EVP_PKEY **pkey, X509 **cert)
{
	FILE *fp;
	PKCS12 *p12;
	int ok;

	*pkey = NULL;
	*cert = NULL;

	fp = fopen(path, "rb");
	if (!fp) {
		log_error("lotw: cannot open .p12 file '%s'", path);
		return -1;
	}

	p12 = d2i_PKCS12_fp(fp, NULL);
	fclose(fp);
	if (!p12) {
		log_ssl_errors("failed to parse .p12");
		return -1;
	}

	ok = PKCS12_parse(p12, password ? password : "", pkey, cert, NULL);
	PKCS12_free(p12);
	if (!ok) {
		log_ssl_errors("failed to decrypt .p12 (wrong password?)");
		return -1;
	}
	return 0;
}

/* Sign msg with pkey (RSA-SHA256) and return a malloc'd base64 signature.
 * Caller frees. Returns NULL on error. */
static char *sign_base64(EVP_PKEY *pkey, const char *msg)
{
	EVP_MD_CTX *ctx = EVP_MD_CTX_new();
	unsigned char *sig = NULL;
	size_t siglen = 0;
	char *b64 = NULL;

	if (!ctx) {
		log_ssl_errors("EVP_MD_CTX_new");
		return NULL;
	}

	if (EVP_DigestSignInit(ctx, NULL, EVP_sha256(), NULL, pkey) != 1 ||
		EVP_DigestSignUpdate(ctx, msg, strlen(msg)) != 1 ||
		EVP_DigestSignFinal(ctx, NULL, &siglen) != 1) {
		log_ssl_errors("DigestSign init/update");
		goto done;
	}

	sig = malloc(siglen);
	if (!sig)
		goto done;

	if (EVP_DigestSignFinal(ctx, sig, &siglen) != 1) {
		log_ssl_errors("DigestSignFinal");
		goto done;
	}

	/* base64-encode the signature. */
	{
		int enc_len = 4 * ((int)(siglen + 2) / 3);
		b64 = malloc(enc_len + 1);
		if (!b64)
			goto done;
		enc_len = EVP_EncodeBlock((unsigned char *)b64, sig, (int)siglen);
		b64[enc_len] = '\0';
	}

done:
	free(sig);
	EVP_MD_CTX_free(ctx);
	return b64;
}

/* --------------------------------------------------------------------------
 * Signed upload envelope
 * ------------------------------------------------------------------------ */

/* Wrap the ADIF record + signature into the TQSL signed-QSO XML envelope
 * that the LoTW upload endpoint expects. Returns a malloc'd string; caller
 * frees. Returns NULL on error. */
static char *build_signed_upload(const char *adif, const char *sig_b64, const char *station_call)
{
	static const char pre_call[]  = "<TQSL_UPLOAD><CALL>";
	static const char pre_rec[]   = "</CALL><RECORD>";
	static const char pre_sig[]   = "</RECORD><SIGNATURE ALG=\"RSA-SHA256\">";
	static const char post_sig[]  = "</SIGNATURE></TQSL_UPLOAD>";

	size_t l_call = strlen(station_call);
	size_t l_adif = strlen(adif);
	size_t l_sig  = strlen(sig_b64);
	size_t cap = (sizeof(pre_call) - 1) + l_call +
				 (sizeof(pre_rec)  - 1) + l_adif +
				 (sizeof(pre_sig)  - 1) + l_sig  +
				 (sizeof(post_sig) - 1) + 1;

	char *out = malloc(cap);
	if (!out) return NULL;

	/* Now for a dirty hack that makes GCC shut up about the %s warning.. */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"

	snprintf(out, cap, "%s%s%s%s%s%s%s",
						pre_call,
						station_call,
						pre_rec,
						adif,
						pre_sig,
						sig_b64,
						post_sig );

#pragma GCC diagnostic pop

	return out;
}

/* --------------------------------------------------------------------------
 * HTTP upload
 * ------------------------------------------------------------------------ */

struct resp_buf {
	char *data;
	size_t len;
};

static size_t collect_resp(void *ptr, size_t size, size_t nmemb, void *userp)
{
	struct resp_buf *r = userp;
	size_t add = size * nmemb;
	char *p = realloc(r->data, r->len + add + 1);
	if (!p)
		return 0;
	r->data = p;
	memcpy(r->data + r->len, ptr, add);
	r->len += add;
	r->data[r->len] = '\0';
	return add;
}

/* POST the signed payload to LoTW. Returns 0 if LoTW accepted it. */
static int upload_to_lotw(const char *payload)
{
	CURL *curl;
	CURLcode rc;
	long http_code = 0;
	struct resp_buf resp = {0};
	struct curl_slist *hdrs = NULL;
	int result = -1;

	curl = curl_easy_init();
	if (!curl) {
		log_error("lotw: curl_easy_init failed");
		return -1;
	}

	hdrs = curl_slist_append(hdrs, "Content-Type: application/octet-stream");

	curl_easy_setopt(curl, CURLOPT_URL, LOTW_UPLOAD_URL);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(payload));
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, collect_resp);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "n1mm2mysql-lotw/1.0");
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

	rc = curl_easy_perform(curl);
	if (rc != CURLE_OK) {
		log_error("lotw: upload failed: %s", curl_easy_strerror(rc));
		goto done;
	}

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
	if (http_code != 200) {
		log_error("lotw: server returned HTTP %ld: %s", http_code, resp.data ? resp.data : "(no body)");
		goto done;
	}

	/* LoTW reports per-record problems in the response body even on 200. */
	if (resp.data && (strstr(resp.data, "rejected")
			|| strstr(resp.data, "error")
			|| strstr(resp.data, "Error")))
	{
		log_error("lotw: upload rejected: %s", resp.data);
		goto done;
	}

	log_info("lotw: QSO accepted%s%s",
			 resp.data ? ": " : "", resp.data ? resp.data : "");
	result = 0;

done:
	free(resp.data);
	curl_slist_free_all(hdrs);
	curl_easy_cleanup(curl);
	return result;
}

/* --------------------------------------------------------------------------
 * Public entry point
 * ------------------------------------------------------------------------ */

int lotw_submit_qso(const contact_t *c,
					const char *p12_path,
					const char *p12_password,
					const char *station_call)
{
	char adif[FIELD_MAX * 4];
	EVP_PKEY *pkey = NULL;
	X509 *cert = NULL;
	char *sig_b64 = NULL;
	char *payload = NULL;
	const char *call;
	int rc = -1;

	if (!c || !p12_path) {
		log_error("lotw: invalid arguments");
		return -1;
	}

	call = station_call && *station_call ? station_call : fv(c, F_mycall);
	if (!*call) {
		log_error("lotw: no station callsign available");
		return -1;
	}

	if (build_adif_record(c, station_call, adif, sizeof adif) != 0)
		return -1;

	if (load_p12(p12_path, p12_password, &pkey, &cert) != 0)
		return -1;

	sig_b64 = sign_base64(pkey, adif);
	if (!sig_b64)
		goto done;

	payload = build_signed_upload(adif, sig_b64, call);
	if (!payload) {
		log_error("lotw: out of memory building upload");
		goto done;
	}

	rc = upload_to_lotw(payload);

done:
	free(payload);
	free(sig_b64);
	if (pkey)
		EVP_PKEY_free(pkey);
	if (cert)
		X509_free(cert);
	return rc;
}

