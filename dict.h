/*
 *  @file    dict.h
 *  @author  N. Devillard
 *  @date    Apr 2011
 *  @brief   Dictionary object
 *  @license MIT
 *  @date    Tue Apr  5 14:11:10 CEST 2011
 *
 *	 This file implements a basic string/string associative array that
 *  grows when needed to store all inserted key/value pairs. The implementation
 *  is very closely based on the Python dictionary, without the associated
 *  Pythonisms and with specification on string/string.
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
 *		   http://www.laurentluce.com/?p=249
 *   	Python implementation
 *		   http://svn.python.org/projects/python/trunk/Objects/dictobject.c
 *
 *	A demo is included and compiled with -DMAIN. Use the Makefile to
 * create a benchmark program and run it.
 */
#ifndef _DICT_H_
#define _DICT_H_

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/* Keypair: holds a key/value pair. Key must be a hashable C string */
typedef struct _keypair_ {
    char    *key;
    char    *val;
    /* Added by joseph to support a blob-type, storing only a pointer */
    void    *blob;
    time_t  ts;
    /* End AppWorx changes */
    unsigned  hash;
} keypair;

/* Dict is the only type needed for clients of the dict object */
typedef struct _dict_ {
    unsigned  fill;
    unsigned  used;
    unsigned  size;
    keypair *table;
} dict;

/*
 *  @brief    Allocate a new dictionary object
 *  @return   Newly allocated dict, to be freed with dict_free()
 *	 Constructor for the dict object.
 */
extern dict *dict_new(void);


/*
 *  @brief    Deallocate a dictionary object
 *  @param    d   dict to deallocate
 *  @return   void
 *	 This function will deallocate a dictionary and all data it holds.
 */
extern void   dict_free(dict *d);

/*
 *  @brief    Add an item to a dictionary
 *  @param    d       dict to add to
 *  @param    key     Key for the element to be inserted
 *  @param    val     Value to associate to the key
 *  @return   0 if Ok, something else in case of error
 *	Insert an element into a dictionary. If an element already exists with
 *  the same key, it is overwritten and the previous associated data are freed.
 */
extern int    dict_add(dict *d, const char *key, const char *val);

/*
 *  @brief    Get an item from a dictionary
 *  @param    d       dict to get item from
 *  @param    key     Key to look for
 *  @param    defval  Value to return if key is not found in dict
 *  @return   Element found, or defval
 *	  Get the value associated to a given key in a dict. If the key is
 *  not found, defval is returned.
 */
extern const char *dict_get(dict *d, const char *key, const char *defval);

/*
 *  @brief   Add a blob to the dictionary
 *		- used for storing arbitrary data
 */
extern int        dict_add_ts(dict *d, const char *key, const char *val, time_t ts);
extern int        dict_add_blob(dict *d, const char *key, const void **ptr);
extern int        dict_add_blob_ts(dict *d, const char *key, const void **ptr, time_t ts);
extern void *dict_get_blob(dict *d, const char *key, const void **defval);

/*
 *  @brief    Delete an item in a dictionary
 *  @param    d       dict where item is to be deleted
 *  @param    key     Key to look for
 *  @return   0 if Ok, something else in case of error
 *	 Delete an item in a dictionary. Will return 0 if item was correctly
 *  deleted and -1 if the item could not be found or an error occurred.
 */
extern int dict_del(dict *d, const char *key);

/*
 *  @brief    Enumerate a dictionary
 *  @param    d       dict to browse
 *  @param    rank    Rank to start the next (linear) search
 *  @param    key     Enumerated key (modified)
 *  @param    val     Enumerated value (modified)
 *  @return   int rank of the next item to enumerate, or -1 if end reached
 *	Enumerate a dictionary by returning all the key/value pairs it
 * contains. Start the iteration by providing rank=0 and two pointers that
 * will be modified to references inside the dict.
 *	The returned value is the immediate successor to the one being
 * returned, or -1 if the end of dict was reached.
 *	Do not free or modify the returned key/val pointers.
 *
 *	See dict_dump() for usage example.
 */

extern int dict_enumerate(dict *d, int rank, const char **key, const char **val, time_t *ts);

/*
 * @brief    Dump dict contents to an opened file pointer
 *  @param    d       dict to dump
 *  @param    out     File to output data to
 *  @return   int     Count of errors encountered
 *
 *	Dump the contents of a dictionary to an opened file pointer.
 *  It is Ok to pass 'stdout' or 'stderr' as file pointers.
 *
 *  This function is mostly meant for debugging purposes.
 */
extern int dict_dump(dict *d, FILE *out);

/* XXX: ToDo: write this
 *     merge_type: 0 = Newer TS wins
 *                 1 = Older TS wins
 *                 2 = A wins
 *                 3 = B wins
 *                 4 = Create renamed key (.old)
 *                 5 = Create renamed key (.new)
 */
extern dict *dict_merge(dict *a, dict *b, int merge_type);
extern const int dict_getInt(dict *cp, const char *key, const int def);
extern int  dict_getBool(dict *cp, const char *key, int def);
extern const char *dict_get(dict *cp, const char *key, const char *def);
extern const double dict_getDouble(dict *cp, const char *key, const double def);
extern void dict_mem_free(dict * d);

#endif
