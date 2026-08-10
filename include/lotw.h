#ifndef LOTW_H
#define LOTW_H

#include "xmlparse.h"

/* Submit a single QSO to ARRL Logbook of The World (LoTW).
 *
 * The contact is rendered as a one-record ADIF document, digitally signed
 * with the private key/certificate contained in the supplied PKCS#12
 * (.p12) file, wrapped in the TQSL signed-upload envelope, and POSTed to
 * the LoTW upload endpoint over HTTPS.
 *
 * Parameters:
 *   c            - the QSO to submit (as parsed into a contact_t).
 *   p12_path     - path to the Logbook .p12 certificate file.
 *   p12_password - password protecting the .p12 (may be "" or NULL if none).
 *   station_call - station callsign to record as STATION_CALLSIGN; if NULL
 *                  the contact's own call (F_mycall) is used.
 *
 * Returns 0 on success (LoTW accepted the upload), -1 on any error.
 * All failures are logged via log_error().
 */
int lotw_submit_qso(const contact_t *c,
					const char *p12_path,
					const char *p12_password,
					const char *station_call);

#endif /* LOTW_H */

