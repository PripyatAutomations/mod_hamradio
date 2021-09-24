/*
 * No warranty of any kind. Good luck!
 */

#if	!defined(__CONFIG_H)
#define	__CONFIG_H
#include <fcntl.h>
#include <switch.h>
#include "dict.h"

extern int  dconf_get_bool(const char *key, const int def);
extern double dconf_get_double(const char *key, const double def);
extern int  dconf_get_int(const char *key, const int def);
extern char *dconf_get_str(const char *key, const char *def);
extern switch_time_t dconf_get_time(const char *key, const switch_time_t def);
extern int  dconf_set(const char *key, const char *val);
extern void dconf_unset(const char *key);
extern dict *dconf_load(const char *file);

#define	_CONF_DICT globals.cfg

#endif                                 /* !defined(__CONFIG_H) */
