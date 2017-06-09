/** 
 @file callbacks.c
 @brief ENet callback functions
*/
#define ENET_BUILDING_LIB 1
#include "enet/enet.h"

static void *glue_malloc(size_t s) { return malloc(s); }
static void glue_free(void *p) { free(p); }
static void glue_abort(void) { abort(); }

static ENetCallbacks callbacks = { glue_malloc, glue_free, glue_abort };

int
enet_initialize_with_callbacks (ENetVersion version, const ENetCallbacks * inits)
{
   if (version < ENET_VERSION_CREATE (1, 3, 0))
     return -1;

   if (inits -> malloc != NULL || inits -> free != NULL)
   {
      if (inits -> malloc == NULL || inits -> free == NULL)
        return -1;

      callbacks.malloc = inits -> malloc;
      callbacks.free = inits -> free;
   }
      
   if (inits -> no_memory != NULL)
     callbacks.no_memory = inits -> no_memory;

   return enet_initialize ();
}

ENetVersion
enet_linked_version (void)
{
    return ENET_VERSION;
}
           
void *
enet_malloc (size_t size)
{
   void * memory = callbacks.malloc (size);

   if (memory == NULL)
     callbacks.no_memory ();

   return memory;
}

void
enet_free (void * memory)
{
   callbacks.free (memory);
}

