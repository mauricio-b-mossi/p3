#include "../libWad/Wad.h"
#include <fuse.h>
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>

using namespace std;

// Not setting permissions
static int wad_getattr(const char *path, struct stat *stbuf) {

    auto wad = ((Wad*)fuse_get_context()->private_data);

    memset(stbuf, 0, sizeof(struct stat));

	stbuf->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
	stbuf->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
	stbuf->st_atime = time( NULL ); // The last "a"ccess of the file/directory is right now
	stbuf->st_mtime = time( NULL ); // The last "m"odification of the file/directory is right now

    if(wad->isDirectory(path) || strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0777; // drwxrwxrwx
        stbuf->st_nlink = 2; // Directories have at least two links
    }
    else if (wad->isContent(path)) {
     stbuf->st_mode = S_IFREG | 0777; // -rwxrwxrwx
        stbuf->st_nlink = 1; // Regular files have one link
        stbuf->st_size = wad->getSize(path);
    }
    else{
        return -ENOENT;
    }

    return 0;
}

// Working
static int wad_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
    auto wad = ((Wad*)fuse_get_context()->private_data);
    return wad->getContents(path, buffer, size, offset);
}

// Working
static int wad_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    auto wad = ((Wad*)fuse_get_context()->private_data);

    filler(buffer, ".", NULL, 0); // Filling buffer with standard Curr and Parent Dir.
    filler(buffer, "..", NULL, 0);

    vector<string> childrenNames;
    wad->getDirectory(path, &childrenNames);

    for (string name : childrenNames) {
	    filler(buffer, name.c_str(), NULL, 0);
    }

    return 0;
}

// Working
static int wad_mkdir(const char *path, mode_t mode)
{
    auto wad = ((Wad*)fuse_get_context()->private_data);
    wad->createDirectory(path);
	
	return 0;
}

// Working
static int wad_mknod(const char *path, mode_t mode, dev_t rdev)
{
    auto wad = ((Wad*)fuse_get_context()->private_data);
    wad->createFile(path);
	
	return 0;
}

static int wad_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *info )
{
    auto wad = ((Wad*)fuse_get_context()->private_data);
    return wad->writeToFile(path, buffer, size, offset);
}

static struct fuse_operations wad_operations = {
    .getattr    = wad_getattr,    // Ensure this matches the 'getattr' field
    .readlink   = NULL,           // If not implemented, set it to NULL
    .mknod      = wad_mknod,
    .mkdir      = wad_mkdir,
    .unlink     = NULL,
    .rmdir      = NULL,
    .symlink    = NULL,
    .rename     = NULL,
    .link       = NULL,
    .chmod      = NULL,
    .chown      = NULL,
    .truncate   = NULL,
    .open       = NULL,
    .read       = wad_read,
    .write      = wad_write,
    .readdir    = wad_readdir,
};


int main(int argc, char* argv[]){
    if(argc < 3){
        cout << "Not enough arguments" << endl;
        return 0;
    }

    string wadPath = argv[argc - 2];

    if(wadPath.at(0) != '/'){
        wadPath = string(get_current_dir_name()) + "/" + wadPath;
    }

    Wad* myWad = Wad::loadWad(wadPath);

    argv[argc - 2] = argv[argc - 1];
    argc--;

    //((Wad*)fuse_get_context()->private_data)
    fuse_main(argc, argv, &wad_operations, myWad);

    delete myWad;

    return 0;

}