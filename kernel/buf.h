struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  uint dev;
  uint blockno;
  uint utime;
  struct sleeplock lock;
  uint refcnt;
  uchar data[BSIZE];
};

