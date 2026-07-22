#ifndef LOG_H
#define LOG_H

/* Print a timestamped log line to stdout (level "INFO") or stderr
 * (level "ERROR"/"WARN"). printf-style formatting. */
void log_msg(const char *level, const char *fmt, ...);

#define log_info(...)  log_msg("INFO",  __VA_ARGS__)
#define log_warn(...)  log_msg("WARN",  __VA_ARGS__)
#define log_error(...) log_msg("ERROR", __VA_ARGS__)

#endif /* LOG_H */
