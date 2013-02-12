/* Copied and reduced from glib's hashtable */
/* to do: change this into my own implementation */
#include "linux_ver.h"
#include "inc/xplatcfg.h"
#include "inc/sal/libc.h"
#include "inc/xlib/gtypes.h"
#include "inc/xlib/xhash.h"
#include "inc/sal/types.h"
#include "inc/sal/mem.h"
#include "inc/sal/debug.h"

#define HASH_TABLE_MIN_SIZE 11
#define HASH_TABLE_MAX_SIZE 13845163

#define DISABLE_MEM_POOLS
#define ENABLE_GC_FRIENDLY
#define g_new(stype, n) (stype*) sal_malloc(sizeof(stype) * (n))

#define g_free(p)            sal_free(p)
#define g_return_if_fail(expr)    do { sal_assert(expr); if (!(expr)) return; } while(0)
#define g_return_val_if_fail(expr, val)    do { sal_assert(expr); if (!(expr)) return val; } while(0)

#define G_STMT_START
#define G_STMT_END

#undef    CLAMP
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

typedef struct _GHashNode      GHashNode;

struct _GHashNode
{
  gpointer   key;
  gpointer   value;
  GHashNode *next;
};

struct _XLIB_HASH_TABLE
{
  gint             size;
  gint             nnodes;
  GHashNode      **nodes;
  GHashFunc        hash_func;
  GEqualFunc       key_equal_func;
  GDestroyNotify   key_destroy_func;
  GDestroyNotify   value_destroy_func;
};

typedef XLIB_HASH_TABLE GHashTable;

#define G_HASH_TABLE_RESIZE(hash_table)                \
   G_STMT_START {                        \
     if ((hash_table->size >= 3 * hash_table->nnodes &&            \
      hash_table->size > HASH_TABLE_MIN_SIZE) ||        \
     (3 * hash_table->size <= hash_table->nnodes &&            \
      hash_table->size < HASH_TABLE_MAX_SIZE))        \
       g_hash_table_resize (hash_table);            \
   } G_STMT_END

typedef gboolean  (*GHRFunc)  (gpointer  key,
                               gpointer  value,
                               gpointer  user_data);

static void        g_hash_table_resize      (GHashTable      *hash_table);
static GHashNode**    g_hash_table_lookup_node  (GHashTable     *hash_table,
                                                   gconstpointer   key);
static GHashNode*    g_hash_node_new          (gpointer       key,
                                                   gpointer        value);
static void        g_hash_node_destroy      (GHashNode      *hash_node,
                                                   GDestroyNotify  key_destroy_func,
                                                   GDestroyNotify  value_destroy_func);
static void        g_hash_nodes_destroy      (GHashNode      *hash_node,
                          GDestroyNotify   key_destroy_func,
                          GDestroyNotify   value_destroy_func);
static guint g_hash_table_foreach_remove_or_steal (GHashTable     *hash_table,
                                                   GHRFunc       func,
                                                   gpointer       user_data,
                                                   gboolean        notify);
LOCAL GHashTable* g_hash_table_new_full (GHashFunc       hash_func,
               GEqualFunc      key_equal_func,
               GDestroyNotify  key_destroy_func,
               GDestroyNotify  value_destroy_func);

#ifndef DISABLE_MEM_POOLS
G_LOCK_DEFINE_STATIC (g_hash_global);

static GMemChunk *node_mem_chunk = NULL;
static GHashNode *node_free_list = NULL;
#endif

LOCAL guint
g_direct_hash (gconstpointer v)
{
  return (guint)(gulong)v;
}

/**
 * g_hash_table_new:
 * @hash_func: a function to create a hash value from a key.
 *   Hash values are used to determine where keys are stored within the
 *   #GHashTable data structure. The g_direct_hash(), g_int_hash() and 
 *   g_str_hash() functions are provided for some common types of keys. 
 *   If hash_func is %NULL, g_direct_hash() is used.
 * @key_equal_func: a function to check two keys for equality.  This is
 *   used when looking up keys in the #GHashTable.  The g_direct_equal(),
 *   g_int_equal() and g_str_equal() functions are provided for the most
 *   common types of keys. If @key_equal_func is %NULL, keys are compared
 *   directly in a similar fashion to g_direct_equal(), but without the
 *   overhead of a function call.
 *
 * Creates a new #GHashTable.
 * 
 * Return value: a new #GHashTable.
 **/
LOCAL GHashTable*
g_hash_table_new (GHashFunc    hash_func,
          GEqualFunc   key_equal_func)
{
  return g_hash_table_new_full (hash_func, key_equal_func, NULL, NULL);
}


/**
 * g_hash_table_new_full:
 * @hash_func: a function to create a hash value from a key.
 * @key_equal_func: a function to check two keys for equality.
 * @key_destroy_func: a function to free the memory allocated for the key 
 *   used when removing the entry from the #GHashTable or %NULL if you 
 *   don't want to supply such a function.
 * @value_destroy_func: a function to free the memory allocated for the 
 *   value used when removing the entry from the #GHashTable or %NULL if 
 *   you don't want to supply such a function.
 * 
 * Creates a new #GHashTable like g_hash_table_new() and allows to specify
 * functions to free the memory allocated for the key and value that get 
 * called when removing the entry from the #GHashTable.
 * 
 * Return value: a new #GHashTable.
 **/
LOCAL GHashTable*
g_hash_table_new_full (GHashFunc       hash_func,
               GEqualFunc      key_equal_func,
               GDestroyNotify  key_destroy_func,
               GDestroyNotify  value_destroy_func)
{
  GHashTable *hash_table;
  gint i;
  
  hash_table = g_new (GHashTable, 1);
  if ( !hash_table ) return NULL;
  hash_table->size               = HASH_TABLE_MIN_SIZE;
  hash_table->nnodes             = 0;
  hash_table->hash_func          = hash_func ? hash_func : g_direct_hash;
  hash_table->key_equal_func     = key_equal_func;
  hash_table->key_destroy_func   = key_destroy_func;
  hash_table->value_destroy_func = value_destroy_func;
  hash_table->nodes              = g_new (GHashNode*, hash_table->size);
  
  for (i = 0; i < hash_table->size; i++)
    hash_table->nodes[i] = NULL;
  
  return hash_table;
}

/**
 * g_hash_table_destroy:
 * @hash_table: a #GHashTable.
 * 
 * Destroys the #GHashTable. If keys and/or values are dynamically 
 * allocated, you should either free them first or create the #GHashTable
 * using g_hash_table_new_full(). In the latter case the destroy functions 
 * you supplied will be called on all keys and values before destroying 
 * the #GHashTable.
 **/
LOCAL void
g_hash_table_destroy (GHashTable *hash_table)
{
  gint i;
  
  g_return_if_fail (hash_table != NULL);
  
  for (i = 0; i < hash_table->size; i++)
    g_hash_nodes_destroy (hash_table->nodes[i], 
              hash_table->key_destroy_func,
              hash_table->value_destroy_func);
  
  g_free (hash_table->nodes);
  g_free (hash_table);
}

static inline GHashNode**
g_hash_table_lookup_node (GHashTable    *hash_table,
              gconstpointer     key)
{
  GHashNode **node;
  
  node = &hash_table->nodes
    [(* hash_table->hash_func) (key) % hash_table->size];
  
  /* Hash table lookup needs to be fast.
   *  We therefore remove the extra conditional of testing
   *  whether to call the key_equal_func or not from
   *  the inner loop.
   */
  if (hash_table->key_equal_func)
    while (*node && !(*hash_table->key_equal_func) ((*node)->key, key))
      node = &(*node)->next;
  else
    while (*node && (*node)->key != key)
      node = &(*node)->next;
  
  return node;
}

/**
 * g_hash_table_lookup:
 * @hash_table: a #GHashTable.
 * @key: the key to look up.
 * 
 * Looks up a key in a #GHashTable.
 * 
 * Return value: the associated value, or %NULL if the key is not found.
 **/
LOCAL gpointer
g_hash_table_lookup (GHashTable      *hash_table,
             gconstpointer key)
{
  GHashNode *node;
  
  g_return_val_if_fail (hash_table != NULL, NULL);
  
  node = *g_hash_table_lookup_node (hash_table, key);
  
  return node ? node->value : NULL;
}

#if 0 /* not used */
/**
 * g_hash_table_lookup_extended:
 * @hash_table: a #GHashTable.
 * @lookup_key: the key to look up.
 * @orig_key: returns the original key.
 * @value: returns the value associated with the key.
 * 
 * Looks up a key in the #GHashTable, returning the original key and the
 * associated value and a #gboolean which is %TRUE if the key was found. This 
 * is useful if you need to free the memory allocated for the original key, 
 * for example before calling g_hash_table_remove().
 * 
 * Return value: %TRUE if the key was found in the #GHashTable.
 **/
LOCAL gboolean
g_hash_table_lookup_extended (GHashTable    *hash_table,
                  gconstpointer  lookup_key,
                  gpointer        *orig_key,
                  gpointer        *value)
{
  GHashNode *node;
  
  g_return_val_if_fail (hash_table != NULL, FALSE);
  
  node = *g_hash_table_lookup_node (hash_table, lookup_key);
  
  if (node)
    {
      if (orig_key)
    *orig_key = node->key;
      if (value)
    *value = node->value;
      return TRUE;
    }
  else
    return FALSE;
}
#endif
/**
 * g_hash_table_insert:
 * @hash_table: a #GHashTable.
 * @key: a key to insert.
 * @value: the value to associate with the key.
 * 
 * Inserts a new key and value into a #GHashTable.
 * 
 * If the key already exists in the #GHashTable its current value is replaced
 * with the new value. If you supplied a @value_destroy_func when creating the 
 * #GHashTable, the old value is freed using that function. If you supplied
 * a @key_destroy_func when creating the #GHashTable, the passed key is freed 
 * using that function.
 **/
LOCAL void
g_hash_table_insert (GHashTable *hash_table,
             gpointer     key,
             gpointer     value)
{
  GHashNode **node;
  
  g_return_if_fail (hash_table != NULL);
  
  node = g_hash_table_lookup_node (hash_table, key);
  
  if (*node)
    {
      /* do not reset node->key in this place, keeping
       * the old key is the intended behaviour. 
       * g_hash_table_replace() can be used instead.
       */

      /* free the passed key */
      if (hash_table->key_destroy_func)
    hash_table->key_destroy_func (key);
      
      if (hash_table->value_destroy_func)
    hash_table->value_destroy_func ((*node)->value);

      (*node)->value = value;
    }
  else
    {
      *node = g_hash_node_new (key, value);
      hash_table->nnodes++;
      G_HASH_TABLE_RESIZE (hash_table);
    }
}

#if 0 /* not used */
/**
 * g_hash_table_replace:
 * @hash_table: a #GHashTable.
 * @key: a key to insert.
 * @value: the value to associate with the key.
 * 
 * Inserts a new key and value into a #GHashTable similar to 
 * g_hash_table_insert(). The difference is that if the key already exists 
 * in the #GHashTable, it gets replaced by the new key. If you supplied a 
 * @value_destroy_func when creating the #GHashTable, the old value is freed 
 * using that function. If you supplied a @key_destroy_func when creating the 
 * #GHashTable, the old key is freed using that function. 
 **/
LOCAL void
g_hash_table_replace (GHashTable *hash_table,
              gpointer      key,
              gpointer      value)
{
  GHashNode **node;
  
  g_return_if_fail (hash_table != NULL);
  
  node = g_hash_table_lookup_node (hash_table, key);
  
  if (*node)
    {
      if (hash_table->key_destroy_func)
    hash_table->key_destroy_func ((*node)->key);
      
      if (hash_table->value_destroy_func)
    hash_table->value_destroy_func ((*node)->value);

      (*node)->key   = key;
      (*node)->value = value;
    }
  else
    {
      *node = g_hash_node_new (key, value);
      hash_table->nnodes++;
      G_HASH_TABLE_RESIZE (hash_table);
    }
}
#endif
/**
 * g_hash_table_remove:
 * @hash_table: a #GHashTable.
 * @key: the key to remove.
 * 
 * Removes a key and its associated value from a #GHashTable.
 *
 * If the #GHashTable was created using g_hash_table_new_full(), the
 * key and value are freed using the supplied destroy functions, otherwise
 * you have to make sure that any dynamically allocated values are freed 
 * yourself.
 * 
 * Return value: %TRUE if the key was found and removed from the #GHashTable.
 **/
LOCAL gboolean
g_hash_table_remove (GHashTable       *hash_table,
             gconstpointer  key)
{
  GHashNode **node, *dest;
  
  g_return_val_if_fail (hash_table != NULL, FALSE);
  
  node = g_hash_table_lookup_node (hash_table, key);
  if (*node)
    {
      dest = *node;
      (*node) = dest->next;
      g_hash_node_destroy (dest, 
               hash_table->key_destroy_func,
               hash_table->value_destroy_func);
      hash_table->nnodes--;
  
      G_HASH_TABLE_RESIZE (hash_table);

      return TRUE;
    }

  return FALSE;
}

#if 0
/**
 * g_hash_table_steal:
 * @hash_table: a #GHashTable.
 * @key: the key to remove.
 * 
 * Removes a key and its associated value from a #GHashTable without
 * calling the key and value destroy functions.
 *
 * Return value: %TRUE if the key was found and removed from the #GHashTable.
 **/
LOCAL gboolean
g_hash_table_steal (GHashTable    *hash_table,
                    gconstpointer  key)
{
  GHashNode **node, *dest;
  
  g_return_val_if_fail (hash_table != NULL, FALSE);
  
  node = g_hash_table_lookup_node (hash_table, key);
  if (*node)
    {
      dest = *node;
      (*node) = dest->next;
      g_hash_node_destroy (dest, NULL, NULL);
      hash_table->nnodes--;
  
      G_HASH_TABLE_RESIZE (hash_table);

      return TRUE;
    }

  return FALSE;
}
#endif
/**
 * g_hash_table_foreach_remove:
 * @hash_table: a #GHashTable.
 * @func: the function to call for each key/value pair.
 * @user_data: user data to pass to the function.
 * 
 * Calls the given function for each key/value pair in the #GHashTable.
 * If the function returns %TRUE, then the key/value pair is removed from the
 * #GHashTable. If you supplied key or value destroy functions when creating
 * the #GHashTable, they are used to free the memory allocated for the removed
 * keys and values.
 * 
 * Return value: the number of key/value pairs removed.
 **/
LOCAL guint
g_hash_table_foreach_remove (GHashTable    *hash_table,
                 GHRFunc     func,
                 gpointer     user_data)
{
  g_return_val_if_fail (hash_table != NULL, 0);
  g_return_val_if_fail (func != NULL, 0);
  
  return g_hash_table_foreach_remove_or_steal (hash_table, func, user_data, TRUE);
}

#if 0 /* not used */
/**
 * g_hash_table_foreach_steal:
 * @hash_table: a #GHashTable.
 * @func: the function to call for each key/value pair.
 * @user_data: user data to pass to the function.
 * 
 * Calls the given function for each key/value pair in the #GHashTable.
 * If the function returns %TRUE, then the key/value pair is removed from the
 * #GHashTable, but no key or value destroy functions are called.
 * 
 * Return value: the number of key/value pairs removed.
 **/
LOCAL guint
g_hash_table_foreach_steal (GHashTable *hash_table,
                            GHRFunc    func,
                            gpointer    user_data)
{
  g_return_val_if_fail (hash_table != NULL, 0);
  g_return_val_if_fail (func != NULL, 0);
  
  return g_hash_table_foreach_remove_or_steal (hash_table, func, user_data, FALSE);
}
#endif
static guint
g_hash_table_foreach_remove_or_steal (GHashTable *hash_table,
                                      GHRFunc      func,
                                      gpointer      user_data,
                                      gboolean    notify)
{
  GHashNode *node, *prev;
  gint i;
  guint deleted = 0;
  
  for (i = 0; i < hash_table->size; i++)
    {
    restart:
      
      prev = NULL;
      
      for (node = hash_table->nodes[i]; node; prev = node, node = node->next)
    {
      if ((* func) (node->key, node->value, user_data))
        {
          deleted += 1;
          
          hash_table->nnodes -= 1;
          
          if (prev)
        {
          prev->next = node->next;
          g_hash_node_destroy (node,
                       notify ? hash_table->key_destroy_func : NULL,
                       notify ? hash_table->value_destroy_func : NULL);
          node = prev;
        }
          else
        {
          hash_table->nodes[i] = node->next;
          g_hash_node_destroy (node,
                       notify ? hash_table->key_destroy_func : NULL,
                       notify ? hash_table->value_destroy_func : NULL);
          goto restart;
        }
        }
    }
    }
  
  G_HASH_TABLE_RESIZE (hash_table);
  
  return deleted;
}

/**
 * g_hash_table_foreach:
 * @hash_table: a #GHashTable.
 * @func: the function to call for each key/value pair.
 * @user_data: user data to pass to the function.
 * 
 * Calls the given function for each of the key/value pairs in the
 * #GHashTable.  The function is passed the key and value of each
 * pair, and the given @user_data parameter.  The hash table may not
 * be modified while iterating over it (you can't add/remove
 * items). To remove all items matching a predicate, use
 * g_hash_table_remove().
 **/
LOCAL void
g_hash_table_foreach (GHashTable *hash_table,
              GHFunc      func,
              gpointer      user_data)
{
  GHashNode *node;
  gint i;
  
  g_return_if_fail (hash_table != NULL);
  g_return_if_fail (func != NULL);
  
  for (i = 0; i < hash_table->size; i++)
    for (node = hash_table->nodes[i]; node; node = node->next)
      (* func) (node->key, node->value, user_data);
}

#if 0 /* not used */
/**
 * g_hash_table_find:
 * @hash_table: a #GHashTable.
 * @predicate:  function to test the key/value pairs for a certain property.
 * @user_data:  user data to pass to the function.
 * 
 * Calls the given function for key/value pairs in the #GHashTable until 
 * @predicate returns %TRUE.  The function is passed the key and value of 
 * each pair, and the given @user_data parameter. The hash table may not
 * be modified while iterating over it (you can't add/remove items). 
 *
 * Return value: The value of the first key/value pair is returned, for which 
 * func evaluates to %TRUE. If no pair with the requested property is found, 
 * %NULL is returned.
 *
 * Since: 2.4
 **/
LOCAL gpointer
g_hash_table_find (GHashTable       *hash_table,
                   GHRFunc        predicate,
                   gpointer        user_data)
{
  GHashNode *node;
  gint i;
  
  g_return_val_if_fail (hash_table != NULL, NULL);
  g_return_val_if_fail (predicate != NULL, NULL);
  
  for (i = 0; i < hash_table->size; i++)
    for (node = hash_table->nodes[i]; node; node = node->next)
      if (predicate (node->key, node->value, user_data))
        return node->value;       
  return NULL;
}
#endif
/**
 * g_hash_table_size:
 * @hash_table: a #GHashTable.
 * 
 * Returns the number of elements contained in the #GHashTable.
 * 
 * Return value: the number of key/value pairs in the #GHashTable.
 **/
LOCAL guint
g_hash_table_size (GHashTable *hash_table)
{
  g_return_val_if_fail (hash_table != NULL, 0);
  
  return hash_table->nnodes;
}

static guint 
g_spaced_primes_closest( guint N )
{
    guint i;
    
    if( N % 2 == 0 )
        N++;
    for( ; ; N += 2 )
    {
        for( i = 3; i * i <= N; i += 2 )
            if( N % i == 0 )
                goto ContOuter;  /* Sorry about this! */
        return N;
        ContOuter: ;
    }
}

static void
g_hash_table_resize (GHashTable *hash_table)
{
  GHashNode **new_nodes;
  GHashNode *node;
  GHashNode *next;
  guint hash_val;
  gint new_size;
  gint i;

  new_size = g_spaced_primes_closest (hash_table->nnodes);
  new_size = CLAMP (new_size, HASH_TABLE_MIN_SIZE, HASH_TABLE_MAX_SIZE);
 
  new_nodes = g_new (GHashNode*, new_size);
  sal_memset(new_nodes, 0, sizeof(GHashNode*) * new_size);
  
  for (i = 0; i < hash_table->size; i++)
    for (node = hash_table->nodes[i]; node; node = next)
      {
    next = node->next;

    hash_val = (* hash_table->hash_func) (node->key) % new_size;

    node->next = new_nodes[hash_val];
    new_nodes[hash_val] = node;
      }
  
  g_free (hash_table->nodes);
  hash_table->nodes = new_nodes;
  hash_table->size = new_size;
}

static GHashNode*
g_hash_node_new (gpointer key,
         gpointer value)
{
  GHashNode *hash_node;
  
#ifdef DISABLE_MEM_POOLS
  hash_node = g_new (GHashNode, 1);
#else
  G_LOCK (g_hash_global);
  if (node_free_list)
    {
      hash_node = node_free_list;
      node_free_list = node_free_list->next;
    }
  else
    {
      if (!node_mem_chunk)
    node_mem_chunk = g_mem_chunk_new ("hash node mem chunk",
                      sizeof (GHashNode),
                      1024, G_ALLOC_ONLY);
      
      hash_node = g_chunk_new (GHashNode, node_mem_chunk);
    }
  G_UNLOCK (g_hash_global);
#endif
  
  hash_node->key = key;
  hash_node->value = value;
  hash_node->next = NULL;
  return hash_node;
}

static void
g_hash_node_destroy (GHashNode      *hash_node,
             GDestroyNotify  key_destroy_func,
             GDestroyNotify  value_destroy_func)
{
  if (key_destroy_func)
    key_destroy_func (hash_node->key);
  if (value_destroy_func)
    value_destroy_func (hash_node->value);
  
#ifdef ENABLE_GC_FRIENDLY
  hash_node->key = NULL;
  hash_node->value = NULL;
#endif /* ENABLE_GC_FRIENDLY */

#ifdef DISABLE_MEM_POOLS
  g_free (hash_node);
#else
  G_LOCK (g_hash_global);
  hash_node->next = node_free_list;
  node_free_list = hash_node;
  G_UNLOCK (g_hash_global);
#endif
}

static void
g_hash_nodes_destroy (GHashNode *hash_node,
              GFreeFunc  key_destroy_func,
              GFreeFunc  value_destroy_func)
{
#ifdef DISABLE_MEM_POOLS
  while (hash_node)
    {
      GHashNode *next = hash_node->next;

      if (key_destroy_func)
    key_destroy_func (hash_node->key);
      if (value_destroy_func)
    value_destroy_func (hash_node->value);

      g_free (hash_node);
      hash_node = next;
    }  
#else
  if (hash_node)
    {
      GHashNode *node = hash_node;
  
      while (node->next)
    {
      if (key_destroy_func)
        key_destroy_func (node->key);
      if (value_destroy_func)
        value_destroy_func (node->value);

#ifdef ENABLE_GC_FRIENDLY
      node->key = NULL;
      node->value = NULL;
#endif /* ENABLE_GC_FRIENDLY */

      node = node->next;
    }

      if (key_destroy_func)
    key_destroy_func (node->key);
      if (value_destroy_func)
    value_destroy_func (node->value);

#ifdef ENABLE_GC_FRIENDLY
      node->key = NULL;
      node->value = NULL;
#endif /* ENABLE_GC_FRIENDLY */
 
      G_LOCK (g_hash_global);
      node->next = node_free_list;
      node_free_list = hash_node;
      G_UNLOCK (g_hash_global);
    }
#endif
}

/***************************************************/
/* Wrap ghash functions with xlib friendly funtions */
XLIB_HASH_TABLE* xlib_hash_table_new(XlibHashFunc hash_func, XlibEqualFunc key_equal_func)
{
    return g_hash_table_new((GHashFunc)hash_func, (GEqualFunc)key_equal_func);
}

XLIB_HASH_TABLE* xlib_hash_table_new_full(XlibHashFunc hash_func, XlibEqualFunc key_equal_func, 
    XlibDestroyNotify key_destroy, XlibDestroyNotify value_destroy)
{
    return g_hash_table_new_full((GHashFunc)hash_func, (GEqualFunc)key_equal_func,
        (GDestroyNotify)key_destroy, (GDestroyNotify)value_destroy);
}

void xlib_hash_table_destroy(XLIB_HASH_TABLE* table)
{
    g_hash_table_destroy(table);    
}

void* xlib_hash_table_lookup(XLIB_HASH_TABLE* table, const void* key)
{
    return g_hash_table_lookup(table, key);
}

void xlib_hash_table_insert(XLIB_HASH_TABLE* table, void* key, void* value)
{
    return g_hash_table_insert(table, key, value);
}

xbool xlib_hash_table_remove(XLIB_HASH_TABLE* table, const void* key)
{
    return g_hash_table_remove(table, key);
}

void xlib_hash_table_foreach(XLIB_HASH_TABLE* table, XlibHashIteFunc func, void* user_data)
{
    g_hash_table_foreach(table, func, user_data);
}

xuint32 xlib_hash_table_size(XLIB_HASH_TABLE* table)
{
    return g_hash_table_size(table);
}

xuint32 xlib_hash_table_foreach_remove(XLIB_HASH_TABLE* table, XlibHashMatchFunc func, void* user_data)
{
    return     g_hash_table_foreach_remove(table, (GHRFunc)func, user_data);
}
