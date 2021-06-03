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
          type = (char*)"regular";
        } else {
          type = (char*)"directory";
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

  else if (tmp == "touch") // touch
  {  
    if (ss >> tmp) {
      if (tmp[0] != '/') {
        tmp = cwd_buf + ("/" + tmp);
      }
      sys_open(tmp.c_str(), O_CREAT);
    } else {
      sys_open(tmp.c_str(), O_CREAT);
    }
  } 
  
  else if (tmp == "mv") 
  {
  } 
  
  else if (tmp == "cp") 
  {
  } 
  
  else if (tmp == "rm") 
  {
  } 
  
  else if (tmp == "echo") 
  {
  }

  else if (tmp == "cat") 
  {
  } 

  else if (tmp == "mkdir") 
  {
  } 

  else if (tmp == "su") 
  {
    cout << "su!" << endl;
  } 

  else if (tmp == "exit") 
  {
    return 0;
  }
  
  return 1;
}