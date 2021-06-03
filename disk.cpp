#include "disk.h"
#include "fs.h"
#include <iostream>
#include <cstring>
#include <fstream>
using namespace std;

int create_disk(char *disk_name, uint32_t disk_volumn)
{
    ofstream fp;
    fp.open(disk_name, ios::out | ios::binary);
    if (!fp.is_open()) //打不开时输出ERROR,返回0
    {
        cout << "ERROR: cannot open the file" << endl;
        return 0;
    }
    else
    {
        virtual_disk disk;
        strcpy(disk.disk_name, disk_name);
        disk.disk_volumn = disk_volumn;
        disk.current_partition_count = 0;
        fp.seekp(0, ios::beg);
        fp.write(reinterpret_cast<char *>(&disk), sizeof(disk));
        fp.seekp(disk_volumn, ios::beg); //跳转到设定大小的字节数位置
        fp << "-1" << endl;
    }
    fp.close();
    return 1;
}

int create_partitions(char *disk_name, int partition_num, int *partitions_size, char **partitions_name)
{

    fstream fp;
    uint32_t start_sector_no = 1024;
    fp.open(disk_name, ios::binary | ios::in | ios::out);
    if (!fp.is_open())
    {
        cout << "ERROR: cannot open the file" << endl;
        return 0;
    }
    else
    {
        virtual_disk disk;
        fp.read(reinterpret_cast<char *>(&disk), sizeof(disk));
        for (int i = 0; i < partition_num; i++)
        {
            if (partitions_size[i] < SECTOR_SIZE * 5) //暂定一个分区至少五个块
            {
                cout << "ERROR:partitions_size are too small" << endl;
                return 0;
            }
            else
            {
                disk.current_partition_count++;
                disk.partitions[i].start_sector_no = start_sector_no;
                disk.partitions[i].sector_count = partitions_size[i] / SECTOR_SIZE;
                strcpy(disk.partitions[i].partition_name, partitions_name[i]);
                start_sector_no = start_sector_no + disk.partitions[i].sector_count;
            }
        }
        fp.seekp(0, ios::beg);
        fp.write(reinterpret_cast<char *>(&disk), sizeof(disk));
        fp.close();
        return 1;
    }
}

virtual_disk read_disk(char* disk_name)
{
    ifstream fp;
    fp.open(disk_name, ios::in | ios::binary);
    if (!fp.is_open())
    {
        // useless but remove warning
        virtual_disk disk;
        cout << "ERROR: cannot open the file" << endl;
        return disk;
    }
    else
    {
        virtual_disk disk;
        fp.read(reinterpret_cast<char *>(&disk), sizeof(disk));
        fp.close();
        return disk;
    }
}

// int main()
// {
//     char *disk_name = "testdisk";
    
//     uint32_t disk_volumn = 536870912;
//     create_disk(disk_name, disk_volumn);
//     int partition_num = 8;
//     int partitions_size[8];
//     for(int i = 0;i<8;i++)
//     {
//         partitions_size[i] = {67000000};
        
//     }
//     char *name[8] = { (char*)"ABC", (char*)"ABC", (char*)"ABC", (char*)"ABC", (char*)"ABC", (char*)"ABC", (char*)"ABC", (char*)"ABC"}; 
//     create_partitions(disk_name,partition_num,partitions_size,name);

//     virtual_disk disk;
//     disk = read_disk(disk_name);
//     cout << disk.current_partition_count << " " << disk.disk_volumn << " " << disk.disk_name << endl;
//     return 0;
// }
