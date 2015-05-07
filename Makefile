all: main.cpp poi.o mount_poi.o
	g++ main.cpp poi.o mount_poi.o -D_FILE_OFFSET_BITS=64 `pkg-config fuse --cflags --libs` -o poi

poi.o : poi.hpp poi.cpp
	g++ -Wall -c poi.cpp -D_FILE_OFFSET_BITS=64

mount_poi.o : mount_poi.hpp mount_poi.cpp
	g++ -Wall -c mount_poi.cpp -D_FILE_OFFSET_BITS=64

clean:
	rm *~

clear:
	rm *.o
	rm poi
