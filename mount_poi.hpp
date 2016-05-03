///////////////////////////////
// Header fungsi-fungsi fuse //
///////////////////////////////

#pragma once // efisiensi kompilasi c++

#define FUSE_USE_VERSION 29 // versi fuse yang digunakan 2.9.3

#include <errno.h>
#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "poi.hpp" // filesystem

/* Spesifikasi wajib */

/** Memperoleh atribut dari file
 * @param path
 * @param stat buffer
 * @return 0 jika tidak terjadi error
 * */
int poi_getattr(const char* path, struct stat* stbuf);

/** Membaca directory
 * @param path
 * @param buffer
 * @param filler
 * @param offset
 * @param file_info
 * @return 0 jika tidak terjadi error
 * */
int poi_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

/** Membuat sebuah directory
 * @param path
 * @param mode
 * @return 0 jika tidak terjadi error
 * */
int poi_mkdir(const char *path, mode_t mode);

/** Membuat file node, dipanggil untuk setiap pembuatan node non-directory dan non-symlink
 * @param path
 * @param mode
 * @param dev
 * @return 0 jika tidak terjadi error
 * */
int poi_mknod(const char *path, mode_t mode, dev_t dev);

/** Membaca data dari file yang sudah terbuka
 * @param path
 * @param buffer
 * @param offset
 * @param file_info
 * @return 0 jika tidak terjadi error
 * */
int poi_read(const char *path,char *buf, size_t size,off_t offset, struct fuse_file_info *fi);

/** Menghapus sebuah directory
 * @param path
 * @return 0 jika tidak terjadi error
 * */
int poi_rmdir(const char *path);

/** Menghapus file
 * @param path
 * @return 0 jika tidak terjadi error
 * */
int poi_unlink(const char *path);

/** Mengubah nama file
 * @param path
 * @param newpath
 * @return 0 jika tidak terjadi error
 * */
int poi_rename(const char* path, const char* newpath);

/** Menulis data ke file yang sudah terbuka
 * @param path
 * @param buffer
 * @param size
 * @param offset
 * @param file_info
 * @return 0 jika tidak terjadi error
 * */
int poi_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

/** Mengubah ukuran dari sebuah file yang telah terbuka
 * @param path
 * @param newsize
 * @return 0 jika tidak terjadi error
 * */
int poi_truncate(const char *path, off_t newSize);


/* Spesifikasi bonus */

/**
 * Mengubah permission bits dari sebuah file
 * @param path
 * @param mode
 * @return
 */
int poi_chmod(const char *path, mode_t mode);

/**
 * Membuat hard link ke sebuah file
 * @param path
 * @param newpath
 * @return
 */
int poi_link(const char *path, const char *newpath);

/**
 * Membuka file untuk memungkinkan operasi pada file
 * @param path
 * @param fi file info
 * @return
 */
int poi_open(const char* path, struct fuse_file_info* fi);

/* Other dependencies */
/**
 * Mengubah waktu modifikasi dan/atau akses
 * @param  path [description]
 * @param  ubuf [description]
 * @return      [description]
 */
int poi_utimens(const char *path, const timespec tv[2]);
