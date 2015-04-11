# Introduction #

In order to implement LUDP packets, it is necessary to implement a cache which keeps track of nodes which an AOR-GLU node is currently communicating with.

The ChatterCache should include nodes which the current node has received data from recently (data traffic.) This allows for LUDP notifications to be sent specifically to nodes which are actively sending data to the current node.

**Implementation Steps**
  1. Add a new linked-list structure to the AORGLU Agent Class.
    * Create a ChatterEntry object which represents a single entry in the cache. This will include the LIST\_ENTRY member for the <sys/queue.h> list implementation.
    * Instantiate the head node in the global scope using LIST\_HEAD(..)
    * Add the new cache list head entity to the AORGLU Agent Class as chead.
  1. Initialize the ChatterList in the AORGLU constructor (LIST\_INIT(..))
  1. Add protected Queue management functions:
    * void cc\_insert( nsaddr\_t id) - Add a new ChatterCache entry to the ChatterList. This function checks for an existing entry. If an entry exists, it will not create a duplicate entry, but will refresh the expiration timer.
    * ChatterEntry`*` cc\_lookup( nsaddr\_t id ) - This function returns a pointer to the ChatterEntry object in the Agent ChatterCache with the specified id, or NULL if no id exists.
    * void cc\_purge() - Delete all expired nodes (CURRENT\_TIME > ce->expire).
  1. Create a timer structure that calls cc\_purge() periodically in order to remove any expired entries from the ChatterList.
    * The CC\_SAVE define (default to 30s) defines how long a chatter cache entry is valid.
    * The AORGLUChatterCacheTimer keeps track of purging dead entries from the ChatterList.

# Available Methods #
  * **void AORGLU::cc\_insert( nsaddr\_t id)** - Add a new ChatterCache entry to the ChatterList. This function checks for an existing entry. If an entry exists, it will not create a duplicate entry, but will refresh the expiration timer for the existing entry.

  * **ChatterEntry`*` AORGLU::cc\_lookup( nsaddr\_t id )** - This function returns a pointer to the ChatterEntry object in the Agent ChatterCache with the specified id, or NULL if no id exists.

  * **void AORGLU::cc\_purge()** - Delete all expired nodes (CURRENT\_TIME > ce->expire).