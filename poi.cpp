//////////////////////////////
// File POI.cpp             //
// Modification of CCFS.cpp //
//////////////////////////////

#include <stdexcept> 		// c++ exception
#include "poi.hpp"

/* Global filesystem */
extern POI filesystem;

//////////////////////////
// Realisasi Kelas POI  //
//////////////////////////
/**
 * Konstruktor
 */
POI::POI(){
	time(&mount_time);
}

/**
 * Destruktor
 */
POI::~POI(){
	file.close();
}

/**
 * Buat file *.poi baru
 * @param filename nama file
 */
void POI::create(const char *filename){

	/* buka file dengan mode input-output, binary dan truncate (untuk membuat file baru) */
	file.open(filename, fstream::in | fstream::out | fstream::binary | fstream::trunc);

	/* Buat Volume Information */
	initVolumeInformation(filename);

	/* Buat Allocation Table */
	initAllocationTable();

	/* Buat Data Pool */
	initDataPool();

	file.close();
}

/**
 * Inisialisasi Volume Information
 * @param filename nama file
 */
void POI::initVolumeInformation(const char *filename) {
	/* buffer untuk menulis ke file */
	char buffer[BLOCK_SIZE];
	memset(buffer, 0, BLOCK_SIZE);

	/* Magic string "poi!" */
	memcpy(buffer + 0x00, "poi!", 4);

	/* Nama volume */
	this->filename = string(filename);
	memcpy(buffer + 0x04, filename, strlen(filename));

	/* Kapasitas filesystem, dalam little endian */
	capacity = N_BLOCK;
	memcpy(buffer + 0x24, (char*)&capacity, 4);

	/* Jumlah blok yang belum terpakai, dalam little endian */
	available = N_BLOCK - 1;
	memcpy(buffer + 0x28, (char*)&available, 4);

	/* Indeks blok pertama yang bebas, dalam little endian */
	firstEmpty = 1;
	memcpy(buffer + 0x2C, (char*)&firstEmpty, 4);

	/* String "!iop" */
	memcpy(buffer + 0x1FC, "!iop", 4);

	file.write(buffer, BLOCK_SIZE);
}

/**
 * Inisialisasi Allocation Table
 */
void POI::initAllocationTable() {
	/* root ada */
	short buffer = 0xFFFF;
	file.write((char*)&buffer, 2);

	/* lainnya belum ada */
	buffer = 0;
	for (int i = 1; i < N_BLOCK; i++) {
		file.write((char*)&buffer, 2);
	}
}

/**
 * Inisialisasi Data Pool
 */
void POI::initDataPool() {
	/* Semua blok dikosongkan */
	char buffer[BLOCK_SIZE];
	memset(buffer, 0, BLOCK_SIZE);
	for (int i = 0; i < N_BLOCK; i++) {
		file.write(buffer, BLOCK_SIZE);
	}
}

/**
 * Baca file poi
 * @param filename nama file
 */
void POI::load(const char *filename){
	/* buka file dengan mode input-output, dan binary */
	file.open(filename, fstream::in | fstream::out | fstream::binary);

	/* cek apakah file ada */
	if (!file.is_open()){
		file.close();
		throw runtime_error("File tidak ditemukan");
	}

	/* periksa Volume Information */
	readVolumeInformation();

	/* baca Allocation Table */
	readAllocationTable();
}

/**
 * Membaca Volume Information
 */
void POI::readVolumeInformation() {
	char buffer[BLOCK_SIZE];
	file.seekg(0);

	/* Baca keseluruhan Volume Information */
	file.read(buffer, BLOCK_SIZE);

	/* cek magic string */
	if (string(buffer, 4) != "poi!") {
		file.close();
		throw runtime_error("File bukan file POI yang valid");
	}

	/* baca capacity */
	memcpy((char*)&capacity, buffer + 0x24, 4);

	/* baca available */
	memcpy((char*)&available, buffer + 0x28, 4);

	/* baca firstEmpty */
	memcpy((char*)&firstEmpty, buffer + 0x2C, 4);
}

/**
 * Membaca Allocation Table
 */
void POI::readAllocationTable() {
	char buffer[3];

	/* pindah posisi ke awal Allocation Table */
	file.seekg(0x200);

	/* baca nilai nextBlock */
	for (int i = 0; i < N_BLOCK; i++) {
		file.read(buffer, 2);
		memcpy((char*)&nextBlock[i], buffer, 2);
	}
}

/**
 * Menuliskan Volume Information
 */
void POI::writeVolumeInformation() {
	file.seekp(0x00);

	/* buffer untuk menulis ke file */
	char buffer[BLOCK_SIZE];
	memset(buffer, 0, BLOCK_SIZE);

	/* Magic string "POI" */
	memcpy(buffer + 0x00, "poi!", 4);

	/* Nama volume */
	memcpy(buffer + 0x04, filename.c_str(), filename.length());

	/* Kapasitas filesystem, dalam little endian */
	memcpy(buffer + 0x24, (char*)&capacity, 4);

	/* Jumlah blok yang belum terpakai, dalam little endian */
	memcpy(buffer + 0x28, (char*)&available, 4);

	/* Indeks blok pertama yang bebas, dalam little endian */
	memcpy(buffer + 0x2C, (char*)&firstEmpty, 4);

	/* String "!iop" */
	memcpy(buffer + 0x1FC, "!iop", 4);

	file.write(buffer, BLOCK_SIZE);
}

/**
 * Menuliskan Allocation Table pada posisi tertentu
 * @param position posisi pointer blok
 */
void POI::writeAllocationTable(Block position) {
	file.seekp(BLOCK_SIZE + sizeof(Block) * position);
	file.write((char*)&nextBlock[position], sizeof(Block));
}

/**
 * Mengatur Allocation Table
 * @param position pointer blok
 * @param next     pointer blok berikutnya
 */
void POI::setNextBlock(Block position, Block next) {
	nextBlock[position] = next;
	writeAllocationTable(position);
}

/**
 * Mendapatkan blok kosong yang berikutnya
 * @return pointer ke blok kosong pertama
 */
Block POI::allocateBlock() {
	Block result = firstEmpty;

	setNextBlock(result, END_BLOCK);

	while (nextBlock[firstEmpty] != 0x0000) {
		firstEmpty++;
	}

	available--;
	writeVolumeInformation();

	return result;
}


/**
 * Membebaskan blok
 * @param position pointer yang dibebaskan
 */
void POI::freeBlock(Block position) {
	if (position == EMPTY_BLOCK) {
		return;
	}
	while (position != END_BLOCK) {
		Block temp = nextBlock[position];
		setNextBlock(position, EMPTY_BLOCK);
		position = temp;
		available--;
	}
	writeVolumeInformation();
}

/**
 * Membaca isi block sebesar size kemudian menaruh hasilnya di buf
 * @param  position
 * @param  buffer
 * @param  size
 * @param  offset
 * @return
 */
int POI::readBlock(Block position, char *buffer, int size, int offset) {
	/* kalau sudah di END_BLOCK, return */
	if (position == END_BLOCK) {
		return 0;
	}
	/* kalau offset >= BLOCK_SIZE */
	if (offset >= BLOCK_SIZE) {
		return readBlock(nextBlock[position], buffer, size, offset - BLOCK_SIZE);
	}

	file.seekg(BLOCK_SIZE * DATA_POOL_OFFSET + position * BLOCK_SIZE + offset);
	int size_now = size;
	/* cuma bisa baca sampai sebesar block size */
	if (offset + size_now > BLOCK_SIZE) {
		size_now = BLOCK_SIZE - offset;
	}
	file.read(buffer, size_now);

	/* kalau size > block size, lanjutkan di nextBlock */
	if (offset + size > BLOCK_SIZE) {
		return size_now + readBlock(nextBlock[position], buffer + BLOCK_SIZE, offset + size - BLOCK_SIZE);
	}

	return size_now;
}

/**
 * Menuliskan isi buffer ke filesystem
 * @param  position
 * @param  buffer
 * @param  size
 * @param  offset
 * @return
 */
int POI::writeBlock(Block position, const char *buffer, int size, int offset) {
	/* kalau sudah di END_BLOCK, return */
	if (position == END_BLOCK) {
		return 0;
	}
	/* kalau offset >= BLOCK_SIZE */
	if (offset >= BLOCK_SIZE) {
		/* kalau nextBlock tidak ada, alokasikan */
		if (nextBlock[position] == END_BLOCK) {
			setNextBlock(position, allocateBlock());
		}
		return writeBlock(nextBlock[position], buffer, size, offset - BLOCK_SIZE);
	}

	file.seekp(BLOCK_SIZE * DATA_POOL_OFFSET + position * BLOCK_SIZE + offset);
	int size_now = size;
	if (offset + size_now > BLOCK_SIZE) {
		size_now = BLOCK_SIZE - offset;
	}
	file.write(buffer, size_now);

	/* kalau size > block size, lanjutkan di nextBlock */
	if (offset + size > BLOCK_SIZE) {
		/* kalau nextBlock tidak ada, alokasikan */
		if (nextBlock[position] == END_BLOCK) {
			setNextBlock(position, allocateBlock());
		}
		return size_now + writeBlock(nextBlock[position], buffer + BLOCK_SIZE, offset + size - BLOCK_SIZE);
	}

	return size_now;
}

////////////////////////////
// Realisasi Kelas Entry  //
////////////////////////////

/**
 * Konstruktor default
 * buat Entry kosong
 */
Entry::Entry() {
	position = 0;
	offset = 0;
	memset(data, 0, ENTRY_SIZE);
}

/**
 * Konstruktor parameter
 * @param position
 * @param offset
 */
Entry::Entry(Block position, unsigned char offset) {
	this->position = position;
	this->offset = offset;

	/* baca dari data pool */
	filesystem.file.seekg(BLOCK_SIZE * DATA_POOL_OFFSET + position * BLOCK_SIZE + offset * ENTRY_SIZE);
	filesystem.file.read(data, ENTRY_SIZE);
}

/**
 * Mendapatkan Entry berikutnya
 * @return
 */
Entry Entry::nextEntry() {
	if (offset < 15) {
		return Entry(position, offset + 1);
	}
	else {
		return Entry(filesystem.nextBlock[position], 0);
	}
}

/**
 * Mendapatkan Entry dari path
 * @param  path
 * @return
 */
Entry Entry::getEntry(const char *path) {
	/* mendapatkan direktori teratas */
	unsigned int endstr = 1;
	while (path[endstr] != '/' && endstr < strlen(path)) {
		endstr++;
	}
	string topDirectory = string(path + 1, endstr - 1);

	/* mencari entri dengan nama topDirectory */
	while (getName() != topDirectory && position != END_BLOCK) {
		*this = nextEntry();
	}

	/* kalau tidak ketemu, return Entry kosong */
	if (isEmpty()) {
		return Entry();
	}
	/* kalau ketemu, */
	else {
		if (endstr == strlen(path)) {
			return *this;
		}
		else {
			/* cek apakah direktori atau bukan */
			if (getAttr() & 0x8) {
				Block index;
				memcpy((char*)&index, data + 0x1A, 2);
				Entry next(index, 0);
				return next.getEntry(path + endstr);
			}
			else {
				return Entry();
			}
		}
	}
}

/**
 * Mendapatkan Entry baru dari path
 * @param  path
 * @return
 */
Entry Entry::getNewEntry(const char *path) {
	/* mendapatkan direktori teratas */
	unsigned int endstr = 1;
	while (path[endstr] != '/' && endstr < strlen(path)) {
		endstr++;
	}
	string topDirectory = string(path + 1, endstr - 1);

	/* mencari entri dengan nama topDirectory */
	Entry entry(position, offset);
	while (getName() != topDirectory && position != END_BLOCK) {
		*this = nextEntry();
	}

	/* kalau tidak ketemu, buat entry baru */
	if (isEmpty()) {
		while (!entry.isEmpty()) {
			if (entry.nextEntry().position == END_BLOCK) {
				entry = Entry(filesystem.allocateBlock(), 0);
			}
			else {
				entry = entry.nextEntry();
			}
		}
		/* beri atribut pada entry */
		entry.setName(topDirectory.c_str());
		entry.setAttr(0xF);
		entry.setIndex(filesystem.allocateBlock());
		entry.setSize(BLOCK_SIZE);
		entry.setTime(0);
		entry.setDate(0);
		entry.write();

		*this = entry;
	}

	if (endstr == strlen(path)) {
		return *this;
	}
	else {
		/* cek apakah direktori atau bukan */
		if (getAttr() & 0x8) {
			Block index;
			memcpy((char*)&index, data + 0x1A, 2);
			Entry next(index, 0);
			return next.getNewEntry(path + endstr);
		}
		else {
			return Entry();
		}
	}
}

/**
 * Mengembalikan entry kosong selanjutnya
 * Jika blok penuh akan dibuat entri baru
 * @return
 */
Entry Entry::getNextEmptyEntry() {
	Entry entry(*this);

	while (!entry.isEmpty()) {
		entry = entry.nextEntry();
	}
	if (entry.position == END_BLOCK) {
		/* berarti blok saat ini sudah penuh, buat blok baru */
		Block newPosition = filesystem.allocateBlock();
		Block lastPos = position;
		while (filesystem.nextBlock[lastPos] != END_BLOCK) {
			lastPos = filesystem.nextBlock[lastPos];
		}
		filesystem.setNextBlock(lastPos, newPosition);
		entry.position = newPosition;
		entry.offset = 0;
	}

	return entry;
}

/**
 * Mengosongkan entry
 */
void Entry::makeEmpty() {
	/* menghapus byte pertama data */
	*(data) = 0;
	write();
}

/**
 * Memeriksa apakah Entry kosong atau tidak
 * @return [description]
 */
int Entry::isEmpty() {
	return *(data) == 0;
}

/////////////////////////////////////////
// Getter-Setter atribut-atribut Entry //
/////////////////////////////////////////
string Entry::getName() {
	return string(data);
}

unsigned char Entry::getAttr() {
	return *(data + 0x14);
}

short Entry::getTime() {
	short result;
	memcpy((char*)&result, data + 0x16, 2);
	return result;
}

short Entry::getDate() {
	short result;
	memcpy((char*)&result, data + 0x18, 2);
	return result;
}

Block Entry::getIndex() {
	Block result;
	memcpy((char*)&result, data + 0x1A, 2);
	return result;
}

int Entry::getSize() {
	int result;
	memcpy((char*)&result, data + 0x1C, 4);
	return result;
}

void Entry::setName(const char* name) {
	strcpy(data, name);
}

void Entry::setAttr(const unsigned char attr) {
	data[0x14] = attr;
}

void Entry::setTime(const short time) {
	memcpy(data + 0x16, (char*)&time, 2);
}

void Entry::setDate(const short date) {
	memcpy(data + 0x18, (char*)&date, 2);
}

void Entry::setIndex(const Block index) {
	memcpy(data + 0x1A, (char*)&index, 2);
}

void Entry::setSize(const int size) {
	memcpy(data + 0x1C, (char*)&size, 4);
}

/** Bagian Date Time */
time_t Entry::getDateTime() {
	unsigned int datetime;
	memcpy((char*)&datetime, data + 0x16, 4);

	time_t rawtime;
	time(&rawtime);
	struct tm *result = localtime(&rawtime);

	result->tm_sec = ((datetime >> 16u) & 0x1F) << 1;
	result->tm_min = (datetime >> 21u) & 0x3F;
	result->tm_hour = (datetime >> 27u) & 0x1F;
	result->tm_mday = datetime & 0x1F;
	result->tm_mon = (datetime >> 5u) & 0xF;
	result->tm_year = ((datetime >> 9u) & 0x7F) + 10;

	return mktime(result);
}

void Entry::setCurrentDateTime() {
	time_t now_t;
	time(&now_t);
	struct tm *now = localtime(&now_t);

	int sec = now->tm_sec;
	int min = now->tm_min;
	int hour = now->tm_hour;
	int day = now->tm_mday;
	int mon = now->tm_mon;
	int year = now->tm_year;

	int _time = (sec >> 1) | (min << 5) | (hour << 11);
	int _date = (day) | (mon << 5) | ((year - 10) << 9);

	memcpy(data + 0x16, (char*)&_time, 2);
	memcpy(data + 0x18, (char*)&_date, 2);
}

/**
 * Menuliskan entry ke filesystem
 */
void Entry::write() {
	if (position != END_BLOCK) {
		filesystem.file.seekp(BLOCK_SIZE * DATA_POOL_OFFSET + position * BLOCK_SIZE + offset * ENTRY_SIZE);
		filesystem.file.write(data, ENTRY_SIZE);
	}
}
