test_fs: fs.o disk.o list.o
	g++ -o test_fs fs.o disk.o list.o

fs.o: fs.cpp fs.h tools.h inode.h dir.h super_block.h bitmap.h list.h
	g++ -c fs.cpp

disk.o: disk.cpp disk.h fs.h
	g++ -c disk.cpp

list.o: list.cpp list.h
	g++ -c list.cpp

clean:
	rm test_fs fs.o disk.o list.o