//////////////////////////////
// File POI.hpp             //
// Modification of CCFS.hpp //
//////////////////////////////

#pragma once

#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <ctime>

/** Definisi tipe **/
typedef unsigned short Block;

/** Konstanta **/
/* Konstanta ukuran */
#define BLOCK_SIZE 512
#define N_BLOCK 65536
#define ENTRY_SIZE 32
#define DATA_POOL_OFFSET 257
/* Konstanta untuk Block */
#define EMPTY_BLOCK 0x0000
#define END_BLOCK 0xFFFF

using namespace std;

/**
 * Class POI
 * kelas filesystem
 */
class POI{
public:
/* Method */
	/* konstruktor & destruktor */
	POI();
	~POI();

	/* buat file *.poi */
	void create(const char *filename);
	void initVolumeInformation(const char *filename);
	void initAllocationTable();
	void initDataPool();

	/* baca file *.poi */
	void load(const char *filename);
	void readVolumeInformation();
	void readAllocationTable();

	void writeVolumeInformation();
	void writeAllocationTable(Block position);

	/* bagian alokasi block */
	void setNextBlock(Block position, Block next);
	Block allocateBlock();
	void freeBlock(Block position);

	/* bagian baca/tulis block */
	int readBlock(Block position, char *buffer, int size, int offset = 0);
	int writeBlock(Block position, const char *buffer, int size, int offset = 0);

/* Attributes */
	fstream file;			// file .poi
	Block nextBlock[N_BLOCK];	//pointer ke blok berikutnya

	string filename;		// nama volume
	int capacity;			// kapasitas filesystem dalam blok
	int available;			// jumlah slot yang masih kosong
	int firstEmpty;			// slot pertama yang masih kosong
	time_t mount_time;		// waktu mounting, diisi di konstruktor
};

/**
 * Class Entry
 */
class Entry {
public:
/* Method */
	Entry();
	Entry(Block position, unsigned char offset);
	Entry nextEntry();
	Entry getEntry(const char *path);
	Entry getNewEntry(const char *path);
	Entry getNextEmptyEntry();

	void makeEmpty();
	int isEmpty();

	string getName();
	unsigned char getAttr();
	short getTime();
	short getDate();
	Block getIndex();
	int getSize();

	void setName(const char* name);
	void setAttr(const unsigned char attr);
	void setTime(const short time);
	void setDate(const short date);
	void setIndex(const Block index);
	void setSize(const int size);

	time_t getDateTime();
	void setCurrentDateTime();

	void write();

/* Attributes */
	char data[ENTRY_SIZE];
	Block position;	//posisi blok
	unsigned char offset;	//offset dalam satu blok (0..15)
};
