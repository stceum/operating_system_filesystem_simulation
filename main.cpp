#include <iostream>

#include "fs.h"

int main() {
  // create_disk((char *)"test_disk", 512 * (1048576));
  // int sizes[2] = {268173304, 268173304};
  // char *names[8] = {(char *)"p1", (char *)"p2"};
  // create_partitions((char *)"test_disk", 2, sizes, names);

  virtual_disk test_d = read_disk((char *)"test_disk");
  fs_init(&test_d, 0);
  // uint32_t fd = sys_open("/file1", O_CREAT);
  // uint32_t fd = sys_open("/file1", O_RDWR);

  // //   write
  // sys_write(fd, "Hello world\n", 12);
  // sys_close(fd);
  // std::cout << fd << " closed!" << std::endl;

  // read
  // char test[64];
  // memset(test, 0, 64);
  // sys_read(fd, test, 64);
  // std::cout << test << std::endl;

  // lseek
  // uint32_t fd = sys_open("/file1", O_RDWR);
  // printf("open /file1, fd:%d\n", fd);
  // char buf[64] = {0};
  // int read_bytes = sys_read(fd, buf, 18);
  // printf("1_ read %d bytes:\n%s\n", read_bytes, buf);

  // memset(buf, 0, 64);
  // read_bytes = sys_read(fd, buf, 6);
  // printf("2_ read %d bytes:\n%s", read_bytes, buf);

  // memset(buf, 0, 64);
  // read_bytes = sys_read(fd, buf, 6);
  // printf("3_ read %d bytes:\n%s", read_bytes, buf);

  // printf("________ SEEK_SET 0 ________\n");
  // sys_lseek(fd, 0, F_SEEK_SET);
  // memset(buf, 0, 64);
  // read_bytes = sys_read(fd, buf, 24);
  // printf("4_ read %d bytes:\n%s", read_bytes, buf);

  // sys_close(fd);

  // delete
  // std::cout << "/file1 delete " << (sys_unlink("/file1") == 0 ? "done" : "fail")
  //           << "!\n";
  // std::cout << "/dir1/subdir1 create "
  //           << (sys_mkdir("/dir1/subdir1") == 0 ? "done" : "fail") << "!\n";
  // std::cout << "/dir1 create " << (sys_mkdir("/dir1") == 0 ? "done" : "fail")
  //           << "!\n";
  // std::cout << "now, /dir1/subdir1 create "
  //           << (sys_mkdir("/dir1/subdir1") == 0 ? "done" : "fail") << "!\n";
  // std::cout << "now, /dir1/subdir1 create "
  //           << (sys_mkdir("/dir1/subdir1") == 0 ? "done" : "fail") << "!\n";
  // int fd = sys_open("/dir1/subdir1/file2", O_CREAT | O_RDWR);
  // if (fd != -1) {
  //   std::cout << "/dir1/subdir1/file2 create done!\n";
  //   sys_write(fd, "Catch me if you can!\n", 21);
  //   sys_lseek(fd, 0, F_SEEK_SET);
  //   char buf[32] = {0};
  //   sys_read(fd, buf, 21);
  //   std::cout << "/dir1/subdir1/file2 says:\n" << buf;
  //   sys_close(fd);
  // }
  return 0;
}