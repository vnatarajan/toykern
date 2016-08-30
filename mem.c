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
	uint32_t	magic;	/* Magic# and flag indicating in-use/free */
	int	size;		/* Size of memory region */
} mcb_t;

/* Links used by MCBs in the freelist. This info is kept in the user data
 * area of the memory block in order to keep size of MCB to minimum.
 */
typedef struct freelist_links_ {
	struct	mcb_	*larger;
	struct	mcb_	*smaller;
} freelist_links_t;

/* Minimum size of a free block (including MCB overhead) */
#define MIN_FREE_BLOCK	(sizeof(mcb_t) + sizeof(freelist_links_t))

mcb_t	*mcb;	/* Linked-list of MCBs - free and used */
/* "mcb" is a linked-list with entries in increasing order of address.
 * This list has both the free and used memory blocks. This makes it very
 * efficient to merge freed blocks into a larger sized free block.
 */

mcb_t	*endMem;	/* Address denoting end of memory */

mcb_t	*freelist;	/* Linked-list of free MCBs */
/* "freelist" is a linked-list with entries in decreasing order of
 * size of the memory blocks.
 */

/**
 * @brief
 * Get the addr of the MCB structure of the immediate next memory block.
 *
 * @param[in]
 *       m: Pointer to MCB whose next memory block MCB addr is needed.
 *
 * @param[out]
 *       None.
 *
 * @return
 *       - Success : Pointer to MCB of next memory block
 *       - Failure : NULL
 */
mcb_t *
mcbNext(mcb_t *m)
{
	mcb_t *next;

	next = (mcb_t *) ((char *) m + sizeof(*m) + m->size);
	if (next == endMem) {
		next = NULL;
	}
	return next;
}

/**
 * @brief
 * Get the address of memory area for use by user.
 *
 * @param[in]
 *       m: Pointer to MCB whose usable memory address is needed.
 *
 * @param[out]
 *       None.
 *
 * @return
 *       - Pointer to memory address for user's use.
 */
void *
mcbAddr(mcb_t *m)
{
	return (void *) ((char *) m + sizeof(*m));
}

/**
 * @brief
 * Insert an MCB into the sorted freelist.
 *
 * @note
 * A very simple O(n) sort method is used here. A more runtime efficient
 * one would be 'skiplist'. However, that would result in a bloated mcb_t.
 * A bloated mcb_t would result in poor memory management efficiency (ie.
 * how much of the memory space provided to us for managing is actually
 * available to users of our memory management code.)
 *
 * @param[in]
 *       m: MCB to be inserted into freelist.
 *
 * @param[out]
 *       None.
 *
 * @return
 *       - None.
 */
static void
insertFree(mcb_t *m)
{
	mcb_t *l, *s;
	freelist_links_t *mf, *lf, *sf;

	/* Find where to insert 'm'. */
	l = NULL;
	s = freelist;
	while (s && (m->size < s->size)) {
		l = s;
		sf = mcbAddr(s);
		s = sf->smaller;
	}
	/* Insert 'm' between 'l' and 's'. */
	mf = mcbAddr(m);
	mf->larger = l;
	if (l) {
		lf = mcbAddr(l);
		lf->smaller = m;
	} else {
		freelist = m;
	}
	mf->smaller = s;
	if (s) {
		sf = mcbAddr(s);
		sf->larger = m;
	}
	return;
}

/**
 * @brief
 * Remove a MCB from the freelist.
 *
 * @param[in]
 *       m: The MCB to be removed from freelist.
 *
 * @param[out]
 *       None.
 *
 * @return
 *       - None.
 */
static void
removeFree(mcb_t *m)
{
	freelist_links_t *mf, *f;

	mf = mcbAddr(m);
	if (mf->smaller) {
		f = mcbAddr(mf->smaller);
		f->larger = mf->larger;
	}
	if (mf->larger) {
		f = mcbAddr(mf->larger);
		f->smaller = mf->smaller;
	} else {
		freelist = mf->smaller;
	}
	mf->smaller = mf->larger = NULL;
	return;
}

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
	mcb_t *m, *next;
	freelist_links_t *mf, *f;

	m = mcb;
	while (m) {
		mf = mcbAddr(m);
		/* MCB must have a valid magic#. */
		if ((m->magic != MAGIC_USED) && (m->magic != MAGIC_FREE)) {
			assert(0);
		}
		/* First element will have 'prev' as NULL. */
		if ((m->prev == NULL) && (mcb != m)) {
			assert(0);
		}
		/* Address in successive MCBs must be increasing. */
		next = mcbNext(m);
		if (next && (next <= m)) {
			assert(0);
		}
		/* Check if linked-list prev/next are sane. */
		if (m->prev) {
			if (mcbNext(m->prev) != m) {
				assert(0);
			}
		} else {
			if (mcb != m) {
				assert(0);
			}
		}
		if (next) {
			if (next->prev != m) {
				assert(0);
			}
		}
		if (m->magic == MAGIC_FREE) {
			/* If no MCB is larger than this one, it must be at
			 * head of freelist.
			 */
			if (!mf->larger && (freelist != m)) {
				assert(0);
			}
			if (mf->larger) {
				if (mf->larger->magic != MAGIC_FREE) {
					assert(0);
				}
			}
			if (mf->smaller) {
				if (mf->smaller->magic != MAGIC_FREE) {
					assert(0);
				}
			}
			/* The larger MCB must have a larger (or same) size */
			if (mf->larger && (mf->larger->size < m->size)) {
				assert(0);
			}
			/* The smaller MCB must have smaller (or same) size */
			if (mf->smaller && (mf->smaller->size > m->size)) {
				assert(0);
			}
			/* The must not be 2 contiguous free memory blocks. */
			if (m->prev && (m->prev->magic != MAGIC_USED)) {
				assert(0);
			}
			if (next && (next->magic != MAGIC_USED)) {
				assert(0);
			}
		}
		m = next;
	}

	m = freelist;
	while (m) {
		mf = mcbAddr(m);
		if (m->magic != MAGIC_FREE) {
			assert(0);
		}
		if (mf->smaller) {
			if (mf->smaller->magic != MAGIC_FREE) {
				assert(0);
			}
			if (m->size < mf->smaller->size) {
				assert(0);
			}
			f = mcbAddr(mf->smaller);
			if (f->larger != m) {
				assert(0);
			}
		}
		if (mf->larger) {
			f = mcbAddr(mf->larger);
			if (f->smaller != m) {
				assert(0);
			}
		} else {
			if (freelist != m) {
				assert(0);
			}
		}
		m = mf->smaller;
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
	m->size = size - sizeof(mcb_t);
	m->magic = MAGIC_FREE;
	m->prev = NULL;
	mcb = m;
	endMem = (mcb_t *) ((char *) addr + size);
	freelist = NULL;
	insertFree(m);
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
 * We use the worst-fit method wherein the allocation is done
 * from the memory block of largest size. For better performance
 * we maintain a sorted linked-list in decreasing size of memory
 * blocks.
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
	mcb_t	*m, *n, *next;
	freelist_links_t *nf;
	int	balance;

	/* Any memory block must be able to hold the links needed for
	 * memory block in freelist.
	 */
	if (size < sizeof(freelist_links_t)) {
		size = sizeof(freelist_links_t);
	}
	/* Align size to size of integer */
	size = (size + sizeof(int) - 1) & ~(sizeof(int) - 1);

	m = freelist;
	if (!m || m->size < size) {
		return NULL;
	}

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
		n = (mcb_t *) ((char *) mcbAddr(m) + size);
		n->prev = m;
		next = mcbNext(m);
		if (next) {
			next->prev = n;
		}
		n->magic = MAGIC_FREE;
		n->size = balance - sizeof(*m);
		nf = mcbAddr(n);
		nf->smaller = nf->larger = NULL;
		insertFree(n);
	} else {
		/* Allocate this whole block. */
		size = size + balance;
	}

	removeFree(m);

	/* Mark current block as in use. */
	m->magic = MAGIC_USED;
	m->size = size; /* Set to size allocated */
#ifdef UNIT_TEST
	sanityCheck();
#endif /* UNIT_TEST */
	return (mcbAddr(m));
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
	mcb_t	*m, *next, *nnext;
	freelist_links_t *mf;

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
		mf = mcbAddr(m);
		mf->smaller = mf->larger = NULL;

		/* Merge with preceeding block, if possible */
		if (m->prev && (m->prev->magic == MAGIC_FREE)) {
			m->magic = 0;
			m->prev->size += m->size + sizeof(*m);
			next = mcbNext(m);
			if (next) {
				next->prev = m->prev;
			}
			m = m->prev;
			/* Since size of 'm' is increased, put it back
			 * into freelist in sorted order.
			 */
			removeFree(m);
			insertFree(m);
		} else {
			insertFree(m);
		}

		/* Merge with succeeding block, if possible */
		next = mcbNext(m);
		if (next && (next->magic == MAGIC_FREE)) {
			removeFree(next);
			next->magic = 0;
			m->size += sizeof(*m) + next->size;
			nnext = mcbNext(next);
			if (nnext) {
				nnext->prev = m;
			}
			/* Since size of 'm' is increased, put it back
			 * into freelist in sorted order.
			 */
			removeFree(m);
			insertFree(m);
		}
	}
#ifdef UNIT_TEST
	sanityCheck();
#endif /* UNIT_TEST */
	return;
}
