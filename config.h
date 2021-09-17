/*
 * No warranty of any kind. Good luck!
 */

#if	!defined(__CONF_H)
#define	__CONF_H
#include <fcntl.h>
#include "dict.h"

extern int  dconf_get_bool(const char *key, const int def);
extern double dconf_get_double(const char *key, const double def);
extern int  dconf_get_int(const char *key, const int def);
extern char *dconf_get_str(const char *key, const char *def);
extern time_t dconf_get_time(const char *key, const time_t def);
extern int  dconf_set(const char *key, const char *val);
extern void dconf_unset(const char *key);
extern dict *dconf_load(const char *file);

#define	_CONF_DICT globals.cfg

#endif                                 /* !defined(__DCONF_H) */
