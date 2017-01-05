/*****************************************************************************\
*                                                                             *
*   File name	    dict.h						      *
*									      *
*   Description	    A general purpose dictionary "class" for C programs       *
*									      *
*   Notes:	    A dictionary is an associative array, where the key is    *
*		    a string, and the value is arbitrary.             	      *
*		    Also known as a map in the C++ standard template library. *
*									      *
*		    Implemented as a self-balancing binary tree.              *
*		    							      *
*   Usage:	    #include "dict.h"	   // Include in every module.	      *
*		    DICT_DEFINE_PROCS();   // Define in one of the modules.   *
*		    main() {                                                  *
*		      dict_t *dict = NewDict();				      *
*		      NewDictValue(dict, "one", "Number 1 definition");       *
*		      NewDictValue(dict, "two", "Number 2 definition");       *
*		      printf(DictValue(&dict, "one");			      *
*		    }							      *
*		    							      *
*   History:								      *
*    2010-07-09 JFL Created this module. 				      *
*    2016-12-31 JFL Renamed AddDictValue as NewDictValue, as documented above.*
*    2017-01-02 JFL Adapted to use SysToolsLib's debugm.h definitions.	      *
*    2017-01-04 JFL Added case-independant NewIDict().			      *
*                                                                             *
*         � Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

/* Implemented as a self-balancing binary tree */
#include "tree.h"
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
/* These are standard routines, but Microsoft thinks not */
#define strdup _strdup
#define snprintf _snprintf
#endif /* _WIN32 */

#ifdef __unix__		/* Automatically defined when targeting a Unix app. */
#include <strings.h>
#define _stricmp strcasecmp
#endif /* __unix__ */

/* Private structure used to implement the dictionary nodes */
typedef struct _dictnode {
  NODE_FIELDS(struct _dict_t, struct _dictnode);
  char *pszKey;
  void *pData;
} dictnode;
/* And the dictionary object */
typedef struct _dict_t {
  TREE_FIELDS(struct _dict_t, struct _dictnode);
  int (*keycmp)(const char *s1, const char *s2); /* Key comparison routine */
} dict_t;

/* Define types and declare functions for handling a tree of such structures */
TREE_DEFINE_TYPES(dict_t, dictnode);

/* Declare private static routines defined in the DICT_DEFINE_PROCS() macro. */
int TREE_CMP(dictnode)(dictnode *pn1, dictnode *pn2);
void *dictnode_tree_callback(dictnode *node, void *ref);

/* Declare public static routines */
extern void *NewDictValue(dict_t *dict, char *key, void *value);
extern void DeleteDictValue(dict_t *dict, char *key, void (*cb)(void *value));
extern void *DictValue(dict_t *dict, char *key);

/* Define private types used by the Foreach routine */
typedef void *DICT_CALLBACK_PROC(char *key, void *data, void *ref);
typedef struct {
  DICT_CALLBACK_PROC *cb;
  void *ref;
} DICT_CB_STRUCT;

/* Static routines that need to be defined once somewhere. */
#define DICT_DEFINE_PROCS()							\
TREE_DEFINE_PROCS(dict_t, dictnode);						\
										\
int TREE_CMP(dictnode)(dictnode *pn1, dictnode *pn2) {				\
  TREE_ENTRY((TREE_V(TREE_CMP(dictnode)) "(%p, %p)\n", pn1, pn2));		\
  TREE_IF_DEBUG({                                                               \
    char key1[80] = "NULL";			                                \
    char key2[80] = "NULL";			                                \
    char *pszKeyCmp = "NULL";							\
    if (iDebug) {								\
      if (pn1) snprintf(key1, sizeof(key1), "\"%s\"", pn1->pszKey);		\
      if (pn2) snprintf(key2, sizeof(key2), "\"%s\"", pn2->pszKey);		\
      if (pn1 && pn1->sbbt_tree && pn1->sbbt_tree->keycmp) {			\
      	if (pn1->sbbt_tree->keycmp == strcmp) pszKeyCmp = "strcmp";		\
	else if (pn1->sbbt_tree->keycmp == _stricmp) pszKeyCmp = "stricmp";	\
	else pszKeyCmp = "???";							\
      }										\
      DEBUG_PRINTF(("%s(%s, %s)\n", pszKeyCmp, key1, key2));			\
    }										\
  })                                                                            \
  TREE_RETURN_INT(pn1->sbbt_tree->keycmp(pn1->pszKey, pn2->pszKey));		\
}										\
                                                                                \
TREE_IF_DEBUG(                                                                  \
int TREE_SPRINT(dictnode)(char *buf, int len, dictnode *n) {			\
  if (!n) return snprintf(buf, len, "NULL");					\
  return snprintf(buf, len, "\"%s\"", n->pszKey);	                        \
}                                                                               \
)                                                                               \
                                                                                \
void *dictnode_tree_callback(dictnode *node, void *ref) {			\
  DICT_CB_STRUCT *s = ref;                                                      \
  return (s->cb)(node->pszKey, node->pData, s->ref);                            \
}                                                                               \
                                                                                \
void *NewDictValue(dict_t *dict, char *key, void *value) {                      \
  dictnode *node = calloc(1, sizeof(dictnode));                                 \
  TREE_ENTRY(("NewDictValue(%p, \"%s\", %p)\n", dict, key, value));		\
  if (node) {                                                                   \
    node->pszKey = strdup(key);                                                 \
    node->pData = value;                                                        \
    add_dictnode(dict, node);                                                   \
  }                                                                             \
  TREE_RETURN(node);                                                            \
}                                                                               \
                                                                                \
void DeleteDictValue(dict_t *dict, char *key, void (*cb)(void *value)) {        \
  dictnode *pNodeFound;                                                         \
  dictnode refNode = {0};                                                       \
  refNode.pszKey = key;                                                         \
  pNodeFound = get_dictnode(dict, &refNode);                                    \
  if (pNodeFound) {								\
    char *pszKey = pNodeFound->pszKey;						\
    if (cb) { /* If defined, call the destructor for the value. */              \
      cb(pNodeFound->pData);                                                    \
    }                                                                           \
    remove_dictnode(dict, pNodeFound);                                          \
    free(pszKey); /* Must be freed afterwards, because it's used by remove()! */\
  }                                                                             \
}                                                                               \
                                                                                \
void *DictValue(dict_t *dict, char *key) {                                      \
  dictnode *pNodeFound;                                                         \
  dictnode refNode = {0};                                                       \
  refNode.pszKey = key;                                                         \
  pNodeFound = get_dictnode(dict, &refNode);                                    \
  if (!pNodeFound) {                                                            \
    return 0;                                                                   \
  }                                                                             \
  return pNodeFound->pData;                                                     \
}                                                                               \

extern inline dict_t *NewDict(void) {
  dict_t *dict = new_dictnode_tree();
  if (dict) dict->keycmp = strcmp;	/* Case-dependant key comparison */
  return dict;
}

extern inline dict_t *NewIDict(void) {
  dict_t *dict = new_dictnode_tree();
  if (dict) dict->keycmp = _stricmp;	/* Case-independant key comparison */
  return dict;
}

extern inline void *ForeachDictValue(dict_t *dict, DICT_CALLBACK_PROC cb, void *ref) {
  DICT_CB_STRUCT s;
  s.cb = cb;
  s.ref = ref;
  return foreach_dictnode(dict, dictnode_tree_callback, &s);
}

extern inline dictnode *FirstDictValue(dict_t *dict) {
  return first_dictnode(dict);
}

extern inline dictnode *NextDictValue(dict_t *dict, dictnode *pn) {
  return next_dictnode(dict, pn);
}

extern inline dictnode *LastDictValue(dict_t *dict) {
  return last_dictnode(dict);
}

extern inline dictnode *PrevDictValue(dict_t *dict, dictnode *pn) {
  return prev_dictnode(dict, pn);
}

extern inline int GetDictSize(dict_t *dict) {
  return num_dictnode(dict);
}


