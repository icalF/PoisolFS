//////////////////////////
// Main program Poi-FS  //
//////////////////////////

#include <iostream>
#include "mount_poi.hpp"
#include "poi.hpp"

using namespace std;

struct fuse_operations poi_oper;
void init_fuse();

POI filesystem;

int main(int argc, char** argv){
  if (argc < 3 || (argc > 3 && string(argv[3]) != "-new")) {
    printf("Usage: ./poi <mount folder> <filesystem.poi> [-new]\n");
    return 0;
  }

  // Argumen -new; buat poi baru
  if (argc > 3 && string(argv[3]) == "-new") {
    filesystem.create(argv[2]);
  }

  filesystem.load(argv[2]);

  // Buat argumen baru untuk fuse
  int fuse_argc = 2;
  char* fuse_argv[2] = {argv[0], argv[1]};

  // Jalankan fuse
  init_fuse();
  return fuse_main(fuse_argc, fuse_argv, &poi_oper, NULL);
}

void init_fuse() {
  poi_oper.getattr = poi_getattr;
  poi_oper.readdir = poi_readdir;
  poi_oper.mkdir = poi_mkdir;
  poi_oper.mknod = poi_mknod;
  poi_oper.read = poi_read;
  poi_oper.rmdir = poi_rmdir;
  poi_oper.unlink = poi_unlink;
  poi_oper.rename = poi_rename;
  poi_oper.write = poi_write;
  // poi_oper.utime = poi_utime;
  poi_oper.truncate = poi_truncate;
  poi_oper.chmod = poi_chmod;
  poi_oper.link = poi_link;
  poi_oper.open = poi_open;
};
