/**
 * @file      mem.c
 * @brief     Memory management for toy kernel.
 *
 * A simple memory management code to illustrate basic memory
 * management related kernel APIs.
 *
 * @author    Natarajan Venkataraman, mr.v.natarajan@gmail.com
 * @copyright Copyright (c) 2016, Natarajan Venkataraman
 */

#include <mem.h>
#include <stdint.h>
#include <stdlib.h>
#ifdef UNIT_TEST
#include <assert.h>
#endif /* UNIT_TEST */

#define	TRUE	1
#define	FALSE	0

/* Some magic numbers we will use. Also indicates state of a memory block. */
#define MAGIC_USED	0x4D454D55 /* 'MEMU' */
#define MAGIC_FREE	0x4D454D46 /* 'MEMF' */

/* Memory control block (MCB) */
typedef struct mcb_ {
	struct	mcb_	*prev;	/* Preceeding memory block - may be free or
				 * in-use.
				 */
	struct	mcb_	*next;	/* Succeeding memory block - may be free or
				 * in-use.
				 */
	uint32_t	magic;	/* Magic# and also flag indicating in-use or free */
	int	size;		/* Size of memory region */
	void	*addr;		/* Start address of memory given to user */
} mcb_t;

/* Minimum size of a free block (including MCB overhead) */
#define MIN_FREE_BLOCK	(sizeof(mcb_t) + 16)

mcb_t	*mcb;	/* Linked-list of MCBs - free and used */
/* "mcb" is a linked-list with entries in increasing order of address.
 * This list has both the free and used memory blocks. This makes it very
 * efficient to merge freed blocks into a larger sized free block.
 */

#ifdef UNIT_TEST
/**
 * @brief
 * Do sanity test of the data-strs used by this memory management module.
 *
 * @param[in]
 *       None.
 *
 * @param[out]
 *       None.
 *
 * @return
 *       - None: on success
 *       - Assert fail: on failure
 */
static void
sanityCheck(void)
{
	mcb_t *m;

	m = mcb;
	while (m) {
		/* MCB must have a valid magic#. */
		if ((m->magic != MAGIC_USED) && (m->magic != MAGIC_FREE)) {
			assert(0);
		}
		/* First element will have 'prev' as NULL. */
		if ((m->prev == NULL) && (mcb != m)) {
			assert(0);
		}
		/* Address in successive MCBs must be increasing. */
		if (m->next && (m->next->addr <= m->addr)) {
			assert(0);
		}
		/* Check if linked-list prev/next are sane. */
		if (m->prev) {
			if (m->prev->next != m) {
				assert(0);
			}
		} else {
			if (mcb != m) {
				assert(0);
			}
		}
		if (m->next) {
			if (m->next->prev != m) {
				assert(0);
			}
		}
		/* There must not be 2 contiguous free memory blocks. */
		if (m->magic == MAGIC_FREE) {
			if (m->prev && m->prev->magic != MAGIC_USED) {
				assert(0);
			}
			if (m->next && m->next->magic != MAGIC_USED) {
				assert(0);
			}
		}
		m = m->next;
	}
	return;
}
#endif /* UNIT_TEST */

/**
 * @brief
 * Initialize a region of memory that needs to be managed.
 *
 * @note
 * This function MUST be called before memAlloc() and memFree()
 * API functions are invoked.
 *
 * @param[in]
 *       addr: Start address of region of memory to be managed.
 *       size: Size of region of memory to be managed.
 *
 * @param[out]
 *       None
 *
 * @return
 *       - None
 */
void
memInit(void *addr, int size)
{
	mcb_t	*m;


	/* Mark entire region as free. */
	m = (mcb_t *) addr;
	m->addr = (char *) m + sizeof(*m);
	m->size = size - sizeof(mcb_t);
	m->magic = MAGIC_FREE;
	m->prev = NULL;
	m->next = NULL;
	mcb = m;
#ifdef UNIT_TEST
	sanityCheck();
#endif /* UNIT_TEST */
	return;
}

/**
 * @brief
 * API to allocate memory.
 *
 * @note
 * The algorithm used is a link-list traversal, hence, quite
 * low performing.
 *
 * @param[in]
 *       size: Number of bytes of memory to be allocated.
 *
 * @param[out]
 *       None.
 *
 * @return
 *       - On successful allocation, pointer to start of memory
 *         area which has at least 'size' bytes of memory.
 *       - On failure, NULL is returned.
 */
void *
memAlloc(int size)
{
	mcb_t	*m, *n;
	void	*a = NULL;
	int	balance;

	/* Align size to size of integer */
	size = (size + sizeof(int) - 1) & ~(sizeof(int) - 1);

	m = mcb;
	do {
		if ((m->magic == MAGIC_FREE) && (size < m->size)) {
			/* This memory block is free and has required space
			 * to allocate for this memory allocation request.
			 * Split this block into a used and a free block.
			 */
			balance = m->size - size;

			/* New free block must be at least a certain
			 * minimum size.
			 */
			if (balance > MIN_FREE_BLOCK) {
				/* Create a new free block of smaller size */
				n = (mcb_t *) ((char *) m->addr + size);
				n->prev = m;
				n->next = m->next;
				if (n->next) {
					n->next->prev = n;
				}
				n->magic = MAGIC_FREE;
				n->addr = (char *) n + sizeof(*n);
				n->size = balance - sizeof(*m);
			} else {
				/* Allocate this whole block. */
				n = m->next;
				size = size + balance;
			}

			/* Mark current block as in use. */
			m->magic = MAGIC_USED;
			m->next = n;
			m->size = size; /* Set to size allocated */
			a = m->addr;
			break;
		}
		m = m->next;
	} while(m);
#ifdef UNIT_TEST
	sanityCheck();
#endif /* UNIT_TEST */
	return (a);
}

/**
 * @brief
 * API to free memory.
 *
 * @note
 * Since the memory block is contiguous with the memory allocated,
 * the algorithm to free is quite efficient.
 *
 * @param[in]
 *       addr: Start address of memory to be freed back.
 *             The 'addr' must be same as what was returned by
 *             memAlloc().
 *
 * @param[out]
 *       None.
 *
 * @return
 *       - None.
 */
void
memFree(void *addr)
{
	mcb_t	*m;

	if (!addr) return;

	/* We expect MCB to be just above the addr.
	 * If MCB is not present it means a wrong address has been
	 * passed for freeing.
	 */
	m = (mcb_t *) (addr - sizeof(*m));
	if (m->magic != MAGIC_USED) {
		/* Sanity failed! */
		return;
	}

	if (m) {
		/* Mark block as free */
		m->magic = MAGIC_FREE;

		/* Merge with preceeding block, if possible */
		if (m->prev && (m->prev->magic == MAGIC_FREE)) {
			m->magic = 0;
			m->prev->size += m->size + sizeof(*m);
			m->prev->next = m->next;
			if (m->next) {
				m->next->prev = m->prev;
			}
			m = m->prev;
		}

		/* Merge with succeeding block, if possible */
		if (m->next && (m->next->magic == MAGIC_FREE)) {
			m->next->magic = 0;
			m->size += sizeof(*m) + m->next->size;
			if (m->next->next) {
				m->next->next->prev = m;
			}
			m->next = m->next->next;
		}
	}
#ifdef UNIT_TEST
	sanityCheck();
#endif /* UNIT_TEST */
	return;
}
