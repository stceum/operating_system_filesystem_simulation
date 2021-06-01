test_fs: fs.o disk.o
	g++ -o test_fs fs.o disk.o

fs.o: fs.cpp fs.h tools.h inode.h dir.h super_block.h
	g++ -c fs.cpp

disk.o: disk.cpp disk.h fs.h
	g++ -c disk.cpp

clean:
	rm test_fs fs.o disk.o