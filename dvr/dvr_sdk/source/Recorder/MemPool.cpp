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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MemPool.h"
#include "MemPool_Internal.h"
/*===========================================================================*\
 * Local Preprocessor #define Constants
\*===========================================================================*/

//static MEMORY_POOL uncached_mem_pool;

/*===========================================================================*\
 * Function Definitions
\*===========================================================================*/
static int  Create_Memory_Pool(
	MEMORY_POOL *pool_ptr,
	char *name,
	MemBlock_T *pBlockTbl,
	int nBlock,
	unsigned int min_allocation)
{
	DM_PCB       *pool;                  /* Pool control block ptr    */
	int             i;                      /* Working index variable    */
	DM_HEADER       *header_ptr;            /* Dynamic memory block header ptr */
	int          status = 0;                 /* Completion status         */
	unsigned int        adjusted_min;           /* Adjusted size of minimum  */
	unsigned int        adjusted_pool;          /* Adjusted size of pool     */
	unsigned int		pool_size = 0;
	void			*start_address;

	/* Move input pool pointer into internal pointer.  */
	pool = (DM_PCB *)pool_ptr;

	for (i = 0; i < nBlock; i++)
	{
		pool_size += pBlockTbl[i].size;
	}

	/* Adjust the minimum allocation size to something that is evenly
	divisible by the number of bytes in an UNSIGNED data type.  */
	adjusted_min = ((min_allocation + sizeof(unsigned int) - 1) / sizeof(unsigned int)) * sizeof(unsigned int);

	/* Adjust the pool size to something that is evenly divisible by the
	number of bytes in an UNSIGNED data type.  */
	adjusted_pool = ((pool_size + sizeof(unsigned int) - 1) / sizeof(unsigned int)) * sizeof(unsigned int);

	/* Check for a NULL dynamic memory pool control block pointer or a control
	block that is already created.  */
	if ((pool == NULL) || (pool->dm_id == DM_DYNAMIC_ID))
	{
		/* Invalid dynamic memory pool control block pointer.  */
		status = -1;
	}

	for (i = 0; i < nBlock; i++)
	{
		if (pBlockTbl[i].phyAddr == NULL)
		{
			/* Invalid memory pointer.  */
			status = -1;
		}
	}
	
	if ((adjusted_min == 0) || ((adjusted_min + (nBlock * 2 * DM_OVERHEAD)) > adjusted_pool))
	{
		/* Pool could not even accommodate one allocation.  */
		status = -1;
	}

	if (status)
	{
		return status;
	}

	/* First, clear the dynamic memory pool ID just in case it is an old
	pool control block.  */
	pool->dm_id = 0;

	if (name)
	{
		/* Fill in the dynamic memory pool name.
		NOTE:  Copy until the end of the string (NULL) or up to MEMPOOL_MAX_NAME - 1.
		This ensures the last character will be NULL terminated. */
		for (i = 0; (i < (MEMPOOL_MAX_NAME - 1)) && (name[i] != 0); i++)
		{
			pool->dm_name[i] = name[i];
		}

		/* Set last character in string to NULL */
		pool->dm_name[i] = 0;
	}

	/* Convert the pool's size into something that is evenly divisible by
	the sizeof an UNSIGNED data element.  */
	pool_size = (pool_size / sizeof(unsigned int)) * sizeof(unsigned int);

	for (i = 0; i < nBlock; i++)
	{
		start_address = (void*)pBlockTbl[i].phyAddr;

		/* Check if start address is aligned on an UNSIGNED boundary */
		if (MEM_ALIGNED_CHECK(start_address, sizeof(unsigned int)) == FALSE)
		{
			/* Align the start address on an UNSIGNED boundary */
			start_address = MEM_PTR_ALIGN(start_address, sizeof(unsigned int));
		}

		pBlockTbl[i].phyAddr = (void *)start_address;
	}

	/* Save the starting address and size parameters in the dynamic memory
	control block.  */
	start_address = (void*)pBlockTbl[0].phyAddr;

	pool->dm_start_address = start_address;
	pool->dm_pool_size = pool_size;
	pool->dm_min_allocation = ((min_allocation + sizeof(unsigned int) - 1) / sizeof(unsigned int)) * sizeof(unsigned int);

	/* Initialize the memory parameters.  */
	pool->dm_available = pool_size - (nBlock * 2 * DM_OVERHEAD);
	pool->dm_memory_list = (DM_HEADER *)start_address;
	pool->dm_search_ptr = (DM_HEADER *)start_address;

	for (i = 0; i < nBlock; i++)
	{
		header_ptr = (DM_HEADER *)pBlockTbl[i].phyAddr;
		/* Build the block header.  */
		header_ptr->dm_memory_pool = pool;
		header_ptr->dm_memory_free = TRUE;
		header_ptr->dm_next_memory = (DM_HEADER *)(((unsigned char *)header_ptr) + pBlockTbl[i].size - DM_OVERHEAD);

		if (i == 0)
		{
			header_ptr->dm_previous_memory = (DM_HEADER *)(((unsigned char *)pBlockTbl[nBlock - 1].phyAddr) + pBlockTbl[nBlock - 1].size - DM_OVERHEAD);
		}
		else
		{
			header_ptr->dm_previous_memory = (DM_HEADER *)(((unsigned char *)pBlockTbl[i - 1].phyAddr) + pBlockTbl[i - 1].size - DM_OVERHEAD);
		}

		if (nBlock == 1)
		{
			header_ptr = header_ptr->dm_next_memory;
			header_ptr->dm_next_memory = (DM_HEADER *)pBlockTbl[0].phyAddr;
			header_ptr->dm_previous_memory = (DM_HEADER *)pBlockTbl[0].phyAddr;
			header_ptr->dm_memory_pool = pool;
			header_ptr->dm_memory_free = FALSE;
		}

		if ((nBlock > 1) && (i < nBlock - 1))
		{
			header_ptr = header_ptr->dm_next_memory;
			header_ptr->dm_next_memory = (DM_HEADER *)pBlockTbl[i + 1].phyAddr;
			header_ptr->dm_previous_memory = (DM_HEADER *)pBlockTbl[i].phyAddr;
			header_ptr->dm_memory_pool = pool;
			header_ptr->dm_memory_free = FALSE;
		}
	}

	if (nBlock > 1)
	{
		header_ptr = (DM_HEADER *)(((unsigned char *)pBlockTbl[nBlock - 1].phyAddr) + pBlockTbl[nBlock - 1].size - DM_OVERHEAD);
		header_ptr->dm_next_memory = (DM_HEADER *)pBlockTbl[0].phyAddr;
		header_ptr->dm_previous_memory = (DM_HEADER *)pBlockTbl[nBlock - 1].phyAddr;
		header_ptr->dm_memory_pool = pool;
		header_ptr->dm_memory_free = FALSE;
	}
		
	/* At this point the dynamic memory pool is completely built.  The ID can
	now be set and it can be linked into the created dynamic memory
	pool list. */
	pool->dm_id = DM_DYNAMIC_ID;

	/* Return successful completion.  */
	return 0;

}

static int  Delete_Memory_Pool(MEMORY_POOL *pool_ptr)
{
	DM_PCB          *pool;                  /* Pool control block ptr    */
	int          status = 0;                 /* Completion status         */

	/* Move input pool pointer into internal pointer.  */
	pool = (DM_PCB *)pool_ptr;

	/* Determine if the dynamic memory pool pointer is valid.  */
	if ((pool) && (pool->dm_id == DM_DYNAMIC_ID))
	{
		/* Dynamic memory pool pointer is valid, call the function to
		delete it.  */

		/* Clear the memory pool ID.  */
		pool->dm_id = 0;
	}
	else
	{
		/* Dynamic memory pool pointer is invalid, indicate in
		completion status. */
		status = -1;
	}
	/* Return completion status.  */
	return(status);
}

static int  Allocate_Memory(MEMORY_POOL *pool_ptr, void **return_pointer, unsigned int size)
{
	DM_PCB       *pool;                  /* Pool control block ptr    */
	DM_HEADER    *memory_ptr;            /* Pointer to memory         */
	DM_HEADER    *new_ptr;               /* New split block pointer   */
	unsigned int     free_size;              /* Size of block found       */
	int       status = 0;                 /* Completion status         */

	/* Move input pool pointer into internal pointer.  */
	pool = (DM_PCB *)pool_ptr;

	/* Determine if dynamic memory pool pointer is invalid.  */
	if (pool == NULL)
	{
		/* Dynamic memory pool pointer is invalid, indicate in
		completion status. */
		status = -1;
	}
	else if (pool->dm_id != DM_DYNAMIC_ID)
	{
		/* Dynamic memory pool pointer is invalid, indicate in
		completion status. */
		status = -1;
	}
	else if (return_pointer == NULL)
	{
		/* Return pointer is invalid.  */
		status = -1;
	}
	else if ((size == 0) || (size > (pool->dm_pool_size - (2 * DM_OVERHEAD))))
	{
		/* Return the invalid size error.  */
		status = -1;
	}

	if (status)
	{
		return status;
	}

	/* Adjust the request to a size evenly divisible by the number of bytes
	in an UNSIGNED data element.  Also, check to make sure it is of the
	minimum size.  */
	if (size < pool->dm_min_allocation)
	{
		/* Change size to the minimum allocation.  */
		size = pool->dm_min_allocation;
	}
	else
	{
		/* Insure that size is a multiple of the UNSIGNED size.  */
		size = ((size + sizeof(unsigned int) - 1) / sizeof(unsigned int)) * sizeof(unsigned int);
	}

	/* Search the memory list for the first available block of memory that
	satisfies the request.  Note that blocks are merged during the
	deallocation function.  */
	memory_ptr = pool->dm_search_ptr;

	do
	{
		/* Determine if the block is free and if it can satisfy the request. */
		if (memory_ptr->dm_memory_free)
		{
			/* Calculate the free block size.  */
			free_size = (((unsigned char *)(memory_ptr->dm_next_memory)) -
				((unsigned char *)memory_ptr)) - DM_OVERHEAD;
		}
		else
		{
			/* There are no free bytes available.  */
			free_size = 0;
		}

		/* Determine if the search should continue.  */
		if (free_size < size)
		{
			/* Large enough block has not been found.  Move the search
			pointer to the next block.  */
			memory_ptr = memory_ptr->dm_next_memory;
		}

	} while ((free_size < size) && (memory_ptr != pool->dm_search_ptr));

	/* Determine if the memory is available.  */
	if (free_size >= size)
	{
		/* A block that satisfies the request has been found.  */
		/* Determine if the block needs to be split.  */
		if (free_size >= (size + DM_OVERHEAD + pool->dm_min_allocation))
		{
			/* Yes, split the block.  */
			new_ptr = (DM_HEADER *)(((unsigned char *)memory_ptr) + size + DM_OVERHEAD);

			/* Mark the new block as free.  */
			new_ptr->dm_memory_free = TRUE;

			/* Put the pool pointer into the new block.  */
			new_ptr->dm_memory_pool = pool;

			/* Build the necessary pointers.  */
			new_ptr->dm_previous_memory = memory_ptr;
			new_ptr->dm_next_memory = memory_ptr->dm_next_memory;
			(new_ptr->dm_next_memory)->dm_previous_memory = new_ptr;
			memory_ptr->dm_next_memory = new_ptr;

			/* Decrement the available byte count.  */
			pool->dm_available = pool->dm_available - size - DM_OVERHEAD;
		}
		else
		{
			/* Decrement the entire free size from the available bytes
			count.  */
			pool->dm_available = pool->dm_available - free_size;
		}

		/* Mark the allocated block as not available.  */
		memory_ptr->dm_memory_free = FALSE;

		/* Should the search pointer be moved?   */
		if (pool->dm_search_ptr == memory_ptr)
		{
			/* Move the search pointer to the next free memory slot.  */
			pool->dm_search_ptr = memory_ptr->dm_next_memory;
		}

		/* Return a memory address to the caller.  */
		*return_pointer = (void *)(((unsigned char *)memory_ptr) + DM_OVERHEAD);
	}
	else
	{
		/* Enough dynamic memory is not available. Simply return an error status. */
		status = -1;
		*return_pointer = NULL;
	}

	/* Return the completion status.  */
	return(status);
}

static int  Deallocate_Memory(void *memory)
{
	DM_PCB          *pool;                  /* Pool pointer              */
	DM_HEADER       *header_ptr;            /* Pointer to memory block   */
	DM_HEADER		*new_ptr;               /* New memory block pointer  */
	int          status = 0;                 /* Completion status         */

	/* Pickup the associated pool's pointer.  It is inside the header of
	each memory block.  */
	header_ptr = (DM_HEADER *)(((unsigned char*)memory) - DM_OVERHEAD);
	pool = header_ptr->dm_memory_pool;

	/* Determine if the pointer(s) are NULL.  */
	if ((header_ptr == NULL) || (memory == NULL))
	{
		/* Dynamic memory pointer is invalid.  */
		status = -1;
	}
	/* Determine if dynamic memory pool pointer is invalid.  */
	else if (pool == NULL)
	{
		/* Dynamic memory pointer is invalid, indicate in completion
		status.  */
		status = -1;
	}
	else if (pool->dm_id != DM_DYNAMIC_ID)
	{
		/* Dynamic memory pool pointer is invalid, indicate in completion
		status. */
		status = -1;
	}
	else if (header_ptr->dm_memory_free)
	{
		/* Dynamic memory is free - must not be allocated.  */
		status = -1;
	}

	if (status)
	{
		return status;
	}

	/* Mark the memory as available.  */
	header_ptr->dm_memory_free = TRUE;

	/* Adjust the available number of bytes.  */
	pool->dm_available = pool->dm_available +
		(((unsigned char*)(header_ptr->dm_next_memory)) -
		((unsigned char*)header_ptr)) - DM_OVERHEAD;

	/* Determine if the block can be merged with the previous neighbor.  */
	if ((header_ptr->dm_previous_memory)->dm_memory_free)
	{
		/* Adjust the available number of bytes.  */
		pool->dm_available = pool->dm_available + DM_OVERHEAD;

		/* Yes, merge block with previous neighbor.  */
		(header_ptr->dm_previous_memory)->dm_next_memory =
			header_ptr->dm_next_memory;
		(header_ptr->dm_next_memory)->dm_previous_memory =
			header_ptr->dm_previous_memory;

		/* Move header pointer to previous.  */
		header_ptr = header_ptr->dm_previous_memory;

		/* Adjust the search pointer to the new merged block.  */
		pool->dm_search_ptr = header_ptr;
	}

	/* Determine if the block can be merged with the next neighbor.  */
	if ((header_ptr->dm_next_memory)->dm_memory_free)
	{
		/* Adjust the available number of bytes.  */
		pool->dm_available = pool->dm_available + DM_OVERHEAD;

		/* Yes, merge block with next neighbor.  */
		new_ptr = header_ptr->dm_next_memory;
		(new_ptr->dm_next_memory)->dm_previous_memory =
			header_ptr;
		header_ptr->dm_next_memory = new_ptr->dm_next_memory;

		/* Adjust the search pointer to the new merged block.  */
		pool->dm_search_ptr = header_ptr;
	}
	
	/* Return the completion status.  */
	return(status);
}

static int  Allocate_Aligned_Memory(
	MEMORY_POOL *pool_ptr,
	void **return_pointer, 
	unsigned int size,
	unsigned int alignment)
{
	DM_PCB       *pool;                  /* Pool control block ptr    */
	DM_HEADER    *memory_ptr;            /* Pointer to memory         */
	DM_HEADER    *new_ptr;               /* New split block pointer   */
	unsigned int        free_size;              /* Size of block found       */
	int          status;                 /* Completion status         */
	unsigned int        address = 0;              /* Address of start of block */
	unsigned int        split_size = 0;           /* Bytes for front split     */
	unsigned int        next_aligned;           /* Next aligned block addr   */

	/* Move input pool pointer into internal pointer.  */
	pool = (DM_PCB *)pool_ptr;

	/* Initialize the status as successful.  */
	status = 0;

	/* Adjust the request to a size evenly divisible by the number of bytes
	in an UNSIGNED data element.  Also, check to make sure it is of the
	minimum size.  */
	if (size < pool->dm_min_allocation)
	{
		/* Change size to the minimum allocation.  */
		size = pool->dm_min_allocation;
	}
	else
	{
		/* Insure that size is a multiple of the UNSIGNED size.  */
		size = ((size + sizeof(unsigned int) - 1) / sizeof(unsigned int)) * sizeof(unsigned int);
	}

	/* Adjust the requested alignment to one evenly divisible by the number of
	bytes in an UNSIGNED data element. */
	alignment = ((alignment + sizeof(unsigned int) - 1) / sizeof(unsigned int)) * sizeof(unsigned int);

	/* Search the memory list for the first available block of memory that
	satisfies the request.  Note that blocks are merged during the
	deallocation function.  */
	memory_ptr = pool->dm_search_ptr;

	do
	{
		/* Determine if the block is free and if it can satisfy the request. */
		if (memory_ptr->dm_memory_free)
		{
			/* Calculate the free block size.  */
			free_size = (((unsigned char *)(memory_ptr->dm_next_memory)) -
				((unsigned char*)memory_ptr)) - DM_OVERHEAD;
		}
		else
		{
			/* There are no free bytes available.  */
			free_size = 0;
		}

		/* Free block may be large enough, now check alignment */
		if (free_size >= size)
		{
			/* Get address of memory block */
			address = ((unsigned int)(memory_ptr)) + DM_OVERHEAD;

			/* Is this free block, minus the overhead, already aligned? */
			if (address % alignment != 0)
			{
				/* Not aligned, can the free block be split in front? */
				next_aligned = address + (alignment - 1);
				next_aligned /= alignment;
				next_aligned *= alignment;
				split_size = next_aligned - address;

				/* Is space from start of block to aligned location large enough
				to contain 2 DM_OVERHEAD plus pool -> dm_min_allocation? */
				if (split_size < ((2 * DM_OVERHEAD) + pool->dm_min_allocation))
				{
					/* No, so try to make space for overhead and dm_min_allocation */
					next_aligned = address + (2 * DM_OVERHEAD) +
						(pool->dm_min_allocation) + (alignment - 1);
					next_aligned /= alignment;
					next_aligned *= alignment;
					split_size = next_aligned - address;
				}

				/* Adjust free_size for result of front split */
				if (free_size > split_size)
				{
					free_size -= split_size;
				}
				else
				{
					/* Can't adjust block beginning, so keep searching */
					free_size = 0;
				}
			}
		}

		/* Determine if the search should continue.  */
		if (free_size < size)
		{
			/* Large enough block has not been found.  Move the search
			pointer to the next block.  */
			memory_ptr = memory_ptr->dm_next_memory;
		}

	} while ((free_size < size) && (memory_ptr != pool->dm_search_ptr));

	/* Determine if the memory is available.  */
	if (free_size >= size)
	{
		/* A block that satisfies the request has been found.  */

		/* Is a front split required? The front split will represent the chunk
		of memory that goes from the last pointer to the aligned address. */
		if (address % alignment != 0)
		{
			/* Not aligned, front split the block, leaving an allocated block. */
			new_ptr = (DM_HEADER*)(((unsigned int)(memory_ptr)) + split_size);

			/* Mark the new block as free.  */
			new_ptr->dm_memory_free = TRUE;

			/* Put the pool pointer into the new block.  */
			new_ptr->dm_memory_pool = pool;

			/* Build the necessary pointers.  */
			new_ptr->dm_previous_memory = memory_ptr;
			new_ptr->dm_next_memory = memory_ptr->dm_next_memory;
			(new_ptr->dm_next_memory)->dm_previous_memory = new_ptr;
			memory_ptr->dm_next_memory = new_ptr;

			/* Decrement the available byte count by one DM_OVERHEAD. */
			pool->dm_available = pool->dm_available - DM_OVERHEAD;

			/* Point to new aligned free block. */
			memory_ptr = new_ptr;
		}

		/* Determine if the remaining block needs to be tail split. */
		if (free_size >= (size + DM_OVERHEAD + pool->dm_min_allocation))
		{

			/* Yes, split the block.  */
			new_ptr = (DM_HEADER *)(((unsigned char*)memory_ptr) + size + DM_OVERHEAD);

			/* Mark the new block as free.  */
			new_ptr->dm_memory_free = TRUE;

			/* Put the pool pointer into the new block.  */
			new_ptr->dm_memory_pool = pool;

			/* Build the necessary pointers.  */
			new_ptr->dm_previous_memory = memory_ptr;
			new_ptr->dm_next_memory = memory_ptr->dm_next_memory;
			(new_ptr->dm_next_memory)->dm_previous_memory = new_ptr;
			memory_ptr->dm_next_memory = new_ptr;

			/* Decrement the available byte count.  */
			pool->dm_available = pool->dm_available - size - DM_OVERHEAD;
		}
		else
		{
			/* Decrement the entire free size from the available bytes
			count.  */
			pool->dm_available = pool->dm_available - free_size;
		}

		/* Mark the allocated block as not available.  */
		memory_ptr->dm_memory_free = FALSE;

		/* Should the search pointer be moved?   */
		if (pool->dm_search_ptr == memory_ptr)
		{
			/* Move the search pointer to the next free memory slot.  */
			pool->dm_search_ptr = memory_ptr->dm_next_memory;
		}

		/* Return a memory address to the caller.  */
		*return_pointer = (void *)(((unsigned char*)memory_ptr) + DM_OVERHEAD);
	}
	else
	{
		/* Enough dynamic memory is not available. Simply return an error status.  */
		status = -1;
		*return_pointer = NULL;
	}

	/* Return the completion status.  */
	return(status);
}

static int  Memory_Pool_Information(
	MEMORY_POOL *pool_ptr,
	char *name,
	void **start_address,
	unsigned int *pool_size,
	unsigned int *min_allocation,
	unsigned int *available)
{
	DM_PCB          *pool;                  /* Pool control block ptr    */
	int             i;                      /* Working integer variable  */
	int          completion;             /* Completion status         */

	/* Move input pool pointer into internal pointer.  */
	pool = (DM_PCB *)pool_ptr;

	/* Determine if this memory pool id is valid.  */
	if ((pool != NULL) && (pool->dm_id == DM_DYNAMIC_ID))
	{
		/* The memory pool pointer is valid.  Reflect this in the completion
		status and fill in the actual information.  */
		completion = 0;

		if (name)
		{
			/* Copy the memory pool's name.  */
			for (i = 0; i < MEMPOOL_MAX_NAME; i++)
			{
				*name++ = pool->dm_name[i];
			}
		}

		/* Retrieve information directly out of the control structure.  */
		*start_address = pool->dm_start_address;
		*pool_size = pool->dm_pool_size;
		*min_allocation = pool->dm_min_allocation;
		*available = pool->dm_available;
	}
	else
	{
		/* Indicate that the memory pool pointer is invalid.   */
		completion = -1;
	}

	/* Return the appropriate completion status.  */
	return(completion);
}

void* MemPool_Init(MemBlock_T *pBlockTbl, int nBlock)
{
    MEMORY_POOL *p_MemPool = (MEMORY_POOL *)malloc(sizeof(MEMORY_POOL));
    if (p_MemPool != NULL)
    {
        memset(p_MemPool, 0, sizeof(MEMORY_POOL));
        Create_Memory_Pool(p_MemPool, "mem_pool", pBlockTbl, nBlock, 4);
    }

    return p_MemPool;
}

int MemPool_DeInit(void *pMemPool)
{
    if (pMemPool != NULL)
    {
        Delete_Memory_Pool((MEMORY_POOL *)pMemPool);
        free(pMemPool);
    }
    
    return 0;
}

void *MemPool_Malloc(void *pMemPool, unsigned int size, unsigned int alignment)
{
	void *ptr;
	int error = 0;

    MEMORY_POOL *pool_ptr = (MEMORY_POOL *)pMemPool;
    if (pool_ptr == NULL)
        return NULL;

	if(alignment == 4)
	{
        error = Allocate_Memory(pool_ptr, &ptr, size);
	}
	else
	{
        error = Allocate_Aligned_Memory(pool_ptr, &ptr, size, alignment);
	}
	
	if(error != 0)
		return NULL;

	return ptr;
}

int MemPool_Free(void *ptr)
{
	int error = 0;

	error = Deallocate_Memory(ptr);
	if(error != 0)
		return error;

	return 0;
}

void *MemPool_Calloc(void *pMemPool, unsigned int size)
{
	void *ptr;
	int error = 0;
	int i;

    MEMORY_POOL *pool_ptr = (MEMORY_POOL *)pMemPool;
    if (pool_ptr == NULL)
        return NULL;
	
    error = Allocate_Memory(pool_ptr, &ptr, size);
	if(error != 0)
		return NULL;

	for(i = 0; i < size; i++)
	{
		((char*)ptr)[i] = 0;
	}
	
	return ptr;
}

void MemPool_Memset(void *ptr, int value, int size)
{
	int i;
	
	for(i = 0; i < size; i++)
	{
		((char*)ptr)[i] = value;
	}
}

int MemPool_GetInfo(void *pMemPool, void **start_address,
	unsigned int *pool_size,
	unsigned int *min_allocation,
	unsigned int *available)
{
    MEMORY_POOL *pool_ptr = (MEMORY_POOL *)pMemPool;
    if (pool_ptr != NULL)
    {
        Memory_Pool_Information(pool_ptr, NULL, start_address, pool_size, min_allocation, available);
    }
    
    return 0;
}

