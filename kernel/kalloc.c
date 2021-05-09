// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.
struct spinlock steallock;

void
kinit()
{ 

  //uint64 start = PGROUNDUP((uint64)end);
  //uint64 list_size = ((uint64)PHYSTOP - start) / NCPU;
  initlock(&steallock,"kmem");

  for(int i = 0; i < NCPU;i++){
    char lockname[10];
    snprintf(lockname,9,"kmem_%d",i);
    initlock(&cpus[i].kmem.lock, lockname);
    cpus[i].kmem.freelist = 0;
  }
  freerange((void *)end, (void *)PHYSTOP);
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

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();
  struct cpu *tcpu = mycpu();
  pop_off();

  acquire(&tcpu->kmem.lock);
  r->next = tcpu->kmem.freelist;
  tcpu->kmem.freelist = r;
  release(&tcpu->kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();
  struct cpu *tcpu = mycpu();
  int id = cpuid();
  pop_off();

  acquire(&tcpu->kmem.lock);
  r = tcpu->kmem.freelist;

  if(r)
    tcpu->kmem.freelist = r->next;
  else{
    acquire(&steallock);
    for(int i = 0; i < NCPU;i++){
      if(i != id){
        acquire(&cpus[i].kmem.lock);
        if(cpus[i].kmem.freelist){
          r = cpus[i].kmem.freelist;
          cpus[i].kmem.freelist = r->next;
        }
        release(&cpus[i].kmem.lock);
        if(r){
          break;
        }
      }
    }
    release(&steallock);
  }
  release(&tcpu->kmem.lock);
  
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  
  return (void*)r;
}
