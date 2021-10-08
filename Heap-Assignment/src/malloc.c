#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

/*

   Name: 
   ID:

 */

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)      ((b) + 1)
#define BLOCK_HEADER(ptr)   ((struct _block *)(ptr) - 1)

static int atexit_registered = 0;
static int num_mallocs = 0;
static int num_frees = 0;
static int num_reuses = 0;
static int num_grows = 0;
static int num_blocks = 0;
static int num_requested = 0;
static int max_heap = 0;

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics( void )
{
  printf("\nheap management statistics\n");
  printf("mallocs:\t%d\n", num_mallocs );
  printf("frees:\t\t%d\n", num_frees );
  printf("reuses:\t\t%d\n", num_reuses );
  printf("grows:\t\t%d\n", num_grows );
  printf("blocks:\t\t%d\n", num_blocks );
  printf("requested:\t%d\n", num_requested );
  printf("max heap:\t%d\n", max_heap );
  printf("-----------------\n\n");
}


struct _block
{
  size_t size;           /* Size of the allocated _block of memory in bytes */
  struct _block *prev;   /* Pointer to the previous _block of allcated memory   */
  struct _block *next;   /* Pointer to the next _block of allcated memory   */
  bool free;             /* Is this _block free?                     */
  char padding[3];
};


struct _block *heapList = NULL; /* Free list to track the _blocks available */
struct _block *nextFit = NULL;

/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 * \TODO Implement Next Fit
 * \TODO Implement Best Fit
 * \TODO Implement Worst Fit
 */
struct _block *findFreeBlock(struct _block **last, size_t size)
{
  struct _block *curr = heapList;
  
#if defined FIT && FIT == 0
  /* First fit */
  while (curr && !(curr->free && curr->size >= size))
  {
    *last = curr;
    curr = curr->next;
  }
  
#endif
  
#if defined BEST && BEST == 0
  struct _block *min = NULL;
  size_t diff = INT_MAX;
  while(curr)
  {
    *last = curr;
    if(curr->free && curr->size >= size && diff > (int)(curr->size - size))
    {
      min = curr;
      diff = (int)(curr->size - size);
    }
    curr = curr->next;
  }
  curr = min;
#endif
  
#if defined WORST && WORST == 0
  struct _block *max = NULL;
  size_t diff = 0;
  while(curr)
  {
    *last = curr;
    if(curr->free && curr->size >= size && diff < (int)(curr->size - size))
    {
      max = curr;
      diff = (int)(curr->size - size);
    }
    curr = curr->next;
  }
  curr = max;
#endif
  
#if defined NEXT && NEXT == 0
  
  
  while (nextFit && !(nextFit->free && nextFit->size >= size))
  {
    *last = nextFit;
    nextFit = nextFit->next;
  }
  if(nextFit == NULL)
    nextFit = heapList;
  while (nextFit && !(nextFit->free && nextFit->size >= size))
  {
    *last = nextFit;
    nextFit = nextFit->next;
  }
  curr = nextFit;
  
#endif
  
  if (curr != NULL)
    num_reuses++;
  else
  {
    num_grows++;
    max_heap += size;
  }
  return curr;
}



/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size)
{
  /* Request more space from OS */
  struct _block *curr = (struct _block *)sbrk(0);
  struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);
  
  assert(curr == prev);
  
  /* OS allocation failed */
  if (curr == (struct _block *)-1)
  {
    return NULL;
  }
  
  /* Update heapList if not set */
  if (heapList == NULL)
  {
    heapList = curr;
  }
  
  /* Attach new _block to prev _block */
  if (last)
  {
    last->next = curr;
  }
  
  /* Update _block metadata */
  curr->size = size;
  curr->next = NULL;
  curr->free = false;
  return curr;
}


void *calloc(size_t nmemb, size_t size)
{
  if(atexit_registered == 0)
  {
    atexit_registered = 1;
    atexit( printStatistics );
  }
  void *ptr;
  ptr = malloc (nmemb * size);
  if (ptr == NULL)
    return (ptr);
  memset(ptr, 0, nmemb * size);
  return (ptr);
}


void *realloc(void *ptr, size_t size)
{
  if(atexit_registered == 0)
  {
    atexit_registered = 1;
    atexit( printStatistics );
  }
  
  if(size == 0)
  {
    free(ptr);
    return NULL;
  }
  if(ptr == NULL)
    return calloc(1, size);
  
  struct _block *old = BLOCK_HEADER(ptr);
  void *new = calloc(1, size);
  memcpy(new, ptr, old->size);
  
  free(ptr);
  return new;
  
}

/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process
 * or NULL if failed
 */
void *malloc(size_t size)
{
  num_mallocs++;
  num_requested = num_requested + size;
  
  if(atexit_registered == 0)
  {
    atexit_registered = 1;
    atexit( printStatistics );
  }
  
  /* Align to multiple of 4 */
  size = ALIGN4(size);
  
  /* Handle 0 size */
  if (size == 0)
  {
    return NULL;
  }
  
  /* Look for free _block */
  struct _block *last = heapList;
  struct _block *next = findFreeBlock(&last, size);
  
  /* TODO: Split free _block if possible */
  
  /* Could not find free _block, so grow heap */
  if (next == NULL)
  {
    num_blocks++;
    next = growHeap(last, size);
  }
  
  /* Could not find free _block or grow heap, so just return NULL */
  if (next == NULL)
  {
    return NULL;
  }
  
  /* Mark _block as in use */
  next->free = false;
  
  /* Return data address associated with _block */
  return BLOCK_DATA(next);
}

/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr)
{
  num_frees++;
  if (ptr == NULL)
  {
    return;
  }
  
  /* Make _block as free */
  struct _block *curr = BLOCK_HEADER(ptr);
  assert(curr->free == 0);
  curr->free = true;
  
  /* TODO: Coalesce free _blocks if needed */
}

/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
