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
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/mman.h>
#include <time.h>
#include <signal.h>

#include "vfat.h"
#include "util.h"
#include "debugfs.h"

//#define DEBUG_PRINT(...) printf(__VA_ARGS) doesn't compile for some reason
#define DEBUG_PRINT printf
#define MAX_NAME_SIZE (13 * 0x14)

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define VFAT_ATTR_LFN 0xf
#define ATTR_LONG_NAME (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)
#define VFAT_ATTR_INVAL (0x80|0x40|0x08)


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

void seek_cluster(uint32_t cluster_no) {
    if(cluster_no < 2) {
	err(1, "cluster number < 2");
    }

    uint32_t firstDataSector = vfat_info.reserved_sectors +
	(vfat_info.fat.fat_count * vfat_info.sectors_per_fat); // todo check
    uint32_t firstSectorofCluster = ((cluster_no - 2) * vfat_info.sectors_per_cluster) + firstDataSector;
    if(lseek(vfat_info.fd, firstSectorofCluster * vfat_info.bytes_per_sector, SEEK_SET) == -1) {
	err(1, "lseek cluster_no %d\n", cluster_no);
    }
}

unsigned char
chkSum (unsigned char *pFcbName) {
	short fcbNameLen;
	unsigned char sum;

	sum = 0;
	for (fcbNameLen=11; fcbNameLen!=0; fcbNameLen--) {
		// NOTE: The operation is an unsigned char rotate right
		sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *pFcbName++;
	}
	return (sum);
}

time_t
conv_time(uint16_t date_entry, uint16_t time_entry) {
	struct tm * time_info;
	time_t raw_time;

	time(&raw_time);
	time_info = localtime(&raw_time);
	time_info->tm_sec = (time_entry & 0x1f) << 1;
	time_info->tm_min = (time_entry & 0x1E0) >> 5;
	time_info->tm_hour = (time_entry & 0xFE00) >> 11;
	time_info->tm_mday = date_entry & 0x1F;
	time_info->tm_mon = ((date_entry & 0x1E0) >> 5) - 1;
	time_info->tm_year = ((date_entry & 0xFE00) >> 9) + 80;
	return mktime(time_info);
}

void
setStat(struct fat32_direntry dir_entry, char* buffer, fuse_fill_dir_t filler, void *fillerdata, uint32_t cluster_no){
	struct stat* stat_str = malloc(sizeof(struct stat));
			memset(stat_str, 0, sizeof(struct stat));
			stat_str->st_dev = 0; // Ignored by FUSE
			stat_str->st_ino = cluster_no; // Ignored by FUSE unless overridden
			if((dir_entry.attr & ATTR_READ_ONLY) == ATTR_READ_ONLY){
				stat_str->st_mode = S_IRUSR | S_IRGRP | S_IROTH;
			}
			else{
				stat_str->st_mode = S_IRWXU | S_IRWXG | S_IRWXO;
			}
			if((dir_entry.attr & ATTR_DIRECTORY) == ATTR_DIRECTORY) {
				stat_str->st_mode |= S_IFDIR;
				int cnt = 0;
				uint32_t next_cluster_no = cluster_no;
				off_t pos = lseek(vfat_info.fd, 0, SEEK_CUR);
				while(next_cluster_no < (uint32_t) 0x0FFFFFF8) {
					cnt++;
					next_cluster_no = next_cluster(0x0FFFFFFF & next_cluster_no);
				}
				if(lseek(vfat_info.fd, pos, SEEK_SET) == -1) {
					err(1, "Couldn't return to initial position: %lx", pos);
				}
				stat_str->st_size = cnt * vfat_info.sectors_per_cluster*vfat_info.bytes_per_sector;
			}
			else {
				stat_str->st_mode |= S_IFREG;
				stat_str->st_size = dir_entry.size;
			}
			stat_str->st_nlink = 1;
			stat_str->st_uid = mount_uid;
			stat_str->st_gid = mount_gid;
			stat_str->st_rdev = 0;
			stat_str->st_blksize = 0; // Ignored by FUSE
			stat_str->st_blocks = 1;
			stat_str->st_atime = conv_time(dir_entry.atime_date, 0);
			stat_str->st_mtime = conv_time(dir_entry.mtime_date, dir_entry.mtime_time);
			stat_str->st_ctime = conv_time(dir_entry.ctime_date, dir_entry.ctime_time);
			filler(fillerdata, buffer, stat_str, 0);
			free(stat_str);
}

char*
getfilename(char* nameext, char* filename) {
	if(nameext[0] == 0x20) {
	    err(1, "filename[0] is 0x20");
	}

	uint32_t fileNameCnt = 0;
	bool before_extension = true;
	bool in_spaces = false;
	bool in_extension = false;
	
	int i = 0;
	for(i; i < 11; i++) {
		if(nameext[i] < 0x20 ||
			nameext[i] == 0x22 ||
			nameext[i] == 0x2A ||
			nameext[i] == 0x2B ||
			nameext[i] == 0x2C ||
			nameext[i] == 0x2E ||
			nameext[i] == 0x2F ||
			nameext[i] == 0x3A ||
			nameext[i] == 0x3B ||
			nameext[i] == 0x3C ||
			nameext[i] == 0x3D ||
			nameext[i] == 0x3E ||
			nameext[i] == 0x3F ||
			nameext[i] == 0x5B ||
			nameext[i] == 0x5C ||
			nameext[i] == 0x5D ||
			nameext[i] == 0x7C) {

			err(1, "invalid character in filename %x at %d\n", nameext[i] & 0xFF, i);
		}

		if(before_extension) {
		    if(nameext[i] == 0x20) {
			before_extension = false;
			in_spaces = true;
			filename[fileNameCnt++] = '.';
		    } else if(i == 8) {
			before_extension = false;
			in_spaces = true;
			filename[fileNameCnt++] = '.';
			filename[fileNameCnt++] = nameext[i];
			in_extension = true;
		    } else {
			filename[fileNameCnt++] = nameext[i];
		    }
		} else if(in_spaces) {
			if(nameext[i] != 0x20) {
			    in_spaces = false;
			    in_extension = true;
			    filename[fileNameCnt++] = nameext[i];
			}
		} else if(in_extension) {
			if(nameext[i] == 0x20) {
			    break;
			} else {
			    filename[fileNameCnt++] = nameext[i];
			}
		}
	}

	if(filename[fileNameCnt - 1] == '.') {
	    filename--;
	}
	filename[fileNameCnt] = '\0';
	return filename;
}

static int
read_cluster(uint32_t cluster_no, fuse_fill_dir_t filler, void *fillerdata,bool first_cluster) {
	// todo change
	uint8_t check_sum = '\0';
	char* buffer = calloc(MAX_NAME_SIZE*2, sizeof(char)); // Max size of name: 13 * 0x14 = 260
	char* char_buffer = calloc(MAX_NAME_SIZE, sizeof(char));
	int i, j, seq_nb = 0;
	size_t in_byte_size = 2 * MAX_NAME_SIZE, out_byte_size = MAX_NAME_SIZE;
	struct fat32_direntry short_entry;
	struct fat32_direntry_long long_entry;
	memset(buffer, 0, 2*MAX_NAME_SIZE);

	seek_cluster(cluster_no);

	for(i = 0; i < vfat_info.sectors_per_cluster * vfat_info.bytes_per_sector; i+=32) {
		if(read(vfat_info.fd, &short_entry, 32) != 32){
			err(1, "read(short_dir)");
		}

		if(i < 64 && first_cluster && cluster_no != 2){
			char* filename = (i == 0) ? "." : "..";
			setStat(short_entry,filename,filler,fillerdata,
				(((uint32_t)short_entry.cluster_hi) << 16) | ((uint32_t)short_entry.cluster_lo));

			continue;
		}

		if(((uint8_t) short_entry.nameext[0]) == 0xE5){
			continue;
		} else if(short_entry.nameext[0] == 0x00) {
			free(buffer);
			free(char_buffer);
			return 0;
		} else if(short_entry.nameext[0] == 0x05) {
			short_entry.nameext[0] = (char) 0xE5;
		}

		if((short_entry.attr & ATTR_LONG_NAME) == ATTR_LONG_NAME) {
			long_entry = *((struct fat32_direntry_long *) &short_entry);
			if((long_entry.seq & 0x40) == 0x40) {
				seq_nb = (long_entry.seq & 0x3f) - 1;
				check_sum = long_entry.csum;

				for(j = 0; j < 13; j++) {
					if(j < 5 && long_entry.name1[j] != 0xFFFF) {
						buffer[j*2] = long_entry.name1[j];
						buffer[j*2+1] = long_entry.name1[j] >> 8;
					} else if(j < 11 && long_entry.name2[j - 5] != 0xFFFF) {
						buffer[j*2] = long_entry.name2[j-5];
						buffer[j*2+1] = long_entry.name2[j-5] >> 8;
					} else if(j < 13 && long_entry.name3[j - 11] != 0xFFFF) {
						buffer[j*2] = long_entry.name3[j-11];
						buffer[j*2+1] = long_entry.name3[j-11] >> 8;
					}
				}
			} else if (check_sum == long_entry.csum  && long_entry.seq == seq_nb) {
				seq_nb -= 1;

				char tmp[MAX_NAME_SIZE*2];
				memset(tmp, 0, MAX_NAME_SIZE*2);

				for(j = 0; j < MAX_NAME_SIZE*2; j++) {
					tmp[j] = buffer[j];
				}

				memset(buffer, 0, MAX_NAME_SIZE*2);

				for(j = 0; j < MAX_NAME_SIZE; j++) {
					if(j < 5 && long_entry.name1[j] != 0xFFFF) {
						buffer[j*2] = long_entry.name1[j];
						buffer[j*2+1] = long_entry.name1[j] >> 8;
					} else if(j < 11 && long_entry.name2[j - 5] != 0xFFFF) {
						buffer[j*2] = long_entry.name2[j - 5];
						buffer[j*2+1] = long_entry.name2[j - 5] >> 8;
					} else if(j < 13 && long_entry.name3[j - 11] != 0xFFFF) {
						buffer[j*2] = long_entry.name3[j - 11];
						buffer[j*2+1] = long_entry.name3[j - 11] >> 8;
					} else if(j >= 13 && ((uint16_t*)tmp)[j - 13] != 0xFFFF){
						buffer[j*2] = tmp[(j - 13)*2];
						buffer[j*2+1] = tmp[(j - 13)*2+1];
					}
				}
			} else {
				seq_nb = 0;
				check_sum = '\0';
				memset(buffer, 0, MAX_NAME_SIZE*2);
				err(1, "error: Bad sequence number or checksum\n");
			}
		} else if((short_entry.attr & ATTR_VOLUME_ID) == ATTR_VOLUME_ID) {
			seq_nb = 0;
			check_sum = '\0';
			memset(buffer, 0, MAX_NAME_SIZE*2);
		} else if(check_sum == chkSum((unsigned char *) &(short_entry.nameext)) && seq_nb == 0) {
			char* buffer_pointer = buffer;
			char* char_buffer_pointer = char_buffer;
			iconv(iconv_utf16, &buffer_pointer, &in_byte_size, &char_buffer_pointer, &out_byte_size);
			in_byte_size = MAX_NAME_SIZE*2;
			out_byte_size = MAX_NAME_SIZE;
			char *filename = char_buffer;
			setStat(short_entry,filename,filler,fillerdata,
				(((uint32_t)short_entry.cluster_hi) << 16) | ((uint32_t)short_entry.cluster_lo));
			check_sum = '\0';
			memset(buffer, 0, MAX_NAME_SIZE);
		} else {
			char *filename = char_buffer;
			getfilename(short_entry.nameext, filename);
			setStat(short_entry,filename,filler,fillerdata,
				(((uint32_t)short_entry.cluster_hi) << 16) | ((uint32_t)short_entry.cluster_lo));
		}
	}

	free(buffer);
	free(char_buffer);
	return 1;
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
