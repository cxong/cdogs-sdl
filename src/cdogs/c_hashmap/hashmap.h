/*
 * Generic hashmap manipulation functions
 *
 * Originally by Elliot C Back - http://elliottback.com/wp/hashmap-implementation-in-c/
 *
 * Modified by Pete Warden to fix a serious performance problem, support strings as keys
 * and removed thread synchronization - http://petewarden.typepad.com
 */
#pragma once

#define MAP_MISSING -3  /* No such element */
#define MAP_FULL -2 	/* Hashmap is full */
#define MAP_OMEM -1 	/* Out of Memory */
#define MAP_OK 0 	/* OK */

/*
 * any_t is a pointer.  This allows you to put arbitrary structures in
 * the hashmap.
 */
typedef void *any_t;

/*
 * PFany is a pointer to a function that can take two any_t arguments
 * and return an integer. Returns status code..
 */
typedef int (*PFany)(any_t, any_t);

/*
 * map_t is a pointer to an internally maintained data structure.
 * Clients of this package do not need to know how hashmaps are
 * represented.  They see and manipulate only map_t's.
 */
typedef struct hashmap_map *map_t;

/*
 * Return an empty hashmap. Returns NULL if empty.
*/
map_t hashmap_new(void);

map_t hashmap_copy(const map_t in, any_t (*callback)(any_t));

/*
 * Iteratively call f with argument (item, data) for
 * each element data in the hashmap. The function must
 * return a map status code. If it returns anything other
 * than MAP_OK the traversal is terminated. f must
 * not reenter any hashmap functions, or deadlock may arise.
 */
int hashmap_iterate(map_t in, PFany f, any_t item);
int hashmap_iterate_keys(map_t in, PFany f, any_t item);
int hashmap_iterate_keys_sorted(map_t in, PFany f, any_t item);

/*
 * Add an element to the hashmap. Return MAP_OK or MAP_OMEM.
 */
int hashmap_put(map_t in, const char* key, any_t value);

/*
 * Get an element from the hashmap. Return MAP_OK or MAP_MISSING.
 */
int hashmap_get(const map_t in, const char* key, any_t *arg);

/*
 * Remove an element from the hashmap. Return MAP_OK or MAP_MISSING.
 */
int hashmap_remove(map_t in, char* key);

/*
 * Get any element. Return MAP_OK or MAP_MISSING.
 */
int hashmap_get_one(map_t m, any_t *arg);
int hashmap_get_one_key(map_t m, any_t *arg);

/*
* Remove all elements, with a custom callback to each element, so that they
* may be deallocated by the callback
*/
void hashmap_clear(map_t in, void(*callback)(any_t));

/*
 * Free the hashmap
 */
void hashmap_free(map_t in);

/*
* Free the hashmap, as well as a custom callback to each element, so that they
* may be deallocated by the callback
* It is a shortcut to hashmap_iterate with a deallocation function followed by
* hashmap_free.
*/
void hashmap_destroy(map_t in, void (*callback)(any_t));

/*
 * Get the current size of a hashmap
 */
int hashmap_length(map_t in);
