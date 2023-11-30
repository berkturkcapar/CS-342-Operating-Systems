#include <sys/types.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#define MAXFILENAME 256
#define BLOCKSIZE 4096
#define EXT2_NAME_LEN 255
#define EXT2_N_BLOCKS 15

// these structures were obtained from ext2_fs.h file in Linux
struct ext2_super_block {
	__u32	s_inodes_count;		/* Inodes count */
	__u32	s_blocks_count;		/* Blocks count */
	__u32	s_r_blocks_count;	/* Reserved blocks count */
	__u32	s_free_blocks_count;	/* Free blocks count */
	__u32	s_free_inodes_count;	/* Free inodes count */
	__u32	s_first_data_block;	/* First Data Block */
	__u32	s_log_block_size;	/* Block size */
	__u32	s_log_cluster_size;	/* Allocation cluster size */
	__u32	s_blocks_per_group;	/* # Blocks per group */
	__u32	s_clusters_per_group;	/* # Fragments per group */
	__u32	s_inodes_per_group;	/* # Inodes per group */
	__u32	s_mtime;		/* Mount time */
	__u32	s_wtime;		/* Write time */
	__u16	s_mnt_count;		/* Mount count */
	__s16	s_max_mnt_count;	/* Maximal mount count */
	__u16	s_magic;		/* Magic signature */
	__u16	s_state;		/* File system state */
	__u16	s_errors;		/* Behaviour when detecting errors */
	__u16	s_minor_rev_level;	/* minor revision level */
	__u32	s_lastcheck;		/* time of last check */
	__u32	s_checkinterval;	/* max. time between checks */
	__u32	s_creator_os;		/* OS */
	__u32	s_rev_level;		/* Revision level */
	__u16	s_def_resuid;		/* Default uid for reserved blocks */
	__u16	s_def_resgid;		/* Default gid for reserved blocks */
	__u32	s_first_ino;		/* First non-reserved inode */
	__u16   s_inode_size;		/* size of inode structure */
};

struct ext2_group_desc
{
	__u32	bg_block_bitmap;	/* Blocks bitmap block */
	__u32	bg_inode_bitmap;	/* Inodes bitmap block */
	__u32	bg_inode_table;		/* Inodes table block */
	__u16	bg_free_blocks_count;	/* Free blocks count */
	__u16	bg_free_inodes_count;	/* Free inodes count */
	__u16	bg_used_dirs_count;	/* Directories count */
	__u16	bg_flags;
	__u32	bg_exclude_bitmap_lo;	/* Exclude bitmap for snapshots */
	__u16	bg_block_bitmap_csum_lo;/* crc32c(s_uuid+grp_num+bitmap) LSB */
	__u16	bg_inode_bitmap_csum_lo;/* crc32c(s_uuid+grp_num+bitmap) LSB */
	__u16	bg_itable_unused;	/* Unused inodes count */
	__u16	bg_checksum;		/* crc16(s_uuid+group_num+group_desc)*/
};

struct ext2_dir_entry_2 {
	__u32	inode;			/* Inode number */
	__u16	rec_len;		/* Directory entry length */
	__u8	name_len;		/* Name length */
	__u8	file_type;
	char	name[EXT2_NAME_LEN];	/* File name */
};

struct ext2_inode {
	__u16	i_mode;		/* File mode */
	__u16	i_uid;		/* Low 16 bits of Owner Uid */
	__u32	i_size;		/* Size in bytes */
	__u32	i_atime;	/* Access time */
	__u32	i_ctime;	/* Inode change time */
	__u32	i_mtime;	/* Modification time */
	__u32	i_dtime;	/* Deletion Time */
	__u16	i_gid;		/* Low 16 bits of Group Id */
	__u16	i_links_count;	/* Links count */
	__u32	i_blocks;	/* Blocks count */
	__u32	i_flags;	/* File flags */
	union {
		struct {
			__u32	l_i_version; /* was l_i_reserved1 */
		} linux1;
		struct {
			__u32  h_i_translator;
		} hurd1;
	} osd1;				/* OS dependent 1 */
	__u32	i_block[EXT2_N_BLOCKS];/* Pointers to blocks */
	__u32	i_generation;	/* File version (for NFS) */
	__u32	i_file_acl;	/* File ACL */
	__u32	i_size_high;
	__u32	i_faddr;	/* Fragment address */
	union {
		struct {
			__u16	l_i_blocks_hi;
			__u16	l_i_file_acl_high;
			__u16	l_i_uid_high;	/* these 2 fields    */
			__u16	l_i_gid_high;	/* were reserved2[0] */
			__u16	l_i_checksum_lo; /* crc32c(uuid+inum+inode) */
			__u16	l_i_reserved;
		} linux2;
		struct {
			__u8	h_i_frag;	/* Fragment number */
			__u8	h_i_fsize;	/* Fragment size */
			__u16	h_i_mode_high;
			__u16	h_i_uid_high;
			__u16	h_i_gid_high;
			__u32	h_i_author;
		} hurd2;
	} osd2;				/* OS dependent 2 */
};

// structure to keep information 
// about the inodes in the root directory
struct intNode {
	__u32 number;
	char* fileName;
	struct intNode *next;
};

struct intNode *head = NULL;

void insertToList(__u32 newNumber, char* str) {
	struct intNode *newNode = (struct intNode*)malloc(sizeof(struct intNode));
	newNode->next = NULL;
	newNode->number = newNumber;
	newNode->fileName = strdup(str);
	if (head == NULL)
		head = newNode;
	else {
		struct intNode *temp = head;
		while (temp->next != NULL) {
			temp = temp->next;
		}
		temp->next = newNode;
	}
}

// helper function to read inoode with nodeNo
void readInode(int fd, int nodeNo, const struct ext2_group_desc *desc, struct ext2_inode *inode) {
	lseek(fd, ((desc->bg_inode_table) * BLOCKSIZE) + (nodeNo - 1)*sizeof(struct ext2_inode), 
	      SEEK_SET);
	read(fd, inode, sizeof(struct ext2_inode));
}

// helper function to read and print contents of root directory
void readDirectory(int fd, const struct ext2_inode *inode) {
	void *block;
	if (S_ISDIR(inode->i_mode)) {
		struct ext2_dir_entry_2 *dirEntry;
		unsigned int size = 0;
		block = malloc(BLOCKSIZE);
		lseek(fd, (inode->i_block[0]) * BLOCKSIZE, SEEK_SET);
		read(fd, block, BLOCKSIZE);                
		dirEntry = (struct ext2_dir_entry_2 *) block;  
		printf("Printing the contents of the root directory\n");
		while((size < inode->i_size) && dirEntry->inode) {
			char fileName[EXT2_NAME_LEN + 1];
			memcpy(fileName, dirEntry->name, dirEntry->name_len);
			fileName[dirEntry->name_len] = 0;
			printf("%s\n", fileName);
			insertToList(dirEntry->inode, fileName);
			dirEntry = (void*) dirEntry + dirEntry->rec_len;
			size = size + dirEntry->rec_len;
		}
		free(block);
	}
}

int main(int argc, char *argv[]) {
	// double time_spent = 0.0;
    // clock_t begin = clock();

	// get the disk name from commandline
	char devname[MAXFILENAME];
    strcpy(devname, argv[1]);

	struct ext2_super_block superblock;
	struct ext2_group_desc groupDesc;
	struct ext2_inode inode;
	int fd;

	// open the file
	if ((fd = open(devname, O_RDONLY)) < 0) {
		perror(devname);
		exit(1);
	}

	printf("-------------SUPERBLOCK INFORMATION-------------\n");
	// read superblock information which is located at 1024 byte
	lseek(fd, 1024, SEEK_SET); 
	read(fd, &superblock, sizeof(superblock));

	// print superblock information
	printf("Printing superblock information of %s:\n"
	    	"s_inodes_count: %u\n"
			"s_blocks_count: %u\n"
			"s_r_blocks_count: %u\n"
			"s_free_blocks_count: %u\n"
			"s_free_inodes_count: %u\n"
			"s_first_data_block: %u\n"
			"s_log_block_size: %u\n"
			"s_blocks_per_group: %u\n"
			"s_inodes_per_group: %u\n"
			"s_inode_size: %hu\n" ,
			devname, 
			superblock.s_inodes_count,  
			superblock.s_blocks_count,
			superblock.s_r_blocks_count,
			superblock.s_free_blocks_count,
			superblock.s_free_inodes_count,
			superblock.s_first_data_block,
			superblock.s_log_block_size,
			superblock.s_blocks_per_group,
			superblock.s_inodes_per_group,
			superblock.s_inode_size);

	printf("------------CONTENT OF ROOT DIRECTORY------------\n");
	// get group description info
	lseek(fd, BLOCKSIZE, SEEK_SET);
	read(fd, &groupDesc, sizeof(groupDesc));

	// read inode2 which contains info about the root directory
	readInode(fd, 2, &groupDesc, &inode);

	// read root directory info
	readDirectory(fd, &inode);
	
	printf("\n----------------CONTENT OF INODES----------------");
	// print info of inodes in root directory
	struct intNode *temp = head;
	while (temp != NULL) {
		readInode(fd, temp->number, &groupDesc, &inode);
		time_t timestamp1 = inode.i_atime;
		time_t timestamp2 = inode.i_ctime;
		time_t timestamp3 = inode.i_mtime;
		printf("\nPrinting information about inode %u:\n"
			"filename: %s\n"
			"i_mode: %u\n"
			"i_uid: %u\n"
			"i_size: %u\n"
			"i_atime: %s"
			"i_ctime: %s"
			"i_mtime: %s"
			"i_gid : %u\n"
			"i_links_count: %u\n"
			"i_blocks: %u\n"
			"i_flags: %u\n",
			temp->number, 
			temp->fileName,
			inode.i_mode,  
			inode.i_uid,
			inode.i_size,
			asctime(gmtime(&timestamp1)),
			asctime(gmtime(&timestamp2)),
			asctime(gmtime(&timestamp3)),
			inode.i_gid,
			inode.i_links_count,
			inode.i_blocks,
			inode.i_flags);
		temp = temp->next;
	}
	close(fd);
    // clock_t end = clock();
	// time_spent += (double)(end - begin) / CLOCKS_PER_SEC;
    // printf("\nThe elapsed time is %f seconds\n", time_spent);

	exit(0);
}