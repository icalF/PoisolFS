//////////////////////////////////////
// Implementasi fungsi-fungsi fuse  //
//////////////////////////////////////

#include "mount_poi.hpp"

using namespace std;

extern POI filesystem; // akan dideklarasi di main program

/* Spesifikasi wajib */

/**
 * Memperoleh atribut dari file
 * @param  path  [description]
 * @param  stbuf [description]
 * @return       [description]
 */
int poi_getattr(const char* path, struct stat* stbuf) {
	/* jika root path */
	if (string(path) == "/"){
		stbuf->st_nlink = 1;
		stbuf->st_mode = S_IFDIR | 0777; 			// file dengan permission rwxrwxrwx
		stbuf->st_mtime = filesystem.mount_time;
		return 0;
	}
	else {
		Entry entry = Entry(0, 0).getEntry(path);

		//Kalau path tidak ditemukan
		if (entry.isEmpty()) {
			return -ENOENT;
		}

		// tulis stbuf, tempat memasukkan atribut file
		stbuf->st_nlink = 1;

		// cek direktori atau bukan
		if (entry.getAttr() & 0x8) {
			stbuf->st_mode = S_IFDIR | (0770 + (entry.getAttr() & 0x7));
		}	else {
			stbuf->st_mode = S_IFREG | (0660 + (entry.getAttr() & 0x7));
		}

		// ukuran file
		stbuf->st_size = entry.getSize();

		// waktu pembuatan file
		stbuf->st_mtime = entry.getDateTime();

		return 0;
	}
}

/**
 * Membaca directory
 * @param  path   [description]
 * @param  buf    [description]
 * @param  filler [description]
 * @param  offset [description]
 * @param  fi     [description]
 * @return        [description]
 */
int poi_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
	// current & parent directory
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	Entry entry = Entry(0, 0).getEntry(path);
	Block index = entry.getIndex();
	entry = Entry(index, 0);

	// Menuliskan setiap entry ke buffer "buf"
	while (entry.position != END_BLOCK) {
		if (!entry.isEmpty()) {
			filler(buf, entry.getName().c_str(), NULL, 0);
		}

		entry = entry.nextEntry();
	}

	return 0;
}

/**
 * Membuat sebuah directory
 * @param  path [description]
 * @param  mode [description]
 * @return      [description]
 */
int poi_mkdir(const char *path, mode_t mode){
	int i;
	string parentPath;
	Entry entry;

	// Mencari parent directory
	for (i = strlen(path) - 1; path[i] != '/'; i--);

	parentPath = string(path, i);

	if (parentPath == "") {
		entry = Entry(0, 0);
	}
	else {
		entry = Entry(0, 0).getEntry(parentPath.c_str());
		Block idx = entry.getIndex();
		entry = Entry(idx, 0);
	}

	// mencari entry kosong di parent
	entry = entry.getNextEmptyEntry();

	// menulis data entry
	entry.setName(path + i + 1);
	entry.setAttr(0x0F);
	entry.setCurrentDateTime();
	entry.setIndex(filesystem.allocateBlock());
	entry.setSize(0);

	entry.write();

	return 0;
}

/**
 * Membuat file node, dipanggil untuk setiap pembuatan node non-directory dan non-symlink
 * @param  path [description]
 * @param  mode [description]
 * @param  dev  [description]
 * @return      [description]
 */
int poi_mknod(const char *path, mode_t mode, dev_t dev){
	int i;
	string parentPath;
	Entry entry;

	// Mencari parent directory
	for (i = strlen(path) - 1; path[i] != '/'; i--);

	parentPath = string(path, i);

	if (parentPath == "") {
		entry = Entry(0, 0);
	}
	else {
		entry = Entry(0, 0).getEntry(parentPath.c_str());
		Block idx = entry.getIndex();
		entry = Entry(idx, 0);
	}

	// mencari entry kosong di parent
	entry = entry.getNextEmptyEntry();

	// menulis data entry
	entry.setName(path + i + 1);
	entry.setAttr(0x06);
	entry.setTime(0x00);
	entry.setCurrentDateTime();
	entry.setIndex(filesystem.allocateBlock());
	entry.setSize(0x00);

	entry.write();

	return 0;
}

/**
 * Membaca data dari file yang sudah terbuka
 * @param  path   [description]
 * @param  buf    [description]
 * @param  size   [description]
 * @param  offset [description]
 * @param  fi     [description]
 * @return        [description]
 */
int poi_read(const char *path,char *buf,size_t size,off_t offset,struct fuse_file_info *fi){
	Entry entry = Entry(0, 0).getEntry(path);
	Block index = entry.getIndex();

	if (entry.isEmpty()){
		return -ENOENT;
	}

	return filesystem.readBlock(index, buf, size, offset);
}

/**
 * Menghapus sebuah directory
 * @param  path [description]
 * @return      [description]
 */
int poi_rmdir(const char *path){
	Entry entry = Entry(0, 0).getEntry(path);
	if (entry.isEmpty()) {
		return -ENOENT;
	}

	// menghapus dari allocation table
	filesystem.freeBlock(entry.getIndex());
	entry.makeEmpty();

	return 0;
}

/**
 * Menghapus file
 * @param  path [description]
 * @return      [description]
 */
int poi_unlink(const char *path){
	Entry entry = Entry(0, 0).getEntry(path);
	if (entry.getAttr() & 0x8) {
		return -ENOENT;
	}
	else {
		filesystem.freeBlock(entry.getIndex());
		entry.makeEmpty();
	}

	return 0;
}

/**
 * Mengubah nama file
 * @param  path    [description]
 * @param  newpath [description]
 * @return         [description]
 */
int poi_rename(const char* path, const char* newpath){
	Entry entrySrc = Entry(0, 0).getEntry(path);
	Entry entryDest = Entry(0, 0).getNewEntry(newpath);

	if (!entrySrc.isEmpty()) {
		entryDest.setAttr(entrySrc.getAttr());
		entryDest.setIndex(entrySrc.getIndex());
		entryDest.setSize(entrySrc.getSize());
		entryDest.setTime(entrySrc.getTime());
		entryDest.setDate(entrySrc.getDate());
		entryDest.write();

		entrySrc.makeEmpty();
		return 0;
	}	else {
		return -ENOENT;
	}
}

/**
 * Menulis data ke file yang sudah terbuka
 * @param  path   [description]
 * @param  buf    [description]
 * @param  size   [description]
 * @param  offset [description]
 * @param  fi     [description]
 * @return        [description]
 */
int poi_write(const char *path, const char *buf, size_t size, off_t offset,struct fuse_file_info *fi){
	Entry entry = Entry(0, 0).getEntry(path);
	Block index = entry.getIndex();

	// kasus entry kosong
	if (entry.isEmpty()) {
		return -ENOENT;
	}

	entry.setSize(offset + size);
	entry.write();

	int res = filesystem.writeBlock(index, buf, size, offset);
	return res;
}

/**
 * Mengubah ukuran dari sebuah file yang telah terbuka
 * @param  path    [description]
 * @param  newSize [description]
 * @return         [description]
 */
int poi_truncate(const char *path, off_t newSize){
	Entry entry = Entry(0, 0).getEntry(path);

	// set size
	entry.setSize(newSize);
	entry.write();

	// menangani allocation table
	Block position = entry.getIndex();
	while (newSize > 0) {
		newSize -= BLOCK_SIZE;
		if (newSize > 0) {
			// kasus butuh alokasi baru
			if (filesystem.nextBlock[position] == END_BLOCK)
				filesystem.setNextBlock(position, filesystem.allocateBlock());

			position = filesystem.nextBlock[position];
		}
	}

	filesystem.freeBlock(filesystem.nextBlock[position]);
	filesystem.setNextBlock(position, END_BLOCK);

	return 0;
}

/* Spesifikasi bonus */

/**
 * Mengubah permission bits dari sebuah file
 * @param  path [description]
 * @param  mode [description]
 * @return      [description]
 */
int poi_chmod(const char *path, mode_t mode) {
	Entry entry = Entry(0, 0).getEntry(path);

	if(entry.isEmpty()){
		return -ENOENT;
	}

	// masukkan atribut baru
	entry.setAttr(mode & 0x7);
	entry.write();
	return 0;
}

/**
 * Membuat hard link ke sebuah file
 * @param  path    [description]
 * @param  newpath [description]
 * @return         [description]
 */
int poi_link(const char *path, const char *newpath) {
	Entry oldentry = Entry(0,0).getEntry(path);

	/* kalo nama kosong */
	if(oldentry.isEmpty()){
		return -ENOENT;
	}
	/* buat entry baru dengan nama newpath */
	Entry newentry = Entry(0,0).getNewEntry(newpath);
	/* set atribut untuk newpath */
	newentry.setAttr(oldentry.getAttr());
	newentry.setCurrentDateTime();
	newentry.setSize(oldentry.getSize());
	newentry.write();

	/* copy isi file */
	char buffer[512];
	/* lakukan per 4096 byte */
	int totalsize = oldentry.getSize();
	int offset = 0;
	while (totalsize > 0) {
		int sizenow = totalsize;
		if (sizenow > 512) {
			sizenow = 512;
		}
		filesystem.readBlock(oldentry.getIndex(), buffer, oldentry.getSize(), offset);
		filesystem.writeBlock(newentry.getIndex(), buffer, newentry.getSize(), offset);
		totalsize -= sizenow;
		offset += 512;
	}

	return 0;
}

/**
 * Membukan file untuk memungkinkan operasi pada file
 * @param  path [description]
 * @param  fi   [description]
 * @return      [description]
 */
int poi_open(const char* path, struct fuse_file_info* fi) {
	/* hanya mengecek apakah file ada atau tidak */

	Entry entry = Entry(0,0).getEntry(path);

	if(entry.isEmpty()) {
		return -ENOENT;
	}
	return 0;
}

/* Other dependencies */
/**
 * Mengubah waktu modifikasi dan/atau akses
 * @param  path [description]
 * @param  ubuf [description]
 * @return      [description]
 */
int poi_utime(const char *path, struct utimbuf *ubuf) {
	Entry entry = Entry(0,0).getEntry(path);

	if(entry.isEmpty()) {
		return -ENOENT;
	}

	entry.setCurrentDateTime();
	entry.write();
	return 0;
}
