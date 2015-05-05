// vim: noet:ts=4:sts=4:sw=4:et
#define FUSE_USE_VERSION 26
#define _GNU_SOURCE

#include <assert.h>
#include <endian.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <iconv.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "vfat.h"
#include "util.h"
#include "debugfs.h"

//#define DEBUG_PRINT(...) printf(__VA_ARGS) doesn't compile for some reason
#define DEBUG_PRINT printf

iconv_t iconv_utf16;
char* DEBUGFS_PATH = "/.debug";


static void
vfat_init(const char *dev)
{
    DEBUG_PRINT("vfat_init(1): start of function\n");
    
    struct fat_boot_header s;
    
    uint16_t rootDirSectors;
    uint32_t fatSz, totSec, dataSec, countofClusters;
    uint8_t fat_0;

    iconv_utf16 = iconv_open("utf-8", "utf-16"); // from utf-16 to utf-8
    // These are useful so that we can setup correct permissions in the mounted directories
    vfat_info.mount_uid = getuid();
    vfat_info.mount_gid = getgid();

    // Use mount time as mtime and ctime for the filesystem root entry (e.g. "/")
    vfat_info.mount_time = time(NULL);

    vfat_info.fd = open(dev, O_RDONLY);
    if (vfat_info.fd < 0)
        err(1, "open(%s)", dev);
    if (pread(vfat_info.fd, &s, sizeof(s), 0) != sizeof(s))
        err(1, "read super block");
        
    // Throw error if not FAT32 volume
    // See: http://www.win.tue.nl/~aeb/linux/fs/fat/fat-1.html for the deatils of FAT12, FAT16 and FAT32.
    // See: http://read.pudn.com/downloads77/ebook/294884/FAT32%20Spec%20%28SDA%20Contribution%29.pdf for complete doc from Microsoft

	if(s.root_max_entries != 0) {
		err(1,"[Error] root_max_entries must be 0 in FAT32 volumes.\n");
	}
	rootDirSectors = ((s.root_max_entries * 32) +
		(s.bytes_per_sector - 1)) / s.bytes_per_sector;

	if(s.sectors_per_fat_small != 0){
		fatSz = s.sectors_per_fat_small;
	} else{
		fatSz = vfat_info.sectors_per_fat;
	}

	if(s.total_sectors_small != 0){
		totSec = s.total_sectors_small;
	} else {
		totSec = s.total_sectors;
	}
	
	/* See page 17 of microsoft specs */
	dataSec = totSec - (s.reserved_sectors +
	(s.fat_count * fatSz) + rootDirSectors);
	countofClusters = dataSec / s.sectors_per_cluster;
    
	if(countofClusters < 4085) {
		err(1,"[Error] Volume is FAT12.\n");
	} else if(countofClusters < 65525) {
		err(1,"[Error] Volume is FAT16.\n");
	} else {
		DEBUG_PRINT("Volume looks like FAT32: it has %d clusters\n", countofClusters);
	}

	// Check all other fields
	if(((char*)&s)[510] != 0x55 &&
		((char*)&s)[511] != (char) 0xAA) {
		err(1, "[Error] 510-511 must correspond to signature 0x55aa\n");
	}

	if(s.jmp_boot[0] == 0xEB) {
		if(s.jmp_boot[2] != 0x90) {
			err(1, "[Error] bad jmp_boot[2]\n");
		}
	} else if(s.jmp_boot[0] != 0xE9){
		err(1, "[Error] bad jmp_boot[0]\n");
	}

	if(s.bytes_per_sector != 512 &&
		s.bytes_per_sector != 1024 &&
		s.bytes_per_sector != 2048 &&
		s.bytes_per_sector != 5096) {

		err(1, "[Error] Illegal value for bytes_per_sector\n");
	}

	if(s.sectors_per_cluster != 1 &&
		s.sectors_per_cluster != 2 &&
		s.sectors_per_cluster != 4 &&
		s.sectors_per_cluster != 8 &&
		s.sectors_per_cluster != 16 &&
		s.sectors_per_cluster != 32 &&
		s.sectors_per_cluster != 64 &&
		s.sectors_per_cluster != 128) {
		// Is there a cleverer way of checking? Maybe, it's only 8 am...
		err(1, "[Error] bad sectors_per_cluster.\n");
	}

	if(s.sectors_per_cluster *
		s.bytes_per_sector > 32 * 1024) {
		err(1, "[Error] Out of range bytes_per_sector * sectors_per_cluster\n");
	}

	if(s.reserved_sectors == 0) {
		err(1, "[Error] Reserved_sectors cannot be zero.\n");
	}

	if(s.fat_count < 2) {
		err(1, "[Error] fat_count must be one or two.\n");
	}

	if(s.root_max_entries != 0) {
		err(1, "[Error] root_max_entries must be zero.\n");
	}

	if(s.total_sectors_small != 0) {
		err(1, "[Error] total_sectors_small must be zero.\n");
	}

	if(s.media_info != 0xF0 &&
		s.media_info < 0xF8) {
		err(1, "[Error] Bad media descriptor type.\n");
	}

	if(s.total_sectors == 0) {
		err(1, "[Error] total_sectors must be non-zero\n");
	}
	
	vfat_info.root_cluster = 0xFFFFFFF & s.root_cluster;

	DEBUG_PRINT("Volume is FAT32 for sure.\n");

    vfat_info.root_inode.st_ino = le32toh(s.root_cluster);
    vfat_info.root_inode.st_mode = 0555 | S_IFDIR;
    vfat_info.root_inode.st_nlink = 1;
    vfat_info.root_inode.st_uid = vfat_info.mount_uid;
    vfat_info.root_inode.st_gid = vfat_info.mount_gid;
    vfat_info.root_inode.st_size = 0;
    vfat_info.root_inode.st_atime = vfat_info.root_inode.st_mtime = vfat_info.root_inode.st_ctime = vfat_info.mount_time;
    
    vfat_info.fat = mmap_file(vfat_info.fd, s.reserved_sectors * s.bytes_per_sector, s.sectors_per_fat * s.bytes_per_sector);
    // TODO: do not forget to unmap :)
    
    DEBUG_PRINT("vfat_init(1): end of function\n");

}

/* XXX add your code here */

int vfat_next_cluster(uint32_t c)
{
    DEBUG_PRINT("vfat_next_cluster(1): start of function\n");
    /* TODO: Read FAT to actually get the next cluster */
    
    int next_cluster;
    int first_fat = vfat_info.reserved_sectors * vfat_info.bytes_per_sector;

    if(lseek(vfat_info.fd, first_fat + c * sizeof(uint32_t), SEEK_SET) == -1) {
        err(1, "lseek(%lu)", first_fat + c * sizeof(uint32_t));
    }
	
    if(read(vfat_info.fd, &next_cluster, sizeof(uint32_t)) != sizeof(uint32_t)) {
    	err(1, "read(%lu)",sizeof(uint32_t));
    }
    
    if (next_cluster > 0) {
    	return next_cluster;
    } else {
    	return 0xffffff; // no next cluster	
    }
}

int vfat_readdir(uint32_t first_cluster, fuse_fill_dir_t callback, void *callbackdata)
{
    DEBUG_PRINT("vfat_readdir(3): start of function\n");
    struct stat st; // we can reuse same stat entry over and over again

    memset(&st, 0, sizeof(st));
    st.st_uid = vfat_info.mount_uid;
    st.st_gid = vfat_info.mount_gid;
    st.st_nlink = 1;

    /* XXX add your code here */
    
    bool first_cluster_bool = true;
    int end_of_read = 0;
    bool eof = false;
    uint32_t next_cluster_no = first_cluster;

	while(!eof) {
		end_of_read = read_cluster(next_cluster_no, callback, callbackdata, first_cluster_bool);
		first_cluster = false;

		if(end_of_read == 0) {
			eof = true;
		} else {
			next_cluster_no = 0x0FFFFFFF & vfat_next_cluster(next_cluster_no);
			if(next_cluster_no >= (uint32_t) 0x0FFFFFF8) {
				eof = true;
			}
		}
	}

    return 0;
}


// Used by vfat_search_entry()
struct vfat_search_data {
    const char*  name;
    int          found;
    struct stat* st;
};


// You can use this in vfat_resolve as a callback function for vfat_readdir
// This way you can get the struct stat of the subdirectory/file.
int vfat_search_entry(void *data, const char *name, const struct stat *st, off_t offs)
{
    DEBUG_PRINT("vfat_search_entry(4): start of function\n");
    struct vfat_search_data *sd = data;

    if (strcmp(sd->name, name) != 0) return 0;

    sd->found = 1;
    *sd->st = *st;

    return 1;
}

/**
 * Fills in stat info for a file/directory given the path
 * @path full path to a file, directories separated by slash
 * @st file stat structure
 * @returns 0 iff operation completed succesfully -errno on error
*/
int vfat_resolve(const char *path, struct stat *st)
{
    DEBUG_PRINT("vfat_resolve(2): start of function\n");
    /* TODO: Add your code here.
        You should tokenize the path (by slash separator) and then
        for each token search the directory for the file/dir with that name.
        You may find it useful to use following functions:
        - strtok to tokenize by slash. See manpage
        - vfat_readdir in conjuction with vfat_search_entry
    */
    int res = -ENOENT; // Not Found
    if (strcmp("/", path) == 0) {
        *st = vfat_info.root_inode;
        res = 0;
    }
    return res;
}

// Get file attributes
int vfat_fuse_getattr(const char *path, struct stat *st)
{
    DEBUG_PRINT("vfat_fuse_getattr(2): start of function\n");
    if (strncmp(path, DEBUGFS_PATH, strlen(DEBUGFS_PATH)) == 0) {
        // This is handled by debug virtual filesystem
        return debugfs_fuse_getattr(path + strlen(DEBUGFS_PATH), st);
    } else {
        // Normal file
        return vfat_resolve(path, st);
    }
}

// Extended attributes useful for debugging
int vfat_fuse_getxattr(const char *path, const char* name, char* buf, size_t size)
{
    DEBUG_PRINT("vfat_fuse_getxattr(3): start of function\n");
    struct stat st;
    int ret = vfat_resolve(path, &st);
    if (ret != 0) return ret;
    if (strcmp(name, "debug.cluster") != 0) return -ENODATA;

    if (buf == NULL) {
        ret = snprintf(NULL, 0, "%u", (unsigned int) st.st_ino);
        if (ret < 0) err(1, "WTF?");
        return ret + 1;
    } else {
        ret = snprintf(buf, size, "%u", (unsigned int) st.st_ino);
        if (ret >= size) return -ERANGE;
        return ret;
    }
}

int vfat_fuse_readdir(
        const char *path, void *callback_data,
        fuse_fill_dir_t callback, off_t unused_offs, struct fuse_file_info *unused_fi)
{
    DEBUG_PRINT("vfat_fuse_readdir(5): path = %s\n", path);
    
    if (strncmp(path, DEBUGFS_PATH, strlen(DEBUGFS_PATH)) == 0) {
        // This is handled by debug virtual filesystem
        return debugfs_fuse_readdir(path + strlen(DEBUGFS_PATH), callback_data, callback, unused_offs, unused_fi);
    }
    
    struct stat st;
    if(strcmp(path, "/") != 0) {
        vfat_resolve(path+1, &st);
        vfat_readdir((uint32_t)st.st_ino, callback, callback_data);
    } else {
        vfat_readdir(vfat_info.root_cluster, callback, callback_data);
    }
    return 0;
}

int vfat_fuse_read(
        const char *path, char *buf, size_t size, off_t offs,
        struct fuse_file_info *unused)
{
    DEBUG_PRINT("vfat_fuse_read(4): start of function\n");
    if (strncmp(path, DEBUGFS_PATH, strlen(DEBUGFS_PATH)) == 0) {
        // This is handled by debug virtual filesystem
        return debugfs_fuse_read(path + strlen(DEBUGFS_PATH), buf, size, offs, unused);
    }
    /* TODO: Add your code here. Look at debugfs_fuse_read for example interaction.
    */
    return 0;
}

////////////// No need to modify anything below this point
int
vfat_opt_args(void *data, const char *arg, int key, struct fuse_args *oargs)
{
    DEBUG_PRINT("vfat_opt_args(4): start of function\n");
    if (key == FUSE_OPT_KEY_NONOPT && !vfat_info.dev) {
        vfat_info.dev = strdup(arg);
        return (0);
    }
    return (1);
}

struct fuse_operations vfat_available_ops = {
    .getattr = vfat_fuse_getattr,
    .getxattr = vfat_fuse_getxattr,
    .readdir = vfat_fuse_readdir,
    .read = vfat_fuse_read,
};

int main(int argc, char **argv)
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    fuse_opt_parse(&args, NULL, NULL, vfat_opt_args);

    if (!vfat_info.dev)
        errx(1, "missing file system parameter");

    vfat_init(vfat_info.dev);
    return (fuse_main(args.argc, args.argv, &vfat_available_ops, NULL));
}
