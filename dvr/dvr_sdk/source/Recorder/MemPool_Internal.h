/*===========================================================================*\
 * Copyright 2003 O-Film Technologies, Inc., All Rights Reserved.
 * O-Film Confidential
 *
 * DESCRIPTION:
 *
 * ABBREVIATIONS:
 *   TODO: List of abbreviations used, or reference(s) to external document(s)
 *
 * TRACEABILITY INFO:
 *   Design Document(s):
 *     TODO: Update list of design document(s)
 *
 *   Requirements Document(s):
 *     TODO: Update list of requirements document(s)
 *
 *   Applicable Standards (in order of precedence: highest first):
 *
 * DEVIATIONS FROM STANDARDS:
 *   TODO: List of deviations from standards in this file, or
 *   None.
 *
\*===========================================================================*/
#ifndef _MEMORY_POOL_INTERNAL_H_
#define _MEMORY_POOL_INTERNAL_H_

#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif

/* Define the maximum object string name size in the system */
#define         MEMPOOL_MAX_NAME                         256

/* Define the Dynamic Pool Control Block data type.  */

typedef struct DM_PCB_STRUCT
{
	/* created dynamic pools  */
	unsigned int			dm_id;                 /* Internal PCB ID        */
	unsigned char           dm_name[MEMPOOL_MAX_NAME];  /* Dynamic Pool name      */
	void               		*dm_start_address;      /* Starting pool address  */
	unsigned int            dm_pool_size;          /* Size of pool           */
	unsigned int            dm_min_allocation;     /* Minimum allocate size  */
	unsigned int            dm_available;          /* Total available bytes  */
	struct DM_HEADER_STRUCT *dm_memory_list;        /* Memory list            */
	struct DM_HEADER_STRUCT	*dm_search_ptr;         /* Search pointer         */
} DM_PCB;

/* Define the header structure that is in front of each memory block.  */

typedef struct DM_HEADER_STRUCT
{
	struct DM_HEADER_STRUCT
		*dm_next_memory,        /* Next memory block      */
		*dm_previous_memory;    /* Previous memory block  */
	unsigned char		dm_memory_free;        /* Memory block free flag */

	DM_PCB             *dm_memory_pool;        /* Dynamic pool pointer   */
} DM_HEADER;

typedef         DM_PCB                              MEMORY_POOL;

#define         DM_DYNAMIC_ID          0x01314520UL
#define         DM_OVERHEAD            ((sizeof(DM_HEADER) + sizeof(unsigned int) \
                                        - 1)/sizeof(unsigned int)) *    \
                                        sizeof(unsigned int)

/* Macro used to check if a value is aligned to the required boundary.
Returns TRUE if aligned; FALSE if not aligned
NOTE:  The required alignment must be a power of 2 (2, 4, 8, 16, 32, etc) */
#define         MEM_ALIGNED_CHECK(value, req_align)                             \
                        (((unsigned int)(value) & ((unsigned int)(req_align) - (unsigned int)1)) == (unsigned int)0)

/* Macro used to align a data pointer to next address that meets the specified
required alignment.
NOTE:  The required alignment must be a power of 2 (2, 4, 8, 16, 32, etc)
NOTE:  The next aligned address will be returned regardless of whether the
data pointer is already aligned. */
#define         MEM_PTR_ALIGN(ptr_addr, req_align)                              \
                        (void *)(((unsigned int)(ptr_addr) & (unsigned int)(~((req_align) - 1))) +  \
                                  (unsigned int)(req_align))
#endif

