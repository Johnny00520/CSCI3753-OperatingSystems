/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  Minor modifications and note by Andy Sayler (2012) <www.andysayler.com>

  Source: fuse-2.8.7.tar.gz examples directory
  http://sourceforge.net/projects/fuse/files/fuse-2.X/

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags` fusexmp.c -o fusexmp `pkg-config fuse --libs`

  Note: This implementation is largely stateless and does not maintain
        open file handels between open and release calls (fi->fh).
        Instead, files are opened and closed as necessary inside read(), write(),
        etc calls. As such, the functions that rely on maintaining file handles are
        not implmented (fgetattr(), etc). Those seeking a more efficient and
        more complete implementation may wish to add fi->fh support to minimize
        open() and close() calls and support fh dependent functions.


CSCI3753: Operating Systems - PA4
Chen Hao Cheng
*/

#define FUSE_USE_VERSION 28
#define HAVE_SETXATTR

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include <fuse.h> // /usr/include/fuse.h
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h> // define the type DIR
#include <errno.h>
#include <sys/time.h>
#include <limits.h> // to have PATH_MAX
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include "aes-crypt.h"

//Cipher action (1=encrypt, 0=decrypt, -1=pass-through (copy))
#define DECRYPT 0
#define ENCRYPT 1

typedef struct {
    char *rootdir;
    char *password;
} encfs_state;

//#define ENCFS_DATA ((struct encfs_state *) fuse_get_context()->private_data)

//  All the paths I see are relative to the root of the mounted
//  filesystem.  In order to get to the underlying filesystem, I need to
//  have the mountpoint.  I'll save it away early on in main(), and then
//  whenever I need a path for something I'll call this to construct
//  it.
static void encfs_fullpath(char fpath[PATH_MAX], const char *path)
{
	encfs_state *ENCFS_DATA = ((encfs_state *) fuse_get_context() -> private_data);
    strcpy(fpath, ENCFS_DATA -> rootdir);
    strncat(fpath, path, PATH_MAX); // ridiculously long paths will break here
}
//
// Prototypes for all these functions, and the C-style comments,
// come from /usr/include/fuse.h
//
/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
static int xmp_getattr(const char *path, struct stat *stbuf)
{
	int res;
	
	char fpath[PATH_MAX];
    encfs_fullpath(fpath, path);

	res = lstat(fpath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_access(const char *path, int mask)
{
	int res;

	char fpath[PATH_MAX];
    encfs_fullpath(fpath, path);

	res = access(fpath, mask);
	if (res == -1)
		return -errno;

	return 0;
}

/** Read the target of a symbolic link
 *
 * The buffer should be filled with a null terminated string.  The
 * buffer size argument includes the space for the terminating
 * null character.  If the linkname is too long to fit in the
 * buffer, it should be truncated.  The return value should be 0
 * for success.
 */
// Note the system readlink() will truncate and lose the terminating
// null.  So, the size passed to to the system readlink() must be one
// less than the size passed to bb_readlink()
// bb_readlink() code by Bernardo F Costa (thanks!)
static int xmp_readlink(const char *path, char *buf, size_t size)
{
	int res;

	char fpath[PATH_MAX];
    encfs_fullpath(fpath, path);

	res = readlink(fpath, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
    // DIR type defined in <dirent.h>
	DIR *dp;
	struct dirent *de;

	char fpath[PATH_MAX];
    encfs_fullpath(fpath, path);

	(void) offset;
	(void) fi;

	dp = opendir(fpath);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

/** Create a file node
 *
 * There is no create() operation, mknod() will be called for
 * creation of all non-directory, non-symlink nodes.
 */
// shouldn't that comment be "if" there is no.... ?
static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;

	char fpath[PATH_MAX];
    encfs_fullpath(fpath, path);

	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(fpath, mode);
	else
		res = mknod(fpath, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

/** Create a directory */
static int xmp_mkdir(const char *path, mode_t mode)
{
	int res;

	char fpath[PATH_MAX];
    encfs_fullpath(fpath, path);

	res = mkdir(fpath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

/** Remove/unlink a file/directory */
static int xmp_unlink(const char *path)
{
	int res;

	char fpath[PATH_MAX];
    encfs_fullpath(fpath, path);

	res = unlink(fpath);
	if (res == -1)
		return -errno;

	return 0;
}

/** Remove a directory */
static int xmp_rmdir(const char *path)
{
	int res;

	char fpath[PATH_MAX];
    encfs_fullpath(fpath, path);

	res = rmdir(fpath);
	if (res == -1)
		return -errno;

	return 0;
}

/** Create a symbolic link */
// The parameters here are a little bit confusing, but do correspond
// to the symlink() system call.  The 'path' is where the link points,
// while the 'link' is the link itself.  So we need to leave the path
// unaltered, but insert the link into the mounted directory.
static int xmp_symlink(const char *from, const char *to)
{
	int res;
    char ffrom[PATH_MAX];
    encfs_fullpath(ffrom, from);
    char fto[PATH_MAX];
    encfs_fullpath(fto, to);

	res = symlink(ffrom, fto);
	if (res == -1)
		return -errno;

	return 0;
}

/** Rename a file */
// both path and newpath are fs-relative
static int xmp_rename(const char *from, const char *to)
{
	int res;
    char ffrom[PATH_MAX];
    encfs_fullpath(ffrom, from);
    char fto[PATH_MAX];
    encfs_fullpath(fto, to);

	res = rename(ffrom, fto);
	if (res == -1)
		return -errno;

	return 0;
}

/** Create a hard link to a file */
static int xmp_link(const char *from, const char *to)
{
	int res;
    
    char ffrom[PATH_MAX];
    encfs_fullpath(ffrom, from);
    char fto[PATH_MAX];
    encfs_fullpath(fto, to);

	res = link(ffrom, fto);
	if (res == -1)
		return -errno;

	return 0;
}

/** Change the permission bits of a file */
static int xmp_chmod(const char *path, mode_t mode)
{
	int res;

	char fpath[PATH_MAX];
    encfs_fullpath(fpath, path);

	res = chmod(fpath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

/** Change the owner and group of a file */
static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
	int res;

	char fpath[PATH_MAX];
    encfs_fullpath(fpath, path);

	res = lchown(fpath, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

/* Change the size of a file */
static int xmp_truncate(const char *path, off_t size)
{
	int res;

	char fpath[PATH_MAX];
    encfs_fullpath(fpath, path);

	res = truncate(fpath, size);
	if (res == -1)
		return -errno;

	return 0;
}

/** Change the access and/or modification times of a file */
static int xmp_utimens(const char *path, const struct timespec ts[2])
{
	int res;
	struct timeval tv[2];

	char fpath[PATH_MAX];
    encfs_fullpath(fpath, path);

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	res = utimes(fpath, tv);
	if (res == -1)
		return -errno;

	return 0;
}
/* File open operation */
static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	int res;

	char fpath[PATH_MAX];
    encfs_fullpath(fpath, path);

	res = open(fpath, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

/* Read data from an open file */
static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;
	char fpath[PATH_MAX];
    encfs_fullpath(fpath, path);
    char enc[5];
	(void) fi;

	/* getxattr() retrieves the value of the extended attribute identified
       by name and associated with the given path in the filesystem.  The
       attribute value is placed in the buffer pointed to by value; size
       specifies the size of that buffer.  The return value of the call is
       the number of bytes placed in value. */ 
	//getxattr(fpath, "user.pa4-encfs.encrypted", enc, 5);

	//if encrypted
	if(getxattr(fpath, "user.pa4-encfs.encrypted", enc, 5) != -1){
		FILE* inFile = fopen(fpath, "r");
		FILE* outFile = tmpfile();
		encfs_state *ENCFS_DATA = ((encfs_state *) fuse_get_context()->private_data);

		do_crypt(inFile, outFile, DECRYPT, ENCFS_DATA->password);

 		//int fseek(FILE *stream, long offset, int whence);
 		/* Description from Linux Programmer's manual: The fseek() function sets the file position indicator for the stream
 		pointed to by stream.  The new position, measured in bytes, is
 		obtained by adding offset bytes to the position specified by whence.
 		If whence is set to SEEK_SET, SEEK_CUR, or SEEK_END, the offset is
 		relative to the start of the file, the current position indicator, or
 		end-of-file, respectively.  A successful call to the fseek() function
 		clears the end-of-file indicator for the stream and undoes any
 		effects of the ungetc(3) function on the same stream.*/
		fseek(outFile, 0, SEEK_END);
		// long ftell(FILE *stream);
		/*The ftell() function obtains the current value of the file position
		indicator for the stream pointed to by stream.*/
        size_t outFile_length = ftell(outFile);
        fseek(outFile, 0, SEEK_SET);
        
        res = fread(buf, 1, outFile_length, outFile);
        if (res == -1)
            return -errno;

        fclose(inFile);
        fclose(outFile);

	}

	//else its not encrypted
	else{

		fd = open(fpath, O_RDONLY);
		if (fd == -1)
			return -errno;

		res = pread(fd, buf, size, offset);
		if (res == -1)
			res = -errno;

		close(fd);
	}
	return res;
}

/* Write data to an open file */
static int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;
	char fpath[PATH_MAX];
    encfs_fullpath(fpath, path);
	(void) fi;

    char enc[5];
	getxattr(fpath, "user.pa4-encfs.encrypted", enc, 5);

	if(strcmp(enc,"true") != 0){
		fd = open(fpath, O_WRONLY);
		if (fd == -1)
			return -errno;

		res = pwrite(fd, buf, size, offset);
		if (res == -1)
			res = -errno;

		close(fd);		
	}

	else {
		FILE* inFile = fopen(fpath, "r+"); //read and write
		FILE* outFile = tmpfile();
		encfs_state *ENCFS_DATA = ((encfs_state *) fuse_get_context()->private_data);

 		//int fseek(FILE *stream, long offset, int whence);
 		/* Description from Linux Programmer's manual: The fseek() function sets the file position indicator for the stream
 		pointed to by stream.  The new position, measured in bytes, is
 		obtained by adding offset bytes to the position specified by whence.
 		If whence is set to SEEK_SET, SEEK_CUR, or SEEK_END, the offset is
 		relative to the start of the file, the current position indicator, or
 		end-of-file, respectively.  A successful call to the fseek() function
 		clears the end-of-file indicator for the stream and undoes any
 		effects of the ungetc(3) function on the same stream.*/
		fseek(inFile, 0, SEEK_SET);
        do_crypt(inFile, outFile, DECRYPT, ENCFS_DATA -> password);
        fseek(inFile, 0, SEEK_SET);
        
        res = fwrite(buf, 1, size, outFile);
        if (res == -1)
            return -errno;
        fseek(outFile, 0, SEEK_SET);
		
        do_crypt(outFile, inFile, ENCRYPT, ENCFS_DATA-> password);

        fclose(outFile);
        fclose(inFile);
	}
	
	return res;
}

/* Get file system statistics */
static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	int res;

	char fpath[PATH_MAX];
    encfs_fullpath(fpath, path);

	res = statvfs(fpath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_create(const char* path, mode_t mode, struct fuse_file_info* fi) {

    (void) fi;
	char fpath[PATH_MAX];
    encfs_fullpath(fpath, path);

    int res;
    res = creat(fpath, mode);
    if(res == -1)
	return -errno;

    close(res);

   /* setxattr() sets the value of the extended attribute identified by
    name and associated with the given path in the filesystem.  The size
    argument specifies the size (in bytes) of value; a zero-length value
    is permitted.*/
	setxattr(fpath, "user.pa4-encfs.encrypted", "true", 5, 0);

    return 0;
}

/** Release an open file */
static int xmp_release(const char *path, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */
	(void) path;
	(void) fi;
	return 0;
}

/** Synchronize file contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data.
 *
 * Changed in version 2.2
 */
static int xmp_fsync(const char *path, int isdatasync, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */
	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_SETXATTR
/** Set extended attributes */
static int xmp_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
{
	char fpath[PATH_MAX];
    encfs_fullpath(fpath, path);
	
	int res = lsetxattr(fpath, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}
/** Get extended attributes */
static int xmp_getxattr(const char *path, const char *name, char *value, size_t size)
{
	char fpath[PATH_MAX];
    encfs_fullpath(fpath, path);
	
	int res = lgetxattr(fpath, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

/** List extended attributes */
static int xmp_listxattr(const char *path, char *list, size_t size)
{
	char fpath[PATH_MAX];
    encfs_fullpath(fpath, path);

	int res = llistxattr(fpath, list, size);	
	if (res == -1)
		return -errno;
	return res;
}

/** Remove extended attributes */
static int xmp_removexattr(const char *path, const char *name)
{
	char fpath[PATH_MAX];
    encfs_fullpath(fpath, path);
	int res = lremovexattr(fpath, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations xmp_oper = {
	.getattr	= xmp_getattr,
	.access		= xmp_access,
	.readlink	= xmp_readlink,
	.readdir	= xmp_readdir,
	.mknod		= xmp_mknod,
	.mkdir		= xmp_mkdir,
	.symlink	= xmp_symlink,
	.unlink		= xmp_unlink,
	.rmdir		= xmp_rmdir,
	.rename		= xmp_rename,
	.link		= xmp_link,
	.chmod		= xmp_chmod,
	.chown		= xmp_chown,
	.truncate	= xmp_truncate,
	.utimens	= xmp_utimens,
	.open		= xmp_open,
	.read		= xmp_read,
	.write		= xmp_write,
	.statfs		= xmp_statfs,
	.create     = xmp_create,
	.release	= xmp_release,
	.fsync		= xmp_fsync,
#ifdef HAVE_SETXATTR
	.setxattr	= xmp_setxattr,
	.getxattr	= xmp_getxattr,
	.listxattr	= xmp_listxattr,
	.removexattr	= xmp_removexattr,
#endif
};

void usage()
{
    fprintf(stderr, "usage format: ./pa4-encfs <Key Phrase> <Mirror Directory> <Mount Point>\n");
    abort();
}

int main(int argc, char *argv[])
{
	umask(0);
	encfs_state *encfs_data;
	encfs_data = malloc(sizeof(encfs_state));
    if (encfs_data == NULL) {
		perror("main calloc");
		abort();
    }
    
    // bbfs doesn't do any access checking on its own (the comment
    // blocks in fuse.h mention some of the functions that need
    // accesses checked -- but note there are other functions, like
    // chown(), that also need checking!).  Since running bbfs as root
    // will therefore open Metrodome-sized holes in the system
    // security, we'll check if root is trying to mount the filesystem
    // and refuse if it is.  The somewhat smaller hole of an ordinary
    // user doing it with the allow_other flag is still there because
    // I don't want to parse the options string.
    if ((getuid() == 0) || (geteuid() == 0)) {
		fprintf(stderr, "Running pa4-encfs as root opens unnacceptable security holes\n");
		return 1;
    }

    // Perform some sanity checking on the command line:  make sure
    // there are enough arguments, and that neither of the last two
    // start with a hyphen (this will break if you actually have a
    // rootpoint or mountpoint whose name starts with a hyphen, but so
    // will a zillion other programs)
    if (argc < 4){
		usage();
    }

   	encfs_data->rootdir = realpath(argv[2], NULL);
   	encfs_data->password = argv[1];

	printf("Root Directory: %s \n", argv[2]);
	printf("Mirror: %s \n", argv[argc-2]);
	printf("Key: %s \n", argv[1]);

	//argv[0] = ./pa4-encfs
	argv[1] = argv[3]; //fuse only cares about mount point
	//Setting both of those indexes to null makes argv[] only contain the mountpoint
	argv[3] = NULL; 
	argv[2] = NULL;
	//Since we set those two indexes to null, we want to say there is only two more arguments remaining in argv
	argc = argc - 2;

	return fuse_main(argc, argv, &xmp_oper, encfs_data);
}
