#include <iostream>

#include "dir.h"
#include "fs.h"
#include "sim_shell.h"
#include "pcb.h"

int main() {
  // create_disk((char *)"test_disk", 512 * (1048576));
  // int sizes[2] = {268173304, 268173304};
  // char *names[8] = {(char *)"p1", (char *)"p2"};
  // create_partitions((char *)"test_disk", 2, sizes, names);

  // virtual_disk test_d = read_disk((char*)"test_disk");
  // fs_init(&test_d, 0);
  // uint32_t fd = sys_open("/file1", O_CREAT);
  // uint32_t fd = sys_open("/file1", O_RDWR);
  std::string a;
  virtual_disk v_disk = read_disk((char*)"test_disk");
  fs_init(&v_disk, 0);
  char cur_user[64];
  strcpy(cur_user, "root");
  sys_getcwd(cwd_buf, 32);
  sys_chdir("/");
  sys_getcwd(cwd_buf, 32);
  std::cout << cur_user << "@localhost: " << cwd_buf << (strcmp(cur_user, "root") == 0 ? "#" : "$") << " ";
  while (commands()) {
    sys_getcwd(cwd_buf, 32);
    std::cout << cur_user << "@localhost: " << cwd_buf << (strcmp(cur_user, "root") == 0 ? "#" : "$") << " ";
  }

  return 0;
}