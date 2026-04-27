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

extern uint ticks;

// 质数个哈希桶
#define NBUCKET 13
// 哈希函数
int hash(uint blockno)
{
  return blockno % NBUCKET;
}

struct {
  struct spinlock lock[NBUCKET]; // 每个桶一把锁
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBUCKET]; // 每个桶的表头
  struct spinlock eviction_lock; // 用于串行化回收的全局锁
} bcache;

void
binit(void)
{
  struct buf *b;

  // 初始化全局琐
  initlock(&bcache.eviction_lock, "bcache_eviction");
  // 初始化每个桶的琐和表头
  for(int i = 0; i < NBUCKET; i++)
  {
    initlock(&bcache.lock[i], "bcache_bucket");
    // Create linked list of buffers
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }
  // 所有buf初始挂载到0号
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head[0].next;
    b->prev = &bcache.head[0];
    initsleeplock(&b->lock, "buffer");
    bcache.head[0].next->prev = b;
    bcache.head[0].next = b;
    b->timestamp = 0;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  
  int id = hash(blockno); // 代表当前想要读取的磁盘块应该被放在哪一个桶里

  acquire(&bcache.lock[id]);

  // Is the block already cached?
  for(b = bcache.head[id].next; b != &bcache.head[id]; b = b->next){ // 从前往后找
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[id]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.lock[id]);

  acquire(&bcache.eviction_lock); // 串行化：当缓存中的查找未命中时，它选择要复用的缓冲区
  acquire(&bcache.lock[id]);
  for(b = bcache.head[id].next; b != &bcache.head[id]; b = b->next){ // 从前往后找
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[id]);
      release(&bcache.eviction_lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.lock[id]);


  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // for(b = bcache.head[id].prev; b != &bcache.head[id]; b = b->prev){ // 从后往前找
  //   if(b->refcnt == 0) {
  //     b->dev = dev;
  //     b->blockno = blockno;
  //     b->valid = 0;
  //     b->refcnt = 1;
  //     release(&bcache.lock[id]);
  //     acquiresleep(&b->lock[id]);
  //     return b;
  //   }
  // }

  // 通过时间戳来实现LRU
  struct buf *victim = 0;
  uint min_ticks = -1;
  int victim_bucket = -1; // 记录最近最久未使用的缓存块原来所在的桶
  for(int i = 0; i < NBUCKET; i++)
  {
    acquire(&bcache.lock[i]);
    for(b = bcache.head[i].next; b != &bcache.head[i]; b = b->next)
    {
      if(b->refcnt == 0 && b->timestamp < min_ticks)
      {
        min_ticks = b->timestamp;
        victim = b;
        victim_bucket = i;
      }
    }
    release(&bcache.lock[i]);
  }
  if(victim == 0) panic("bget: no buffers");

  // 按序号大小顺序获取两个桶的锁，避免互相死锁
  if(victim_bucket != id)
  {
    if(victim_bucket < id)
    {
      acquire(&bcache.lock[victim_bucket]); // 先拿小的
      acquire(&bcache.lock[id]);            // 再拿大的
    } 
    else
    {
      acquire(&bcache.lock[id]);
      acquire(&bcache.lock[victim_bucket]);
    }
  }
  else
  {
    acquire(&bcache.lock[id]);
  }

  victim->dev = dev;
  victim->blockno = blockno;
  victim->valid = 0;
  victim->refcnt = 1;

  if(victim_bucket != id)
  {
    // 从旧桶摘除
    victim->next->prev = victim->prev;
    victim->prev->next = victim->next;

    // 插入新桶(id)的头部
    victim->next = bcache.head[id].next;
    victim->prev = &bcache.head[id];
    bcache.head[id].next->prev = victim;
    bcache.head[id].next = victim;
  }
  // 释放锁
  if(victim_bucket != id) 
  {
    release(&bcache.lock[victim_bucket]);
  }

  release(&bcache.lock[id]);
  release(&bcache.eviction_lock);

  acquiresleep(&victim->lock);
  return victim;
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

  int id = hash(b->blockno);
  acquire(&bcache.lock[id]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    // b->next->prev = b->prev;
    // b->prev->next = b->next;
    // b->next = bcache.head.next;
    // b->prev = &bcache.head;
    // bcache.head.next->prev = b;
    // bcache.head.next = b;
    b->timestamp = ticks; // 改为标记上次使用时间的时间戳缓冲区（即使用kernel/trap.c中的ticks）
  }
  
  release(&bcache.lock[id]);
}

void
bpin(struct buf *b) {
  int id = hash(b->blockno);
  acquire(&bcache.lock[id]);
  b->refcnt++;
  release(&bcache.lock[id]);
}

void
bunpin(struct buf *b) {
  int id = hash(b->blockno);
  acquire(&bcache.lock[id]);
  b->refcnt--;
  release(&bcache.lock[id]);
}


