#include "sim_shell.h"

#include <cstring>
#include <iostream>
#include <sstream>

#include "dir.h"
#include "disk.h"
#include "fs.h"
#include "pcb.h"

using namespace std;

int commands() {
  string input, tmp;
  if (!getline(cin, input)) return 0;
  stringstream ss(input);
  ss >> tmp;
  if (tmp == "cd")  // cd
  {
    if (ss >> tmp) {
      if (tmp[0] != '/') {
        tmp = cwd_buf + ("/" + tmp);
      }
      sys_chdir((tmp.c_str()));
    } else {
      sys_chdir((char*)"/");
    }
  }

  else if (tmp == "ls")  // ls
  {
    if (ss >> tmp) {
      if (tmp[0] != '/') {
        tmp = cwd_buf + ("/" + tmp);
      }
    } else {
      tmp = cwd_buf;
    }
    struct dir* p_dir = sys_opendir(tmp.c_str());
    if (p_dir) {
      char* type = NULL;
      struct dir_entry* dir_e = NULL;
      while ((dir_e = sys_readdir(p_dir))) {
        if (dir_e->f_type == FT_REGULAR) {
          type = (char*)"nor";
        } else {
          type = (char*)"dir";
        }
        printf(" %s %s\n", type, dir_e->filename);
      }
      if (sys_closedir(p_dir) == 0) {
        // printf("%s close done!\n", cwd_buf);

      } else {
        printf("%s close fail!\n", cwd_buf);
      }

    } else {
      printf("%s open fail!\n", cwd_buf);
    }
  }

  else if (tmp == "touch")  // touch
  {
    if (ss >> tmp) {
      if (tmp[0] != '/') {
        tmp = cwd_buf + ("/" + tmp);
      }
      uint32_t fd = sys_open(tmp.c_str(), O_CREAT);
      if (fd != -1) sys_close(fd);
    }
  }

  else if (tmp == "mv") {
    string tmp;
    string path_s;
    uint32_t fd_s, fd_d;
    if (ss >> tmp) {
      if (tmp[0] != '/') tmp = cwd_buf + ("/" + tmp);
      path_s = tmp;
      fd_s = sys_open(tmp.c_str(), O_RDWR);
      if (fd_s == -1) {
        cout << tmp << "does not exist." << endl;
      } else {
        if (ss >> tmp) {
          if (tmp[0] != '/') tmp = cwd_buf + ("/" + tmp);
          fd_d = sys_open(tmp.c_str(), O_CREAT | O_RDWR);
          if (fd_d == -1) {
            cout << "create " << tmp << " failed." << endl;
          } else {
            char buf[100];
            int w_size;
            while ((w_size = sys_read(fd_s, buf, 100)) > 0) {
              sys_write(fd_d, buf, w_size);
            }
            sys_close(fd_d);
          }
        }
        sys_close(fd_s);
        sys_unlink(path_s.c_str());
      }
    }
  }

  else if (tmp == "cp") {
    string tmp;
    uint32_t fd_s, fd_d;
    if (ss >> tmp) {
      if (tmp[0] != '/') tmp = cwd_buf + ("/" + tmp);
      fd_s = sys_open(tmp.c_str(), O_RDWR);
      if (fd_s == -1) {
        cout << tmp << "does not exist." << endl;
      } else {
        if (ss >> tmp) {
          if (tmp[0] != '/') tmp = cwd_buf + ("/" + tmp);
          fd_d = sys_open(tmp.c_str(), O_CREAT | O_RDWR);
          if (fd_d == -1) {
            cout << "create " << tmp << " failed." << endl;
          } else {
            char buf[100];
            int w_size;
            while ((w_size = sys_read(fd_s, buf, 100)) > 0) {
              sys_write(fd_d, buf, w_size);
            }
            sys_close(fd_d);
          }
        }
        sys_close(fd_s);
      }
    }
  }

  else if (tmp == "rm") {
    int p_ptr = 0;  // 第一个参数的下标
    if (ss >> tmp) {
      if (tmp[0] == '-') {
        if (tmp[1] == 'r') {
          p_ptr = 1;
          ss >> tmp;
        } else {
          cout << "unknown parameter!" << endl;
        }
      }
      if (tmp[p_ptr] != '/') {
        tmp = cwd_buf + ("/" + tmp);
      }
    } else {
      tmp = cwd_buf;
    }
    if (p_ptr) {  // rmdir
      if (sys_rmdir(tmp.c_str())) {
        cout << "failed to remove " << tmp << endl;
      }
    } else {  // rm file
      if (sys_unlink(tmp.c_str()) != 0) {
        cout << "failed to remove " << tmp << endl;
      }
    }
  }

  else if (tmp == "wtf") {
    if (ss >> tmp) {
      string input = tmp;
      if (ss >> tmp) {
        if (tmp[0] != '/') tmp = cwd_buf + ("/" + tmp);
        uint32_t fd = sys_open(tmp.c_str(), O_RDWR);
        if (fd == -1) fd = sys_open(tmp.c_str(), O_CREAT | O_RDWR);
        int a;
        if ((a = sys_write(fd, input.c_str(), input.size())) == -1)
          cout << "write failed!\n";
        else
          sys_close(fd);
      }
    }
  }

  else if (tmp == "cat") {
    if (ss >> tmp) {
      if (tmp[0] != '/') tmp = cwd_buf + ("/" + tmp);
      uint32_t fd = sys_open(tmp.c_str(), O_RDONLY);
      char buf[64] = {0};
      while (sys_read(fd, buf, 64) != -1) {
        cout << buf;
      }
      cout << endl;
      sys_close(fd);
    }
  }

  else if (tmp == "mkdir") {
    if (ss >> tmp) {
      if (tmp[0] != '/') {
        tmp = cwd_buf + ("/" + tmp);
      }
    } else {
      tmp = cwd_buf;
    }
    if (sys_mkdir(tmp.c_str()) != 0) {
      cout << "mkdir failed!" << endl;
    }
  }

  else if (tmp == "su") {
    if (strcmp(cur_user, "root") == 0) {
      if (ss >> tmp) {
        strcpy(cur_user, tmp.c_str());
      }
    } else {
      cout << "Permission denied!" << endl;
    }
  }

  else if (tmp == "exit") {
    return 0;
  }

  return 1;
}

int login() {
  string username, passwd;
  cout << "Please input username: ";
  cin >> username;
  cout << "password: ";
  cin >> passwd;
  if (username == "root" && passwd == "403") {
    cin.ignore();
    return 0;
  } else {
    cout << "Uncorrect!" << endl;
    cin.ignore();
    return 1;
  }
}