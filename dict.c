/*
 *  @file    dict.c
 *  @author  N. Devillard
 *  @date    Apr 2011
 *  @brief   Dictionary object
 *  @license MIT
 *  @date    Tue Apr  5 14:11:10 CEST 2011
 *
 *	dict: a stand-alone string/string associative array
 *
 *	This single-file implements a stand-alone string/string associative array,
 * similar to a Python dictionary or a Perl hash. The implementation is
 * derived from the Python dictionary with some additional tweaks and
 * optimizations due to the string/string specialization.
 *
 *	Sources:
 *	   Algorithm step-by-step description
 *	   	http://www.laurentluce.com/?p=249
 *	   Python implementation
 *		http://svn.python.org/projects/python/trunk/Objects/dictobject.c
 *
 *	A demo is included and compiled with -DMAIN. Use the Makefile to
 * create a benchmark program and run it.
 */
#include <errno.h>
#include <switch.h>
#include "dict.h"

#define DICT_MIN_SZ     8
/* Dummy pointer to reference deleted keys */
#define DUMMY_PTR       ((void*)-1)
/* Used to hash further when handling collisions */
#define PERTURB_SHIFT   5
/* Beyond this size, a dictionary will not be grown by the same factor */
#define DICT_BIGSZ      64000

/*
 * Specify which hash function to use
 * MurmurHash is fast but may not work on all architectures
 * Dobbs is a tad bit slower but not by much and works everywhere
 */
#define dict_hash   dict_hash_dobbs
/* #define dict_hash   dict_hash_murmur */

/* Forward definitions */
static int dict_resize(dict *d);

/*
 *	This hash function has been taken from an Article in Dr Dobbs Journal.
 * There are probably better ones out there but this one does the job.
 */
static unsigned dict_hash_dobbs(const char * key) {
    int         len;
    unsigned    hash;
    int         i;

    len = strlen(key);

    for (hash = 0, i = 0; i < len; i++) {
       hash += (unsigned)key[i];
       hash += (hash << 10);
       hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}

/* Murmur hash */
#if	0
static unsigned dict_hash_murmur(const char *key) {
    int         len;
    unsigned    h, k, seed;
    unsigned    m = 0x5bd1e995;
    int         r = 24;
    unsigned char *data;

    seed = 0x0badcafe;
    len  = (int)strlen(key);

    h = seed ^ len;
    data = (unsigned char *)key;

    while (len >= 4) {
        k = *(unsigned int *)data;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    switch(len) {
        case 3: h ^= data[2] << 16;
        case 2: h ^= data[1] << 8;
        case 1: h ^= data[0];
                h *= m;
    };

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}
#endif

/***********************
 * BEGIN: implementation
 ***********************/
/* Lookup an element in a dict
 *   This implementation copied almost verbatim from the Python dictionary
 *  object, without the Pythonisms.
 */
static keypair *dict_lookup(dict *d, const char *key, unsigned hash) {
    keypair *mem_freeslot;
    keypair *ep;
    unsigned i;
    unsigned perturb;

    if (!d || !key)
       return NULL;

    i = hash & (d->size-1);

    /* Look for empty slot */
    ep = d->table + i;

    if (ep->key == NULL || ep->key == key) {
       return ep;
    }

    if (ep->key == DUMMY_PTR) {
       mem_freeslot = ep;
    } else {
      if (ep->hash == hash && !strcmp(key, ep->key)) {
         return ep;
      }
      mem_freeslot = NULL;
    }

    for (perturb = hash ; ; perturb >>= PERTURB_SHIFT) {
        i = (i << 2) + i + perturb + 1;
        i &= (d->size-1);
        ep = d->table + i;

        if (ep->key == NULL) {
           return mem_freeslot == NULL ? ep : mem_freeslot;
        }
        if ((ep->key == key) || 
            (ep->hash == hash && ep->key != DUMMY_PTR &&
             !strcmp(ep->key, key))) {
           return ep;
        }
        if (ep->key == DUMMY_PTR && mem_freeslot == NULL) {
           mem_freeslot = ep;
        }
    }
    return NULL;
}

/* Add an item to a dictionary without copying key/val
 *	Used by dict_resize() only.
 */
static int dict_add_p(dict *d, const char *key, const char *val, const void *blob, switch_time_t ts, int copy) {
    unsigned  hash;
    keypair  *slot;

    if (!d || !key)
       return -1;

#if DEBUG>2
    printf("dict_add_p[%s][%s]\n", key, val ? val : "UNDEF");
#endif
    hash = dict_hash(key);
    slot = dict_lookup(d, key, hash);

    if (slot) {
       if (copy) {
          if (!(slot->key = strdup(key)))
             return -1;

          if (val) {
             slot->val = val ? strdup(val) : (char *)val;

             if (!slot->val) {
               switch_safe_free(slot->key);
               return -1;
             }
          }
       } else {
          slot->key  = (char *)key;
          slot->val  = (char *)val;
       }

       if (blob)
          slot->blob = (void *)&blob;

       slot->hash = hash;

       if (ts)
          slot->ts = ts;
       else
          slot->ts = switch_micro_time_now();

       d->used++;
       d->fill++;

       if ((3 * d->fill) >= (d->size * 2)) {
          if (dict_resize(d) != 0) {
             return -1;
          }
       }
    }
    return 0;
}


/* dict_add: Add an item to a dict, with timestamp at current time */
int dict_add(dict *d, const char *key, const char *val) {
    return dict_add_p(d, key, val, NULL, time(NULL), 1);
}

/* dict_add_ts: Add an item to a dict with chosen timestamp */
int dict_add_ts(dict *d, const char *key, const char *val, switch_time_t ts) {
    return dict_add_p(d, key, val, NULL, ts, 1);
}

/* dict_add_blob: Add a blob to a dict by with current timestamp */
int dict_add_blob(dict *d, const char *key, const void **ptr) {
    return dict_add_p(d, key, NULL, &ptr, time(NULL), 1);
}

/* dict_add_blob_ts: Add a blob to a dict with chosen timestamp */
int dict_add_blob_ts(dict *d, const char *key, const void **ptr, switch_time_t ts) {
    return dict_add_p(d, key, NULL, &ptr, ts, 1);
}

/*Resize a dictionary */
static int dict_resize(dict *d) {
    unsigned      newsize;
    keypair      *oldtable;
    unsigned      i;
    unsigned      oldsize;
    unsigned      factor;

    newsize = d->size;
    /*
     * Re-sizing factor depends on the current dict size.
     * Small dicts will expand 4 times, bigger ones only 2 times
     */
    factor = (d->size > DICT_BIGSZ) ? 2 : 4;
 
    while (newsize <= (factor * d->used)) {
       newsize*=2;
    }
 
    /* Exit early if no re-sizing needed */
    if (newsize==d->size)
       return 0;
#if DEBUG>2
    printf("resizing %d to %d (used: %d)\n", d->size, newsize, d->used);
#endif

    /* Shuffle pointers, re-allocate new table, re-insert data */
    oldtable = d->table;
    d->table = calloc(newsize, sizeof(keypair));

    if (!(d->table)) {
       /* Memory allocation failure */
       return -1;
    }

    oldsize  = d->size;
    d->size  = newsize;
    d->used  = 0;
    d->fill  = 0;

    for (i = 0; i < oldsize; i++)
      if (oldtable[i].key && (oldtable[i].key != DUMMY_PTR))
         dict_add_p(d, oldtable[i].key, oldtable[i].val, NULL, oldtable[i].ts, 0);

    switch_safe_free(oldtable);

    return 0;
}


/*
 * begin: implementation dict
 * scope: public
 *  desc: allocate a new dict
 */
dict *dict_new(void) {
    dict *d;

    d = calloc(1, sizeof(dict));

    if (!d)
       return NULL;

    d->size  = DICT_MIN_SZ;
    d->used  = 0;
    d->fill  = 0;
    d->table = calloc(DICT_MIN_SZ, sizeof(keypair));

    return d;
}

/* Public: deallocate a dict */
void dict_free(dict * d) {
    int i;
    
    if (!d)
       return;

    for (i=0; i < d->size; i++) {
      if (d->table[i].key && d->table[i].key != DUMMY_PTR) {
         switch_safe_free(d->table[i].key);
         if (d->table[i].val)
            switch_safe_free(d->table[i].val);
      }
    }

    switch_safe_free(d->table);
    switch_safe_free(d);

    return;
}

/* Public: get an item from a dict */
const char *dict_get(dict *d, const char *key, const char *defval) {
   keypair *kp;
   unsigned  hash;

   if (!d || !key)
      return defval;

   hash = dict_hash(key);
   kp = dict_lookup(d, key, hash);

   if (kp)
      return kp->val;

   return defval;
}

void *dict_get_blob(dict *d, const char *key, const void **defval) {
   keypair *kp;
   unsigned  hash;

   if (!d || !key)
      return defval;

   hash = dict_hash(key);
   kp = dict_lookup(d, key, hash);

   if (kp && kp->blob)
      return kp->blob;

   return defval;
}

/* Public: delete an item in a dict */
int dict_del(dict *d, const char *key) {
    unsigned    hash;
    keypair    *kp;

    if (!d || !key)
       return -1;

    hash = dict_hash(key);
    kp = dict_lookup(d, key, hash);

    if (!kp)
       return -1;

    if (kp->key && kp->key != DUMMY_PTR)
       switch_safe_free(kp->key);

    kp->key = DUMMY_PTR;

    if (kp->val)
       switch_safe_free(kp->val);

    kp->val = NULL;
    d->used --;

    return 0;
}

/* Public: enumerate a dictionary */
int dict_enumerate(dict *d, int rank, const char **key, const char **val, switch_time_t *ts) {
    if (!d || !key || !val || (rank < 0))
       return -1;

    while ((d->table[rank].key == NULL || d->table[rank].key == DUMMY_PTR) &&
           (rank<d->size))
       rank++;

    if (rank >= d->size) {
       *key = NULL;
       *val = NULL;
       ts = 0;
       rank = -1;
    } else {
       *key = d->table[rank].key;
       *val = d->table[rank].val;
       *ts = d->table[rank].ts;
       rank++;
    }

    return rank;
}

/* Public: dump a dict to a file pointer */
int dict_dump(dict *d, FILE *out) {
    const char *key;
    const char *val;
    int    rank = 0;
    int    errors = 0;
    switch_time_t ts = 0;

    if (!d || !out)
       return errors;

    while (1) {
       rank = dict_enumerate(d, rank, &key, &val, &ts);

       if (rank < 0)
          break;

//       if (fprintf(out, "#%s:ts=%lu\n", key, ts) < 0)
//          errors++;

       if (fprintf(out, "%s=%s\n", key, val ? val : "UNDEF") < 0)
          errors++;
    }

    return errors;
}

/*
 *     merge_type: 0 = Newer TS wins
 *                 1 = Older TS wins
 *                 2 = A wins
 *                 3 = B wins
 *                 4 = Create renamed key (.old)
 *                 5 = Create renamed key (.new)
 */
dict *dict_merge(dict *a, dict *b, int merge_type) {
   return NULL;
}

/*
 * Test harness
 */
#ifdef TEST_HARNESS
#define ALIGN   "%15s: %6.4f\n"
#define NKEYS   1024*1024

double epoch_double() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + (t.tv_usec * 1.0) / 1000000.0;
}

int main(int argc, char **argv) {
    dict *d;
    double t1, t2;
    int i;
    int nkeys;
    char *buffer;
    char *val;

    nkeys = (argc > 1) ? (int)atoi(argv[1]) : NKEYS;
    printf("%15s: %d\n", "values", nkeys);
    switch_malloc(buffer, 9 * nkeys);

    d = dict_new();
    t1 = epoch_double();
    for (i = 0; i < nkeys; i++) {
       sprintf(buffer + i * 9, "%08x", i);
    }
    t2 = epoch_double();
    printf(ALIGN, "initialization", t2 - t1);

    t1 = epoch_double();

    for (i = 0; i < nkeys; i++) {
       dict_add(d, buffer + i * 9, buffer + i * 9);
    }
    t2 = epoch_double();
    printf(ALIGN, "adding", t2 - t1);

    t1 = epoch_double();

    for (i = 0; i < nkeys; i++) {
       val = dict_get(d, buffer + i * 9, "UNDEF");
#if DEBUG > 1
       printf("exp[%s] got[%s]\n", buffer + i * 9, val);

       if (val && strcmp(val, buffer+i*9)) {
          printf("-> WRONG got[%s] exp[%s]\n", val, buffer + i * 9);
       }
#endif
    }
    t2 = epoch_double();
    printf(ALIGN, "lookup", t2 - t1);

    if (nkeys < 100)
       dict_dump(d, stdout);

    t1 = epoch_double();

    for (i = 0; i < nkeys; i++) {
       dict_del(d, buffer + i*9);
    }

    t2 = epoch_double();
    printf(ALIGN, "delete", t2 - t1);

    t1 = epoch_double();

    for (i = 0; i < nkeys; i++) {
       dict_add(d, buffer + i * 9, buffer + i * 9);
    }
    t2 = epoch_double();
    printf(ALIGN, "adding", t2 - t1);

    t1 = epoch_double();
    dict_free(d);
    t2 = epoch_double();
    printf(ALIGN, "free", t2 - t1);

    switch_safe_free(buffer);

    return 0;
}
#endif

int dict_getBool(dict *cp, const char *key, int def) {
   const char *str;

   if ((str = dict_get(cp, key, NULL))) {
      if (strcasecmp(str, "false") == 0 || strcasecmp(str, "no") == 0)
         return 0;
      else if (strcasecmp(str, "true") == 0 || strcasecmp(str, "yes") == 0)
         return 1;
   }

   return def;
}

const double dict_getDouble(dict *cp, const char *key, const double def) {
   char *end;
   const char *str;
   double val;

   if (!(str = dict_get(cp, key, NULL)))
      return def;

   errno = 0;
   val = strtod(str, &end);

   if (errno || (val == 0 && (end == str)))
      return def;

   return val;
}

const int dict_getInt(dict *cp, const char *key, const int def) {
   char *end;
   const char *str;
   int val;

   if (!(str = dict_get(cp, key, NULL)))
      return def;

   errno = 0;
   val = strtol(str, &end, 0);

   if (errno || (val == 0 && (end == str)))
      return def;

   return val;
   return def;
}
