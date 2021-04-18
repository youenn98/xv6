// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.
struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct 
{
  struct spinlock lock;
  int pacnt[(PHYSTOP-KERNBASE)/PGSIZE];
}kref;

void init_kref(){
  acquire(&kref.lock);
  memset(kref.pacnt, 0, sizeof(kref.pacnt));
  release(&kref.lock);
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&kref.lock,"kref");
  init_kref();
  freerange(end, (void*)PHYSTOP);
}

//Get a phsical address's reference count key
int ref_key(uint64 pa){
  return (PGROUNDDOWN(pa) - KERNBASE)/PGSIZE;
}

void incre_ref(void * pa){
  int key = ref_key((uint64)pa);
  acquire(&kref.lock);
  kref.pacnt[key]++;
  release(&kref.lock);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
  
  int do_realse = 1;
  acquire(&kref.lock);
  int key = ref_key((uint64)pa);
  if(kref.pacnt[key] > 1){
    do_realse = 0;
  }
  kref.pacnt[key]--;
  if(kref.pacnt[key] < 0) kref.pacnt[key] = 0;
  release(&kref.lock);

  if(!do_realse) return;


  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
    
  if(r){
    acquire(&kref.lock);
    int key = ref_key((uint64)r);
    kref.pacnt[key] = 1;
    release(&kref.lock);
  }
  
  return (void*)r;
}
