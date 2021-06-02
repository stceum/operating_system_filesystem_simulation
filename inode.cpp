#include "inode.h"

#include "fs.h"

/* 用来存储inode位置 */
typedef struct inode_position
{
  bool two_blk;      // inode是否跨块
  uint32_t lba;      // inode所在的块号
  uint32_t off_size; // inode在块内的 字节 偏移量
} inode_position;

/* 获取inode所在的块和块内的偏移量 */
static void
inode_locate(current_partition* cp,
             uint32_t inode_no,
             inode_position* inode_pos)
{
  /* inode_table在硬盘上是连续的 */
  if (inode_no < 4096) {
    return;
  }
  uint32_t inode_table_lba = cp->sb->inode_table_lba;

  uint32_t inode_size = sizeof(struct inode);
  uint32_t off_size =
    inode_no * inode_size; // 第inode_no号I结点相对于inode_table_lba的字节偏移量
  uint32_t off_sec =
    off_size / BLOCK_SIZE; // 第inode_no号I结点相对于inode_table_lba的扇区偏移量
  uint32_t off_size_in_blk =
    off_size % BLOCK_SIZE; // 待查找的inode所在扇区中的起始地址

  /* 判断此i结点是否跨越2个扇区 */
  uint32_t left_in_sec = 512 - off_size_in_blk;
  if (
    left_in_sec <
    inode_size) { // 若扇区内剩下的空间不足以容纳一个inode,必然是I结点跨越了2个扇区
    inode_pos->two_blk = true;
  } else { // 否则,所查找的inode未跨扇区
    inode_pos->two_blk = false;
  }
  inode_pos->lba = inode_table_lba + off_sec;
  inode_pos->off_size = off_size_in_blk;
}

/* 将inode写入到分区part */
void
inode_sync(current_partition* cp, struct inode* inode, void* io_buf)
{ // io_buf是用于硬盘io的缓冲区
  uint8_t inode_no = inode->i_no;
  inode_position inode_pos;
  inode_locate(cp, inode_no, &inode_pos); // inode位置信息会存入inode_pos
  if (inode_pos.lba <=
      (cp->v_disk->partitions[cp->partition_no].start_sector_no +
       cp->v_disk->partitions[cp->partition_no].sector_count))
    return;

  /* 硬盘中的inode中的成员inode_tag和i_open_cnts是不需要的,
   * 它们只在内存中记录链表位置和被多少进程共享 */
  struct inode pure_inode;
  memcpy(&pure_inode, inode, sizeof(struct inode));

  /* 以下inode的三个成员只存在于内存中,现在将inode同步到硬盘,清掉这三项即可 */
  pure_inode.i_open_counts = 0;
  pure_inode.write_deny = false; // 置为false,以保证在硬盘中读出时为可写
  pure_inode.inode_tag.prev = pure_inode.inode_tag.next = NULL;

  char* inode_buf = (char*)io_buf;
  if (inode_pos.two_blk) { // 若是跨了两个扇区,就要读出两个扇区再写入两个扇区
    /* 读写硬盘是以扇区为单位,若写入的数据小于一扇区,要将原硬盘上的内容先读出来再和新数据拼成一扇区后再写入
     */
    char* tmp = read_blocks_from_disk(cp->v_disk, inode_pos.lba, 2);
    memcpy(inode_buf, tmp, sizeof(tmp));
    free(tmp);

    /* 开始将待写入的inode拼入到这2个扇区中的相应位置 */
    memcpy((inode_buf + inode_pos.off_size), &pure_inode, sizeof(struct inode));

    /* 将拼接好的数据再写入磁盘 */
    write_blocks_to_disk(cp->v_disk, inode_pos.lba, inode_buf, 2);
  } else { // 若只是一个扇区
    char* tmp = read_blocks_from_disk(cp->v_disk, inode_pos.lba, 1);
    memcpy(inode_buf, tmp, sizeof(tmp));
    free(tmp);
    memcpy((inode_buf + inode_pos.off_size), &pure_inode, sizeof(struct inode));
    write_blocks_to_disk(cp->v_disk, inode_pos.lba, inode_buf, 1);
  }
}