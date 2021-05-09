// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
} bcache[NBUCKET];

void
binit(void)
{ 
  //struct buf * b;

  for(int i = 0 ;i < NBUCKET; i++){
    char lockname[15];
    snprintf(lockname,14,"bcache_%d",i);
    initlock(&bcache[i].lock,lockname);
    for(int j = 0; j < NBUF;j++){
      initsleeplock(&bcache[i].buf[j].lock, "buffer");
      bcache[i].buf[j].utime = 0;
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int bucket = (blockno + dev) % NBUCKET;

  acquire(&bcache[bucket].lock);
  // Is the block already cached?
  for(b = bcache[bucket].buf; b != bcache[bucket].buf + NBUF; b++){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache[bucket].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  struct buf *candi = 0;
  uint ltime = 0;
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache[bucket].buf; b != bcache[bucket].buf + NBUF; b++){
    if(b->refcnt == 0 && candi == 0) {
      candi = b;
      ltime = candi->utime;
    }else if(b -> refcnt == 0 && b->utime < ltime){
      candi = b;
      ltime = candi->utime;
    }
  }
  if(candi){
    candi->dev = dev;
    candi->refcnt = 1;
    candi->blockno = blockno;
    candi->valid = 0;
    acquire(&tickslock);
    candi->utime = ticks;
    release(&tickslock);
    b = candi;
    release(&bcache[bucket].lock);
    acquiresleep(&b->lock);
    return b;
  }


  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int bucket = (b->dev + b->blockno)%NBUCKET;
  acquire(&bcache[bucket].lock);
  b->refcnt--;  
  release(&bcache[bucket].lock);
}

void
bpin(struct buf *b) {
  int bucket = (b->dev + b->blockno)%NBUCKET;
  acquire(&bcache[bucket].lock);
  b->refcnt++;
  release(&bcache[bucket].lock);
}

void
bunpin(struct buf *b) {
  int bucket = (b->dev + b->blockno)%NBUCKET;
  acquire(&bcache[bucket].lock);
  b->refcnt--;
  release(&bcache[bucket].lock);
}


