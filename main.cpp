#include <iostream>
#include <sstream>
#include <string>

#include "dir.h"
#include "fs.h"
#include "pcb.h"
#include "sim_shell.h"

using namespace std;
int main(int argc, char** argv) {
  if (argc >= 2 && !strcmp(argv[1], "-i")) {
    char c[16];
    uint32_t i;
    int p[8];
    char* pn[MAX_SECTOR_NAME_LEN];
    for (int i = 0; i < 8; i++) {
      pn[i] = (char*)malloc(sizeof(char) * 16);
    }

    cout << "Please input disk name(lenth<16): ";
    cin >> c;
    cout << "Please input disk volumn(MB): ";
    cin >> i;
    i *= 1048576;
    create_disk(c, i);
    cout << "Please input partitions number(<8): ";
    cin >> i;
    for (int j = 0; j < i; j++) {
      cout << "Partition " << j << ": " << endl;
      cout << "Partition name(lenth<16): ";
      cin >> pn[j];
      cout << "Partition size(MB): ";
      cin >> p[j];
      p[j] *= 1048576;
    }
    create_partitions(c, i, p, pn);
    for (int i = 0; i < 8; i++) {
      free(pn[i]);
    }
  }
  // create_disk((char *)"test_disk", 512 * (1048576));
  // int sizes[2] = {268173304, 268173304};
  // char *names[8] = {(char *)"p1", (char *)"p2"};
  // create_partitions((char *)"test_disk", 2, sizes, names);

  // virtual_disk test_d = read_disk((char*)"test_disk");
  // fs_init(&test_d, 0);
  // uint32_t fd = sys_open("/file1", O_CREAT);
  // uint32_t fd = sys_open("/file1", O_RDWR);
  string a;
  cout << "Disk name: ";
  cin >> a;
  virtual_disk v_disk = read_disk((char*)a.c_str());
  if (v_disk.disk_volumn == -1) {
    return 0;
  }

  cout << "Partition number: ";
  cin.clear();
  cin.ignore();
  int pn = 0;
  getline(cin, a);
  stringstream t(a);
  if (!((t >> pn) && (pn < 8) && (pn > -1))) {
    pn = 0;
  }
  fs_init(&v_disk, pn);
  while (login())
    ;
  init_pcb(cur);
  strcpy(cur_user, "root");
  sys_getcwd(cwd_buf, 32);
  sys_chdir("/");
  sys_getcwd(cwd_buf, 32);
  std::cout << cur_user << "@localhost: " << cwd_buf
            << (strcmp(cur_user, "root") == 0 ? "#" : "$") << " ";
  while (commands()) {
    sys_getcwd(cwd_buf, 32);
    std::cout << cur_user << "@localhost: " << cwd_buf
              << (strcmp(cur_user, "root") == 0 ? "#" : "$") << " ";
  }
  cout << endl << "logout" << endl;
  return 0;
}