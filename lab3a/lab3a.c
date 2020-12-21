// NAME: Haley Kim,Shri Narayana                                                         
// EMAIL: haleykim@g.ucla.edu,shrinikethn@ucla.edu                                       
// ID: 405111152,505313060

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include "ext2_fs.h"

#define SUPER_OFFSET 1024

// Definition of Variables
int disk_fd; 
struct ext2_super_block super_block;
struct ext2_group_desc group_desc;
unsigned int block_size;
char* bytes_block;
char* bytes_inode;
int* is_inode_allocated;
struct ext2_inode* inodes;
struct ext2_inode temp_inode;

void read_superblock() {
	pread(disk_fd, &super_block, sizeof(super_block), SUPER_OFFSET);
  block_size = SUPER_OFFSET << super_block.s_log_block_size;
}

void read_group_desc() {
	pread(disk_fd, &group_desc, sizeof(group_desc), SUPER_OFFSET + block_size);
}

void print_super_block() {
	fprintf(stdout, "SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n", 
                super_block.s_blocks_count, // total number of blocks
                super_block.s_inodes_count, // total number of i-nodes
                block_size, // block size
                super_block.s_inode_size, // i-node size
                super_block.s_blocks_per_group, // blocks per group
                super_block.s_inodes_per_group, // i-nodes per group
                super_block.s_first_ino // first non-reserved i-node
  );
}

void print_group_desc() {
  int num_blocks_in_group = super_block.s_blocks_count % super_block.s_blocks_per_group;
  fprintf(stdout, "GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n", 
                0, // group number
                //super_block.s_blocks_per_group, // total number of blocks in this group
                num_blocks_in_group, // total number of blocks in this group
                super_block.s_inodes_count, // total number of i-nodes in this group
                group_desc.bg_free_blocks_count, // number of free blocks
                group_desc.bg_free_inodes_count, // number of free i-nodes
                group_desc.bg_block_bitmap, // block number of free block bitmap for this group
                group_desc.bg_inode_bitmap, // block number of free i-node bitmap for this group
                group_desc.bg_inode_table // block number of first block of i-nodes in this group
  );
}

void free_blocks() {
  bytes_block = (char*)malloc(block_size);
  // read in entire block
  pread(disk_fd, bytes_block, block_size, SUPER_OFFSET + 2*block_size);
  int curr_block_num = super_block.s_first_data_block; // block number of first data block
  // calculate how many bytes_block to read, since entire block might not be full
  int num_bytes_to_read;
  if (super_block.s_blocks_count == 0)
    num_bytes_to_read = 0;
  else 
    num_bytes_to_read = 1+((super_block.s_blocks_count-1)/8);
  // iterate through each byte of bitmap
  for (int i = 0; i < num_bytes_to_read; i++) {
    char bit = bytes_block[i];
    // iterate through each bit of byte
    for (int j = 0; j < 8; j++) {
      // checking whether the bits in this byte is part of bitmap
      if ((unsigned int)((i * 8) + j) >= super_block.s_blocks_count)
        return;
      // check rightmost bit first, right shift each time
      unsigned int used = 1 & bit;
      if (!used) {
        fprintf(stdout, "BFREE,%d\n", curr_block_num);
      }
      bit >>= 1;
      curr_block_num++; // block number incremented with each bitmap bit
    }
  }
}

void free_inodes() {
  is_inode_allocated = (int*)malloc(sizeof(int)*super_block.s_inodes_count);
  bytes_inode = (char*)malloc(block_size);
  // read in entire block
  pread(disk_fd, bytes_inode, block_size, SUPER_OFFSET + 3*block_size);
  int curr_inode_num = 1; // inode number of first inode
  // calculate how many bytes to read, since entire block might not be full
  int num_bytes_to_read;
  if (super_block.s_inodes_count == 0)
    num_bytes_to_read = 0;
  else 
    num_bytes_to_read = 1+((super_block.s_inodes_count-1)/8);
  // iterate through each byte of bitmap
  for (int i = 0; i < num_bytes_to_read; i++) {
    char bit = bytes_inode[i];
    // iterate through each bit of byte
    for (int j = 0; j < 8; j++) {
      // checking whether the bits in this byte is part of bitmap
      if ((unsigned int)((i*8) + j) >= super_block.s_inodes_count)
        return;
      // check rightmost bit first, right shift each time
      unsigned int used = 1 & bit;
      is_inode_allocated[curr_inode_num-1] = 1; 
      if (!used) {
        is_inode_allocated[curr_inode_num-1] = 0; 
        fprintf(stdout, "IFREE,%d\n", curr_inode_num);
      }
      bit >>= 1;
      curr_inode_num++; // inode number incremented with each bitmap bit
    }
  }
}

void read_directory(int inode_num, __u32 curr_block) {
  if (curr_block == 0) {
    return;
  }
  //__u32 curr_block = inode.i_block[i]; // in order to calculate offset
  unsigned int offset = SUPER_OFFSET + (curr_block-1)*block_size; // this is how we've been calculating offset so far
  unsigned int curr_offset = 0; // this is the offset WITHIN this block
  struct ext2_dir_entry dir_entry; // this is where we read in data
  while (curr_offset < block_size) { // parsing through linked list structure
    memset(dir_entry.name, 0, 256);
    pread(disk_fd, &dir_entry, sizeof(dir_entry), offset + curr_offset);
    if (dir_entry.inode != 0) { // if this directory entry is valid
      // do a bunch of fprintfs
      fprintf(stdout, "DIRENT,%d,%d,%d,%d,%d,'%s'\n",
                    inode_num, // parent inode number
                    curr_offset, // logical byte offset
                    dir_entry.inode, // inode number of referenced file
                    dir_entry.rec_len, // entry length
                    dir_entry.name_len, // name length
                    dir_entry.name // name
      );
    }
    curr_offset += dir_entry.rec_len; // increment offset WITHIN this block by how long this dir_entry is
  }
}

void read_indirect(struct ext2_inode inode, int inode_num) {
  if (inode.i_block[12] != 0) {
    // reading in block of pointers
    __u32 *block_ptrs = malloc(block_size);
    __u32 num_ptrs = block_size / sizeof(__u32);
    unsigned int indirect1_offset = SUPER_OFFSET + (inode.i_block[12]-1)*block_size;
    pread(disk_fd, block_ptrs, block_size, indirect1_offset);
    // iterating through each pointer
    for (unsigned int i = 0; i < num_ptrs; i++) {
      if (block_ptrs[i] != 0) {
        //read_directory(inode_num, block_ptrs[i]);
        fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                    inode_num, // I-node number of the owning file
                    1, // level of indirection
                    12+i, // logical block offset
                    inode.i_block[12], // block number
                    block_ptrs[i] // block number of the referenced block
        );
      }
    }
    free(block_ptrs);
  }
}

void read_doubly_indirect(struct ext2_inode inode, int inode_num) {
  if (inode.i_block[13] != 0) {
    __u32 *indir_block_ptrs = malloc(block_size);
    __u32 num_ptrs = block_size / sizeof(__u32);
    unsigned int indirect2_offset = SUPER_OFFSET + (inode.i_block[13]-1)*block_size;
    pread(disk_fd, indir_block_ptrs, block_size, indirect2_offset);
    // iterating through each indirect pointer
    for (unsigned int i = 0; i < num_ptrs; i++) {
      if (indir_block_ptrs[i] != 0) {
        fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                    inode_num, // I-node number of the owning file
                    2, // level of indirection
                    256+12+i, // logical block offset
                    inode.i_block[13], // block number
                    indir_block_ptrs[i] // block number of the referenced block
        );
        // reading in block of pointers
        __u32 *block_ptrs = malloc(block_size);
        unsigned int indirect1_offset = SUPER_OFFSET + (indir_block_ptrs[i]-1)*block_size;
        pread(disk_fd, block_ptrs, block_size, indirect1_offset);
        // iterating through each pointer
        for (unsigned int j = 0; j < num_ptrs; j++) {
          if (block_ptrs[j] != 0) {
            //read_directory(inode_num, block_ptrs[j]);
            fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                        inode_num, // I-node number of the owning file
                        1, // level of indirection
                        256+12+j, // logical block offset
                        indir_block_ptrs[i], // block number
                        block_ptrs[j] // block number of the referenced block
            );
          }
        }
        free(block_ptrs);
      }
    }
    free(indir_block_ptrs);
  }
}

void read_triply_indirect(struct ext2_inode inode, int inode_num) {
  if (inode.i_block[14] != 0) {
    __u32 *indir2_block_ptrs = malloc(block_size);
    __u32 num_ptrs = block_size / sizeof(__u32);
    unsigned int indirect3_offset = SUPER_OFFSET + (inode.i_block[14]-1)*block_size;
    pread(disk_fd, indir2_block_ptrs, block_size,indirect3_offset);
    for (unsigned int i = 0; i < num_ptrs; i++) {
      if (indir2_block_ptrs[i] != 0) {
        fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                      inode_num, //inode number
                      3, //level of indirection
                      65536 + 256 + 12 + i, //logical block offset
                      inode.i_block[14], //block number of indirect block being scanned
                      indir2_block_ptrs[i] //block number of reference block
        );
        __u32 *indir_block_ptrs = malloc(block_size);
        unsigned int indirect2_offset = SUPER_OFFSET + (indir2_block_ptrs[i]-1)*block_size;
        pread(disk_fd, indir_block_ptrs, block_size, indirect2_offset);
        for (unsigned int j = 0; j < num_ptrs; j++) {
          if (indir_block_ptrs[j] != 0) {
            fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                        inode_num, //inode number
                        2, //level of indirection
                        65536 + 256 + 12 + j, //logical block offset
                        indir2_block_ptrs[i], //block number of indirect block being scanned
                        indir_block_ptrs[j] //block number of reference block	
            );
            __u32 *block_ptrs = malloc(block_size);
            unsigned int indirect1_offset = SUPER_OFFSET + (indir_block_ptrs[j]-1)*block_size;
            pread(disk_fd, block_ptrs, block_size, indirect1_offset);
            for (unsigned int k = 0; k < num_ptrs; k++) {
              if (block_ptrs[k] != 0) {
                //read_directory(inode_num, block_ptrs[k]);
                fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                            inode_num, //inode number
                            1, //level of indirection
                            65536 + 256 + 12 + k, //logical block offset
                            indir_block_ptrs[j], //block number of indirect block being scanned
                            block_ptrs[k] //block number of reference block	
                );
              }
            }
            free(block_ptrs);
          }
        }
        free(indir_block_ptrs);
      }
    }
    free(indir2_block_ptrs);
  }
}

void read_inodes() {
  inodes = malloc(sizeof(temp_inode)*super_block.s_inodes_count);
  pread(disk_fd, inodes, sizeof(temp_inode)*super_block.s_inodes_count, SUPER_OFFSET + 4*block_size);
  for (unsigned int i = 0; i < super_block.s_inodes_count; i++) {
    if(is_inode_allocated[i]) {
      // determine file type and assign char
      char file_type = '?';
      if ((inodes[i].i_mode & 0xA000) == 0xA000)
        file_type = 's';
      else if ((inodes[i].i_mode & 0x8000) == 0x8000)
        file_type = 'f';
      else if ((inodes[i].i_mode & 0x4000) == 0x4000)
        file_type = 'd';
      if ((inodes[i].i_mode == 0) || (inodes[i].i_links_count == 0))
        continue;
      fprintf(stdout, "INODE,%d,%c,%o,%d,%d,%d,", 
                  i+1, // inode number
                  file_type, // file type ('f' for file, 'd' for directory, 's' for symbolic link, '?" for anything else)
                  inodes[i].i_mode & 0xfff, // number of free blocks
                  inodes[i].i_uid, //owner
                  inodes[i].i_gid, // block number of free block bitmap for this group
                  inodes[i].i_links_count // block number of free i-node bitmap for this group
      );
      
      time_t time_c = (time_t)(inodes[i].i_ctime);
      struct tm* time_c_format;
      time_c_format = gmtime(&time_c);
      char c_time[20];
      strftime(c_time, 80, "%m/%d/%y %H:%M:%S,", time_c_format);
      
      time_t time_m = (time_t)(inodes[i].i_mtime);
      struct tm* time_m_format;
      time_m_format = gmtime(&time_m);
      char m_time[20];
      strftime(m_time, 80, "%m/%d/%y %H:%M:%S,", time_m_format);
      
      time_t time_a = (time_t)(inodes[i].i_atime);
      struct tm* time_a_format;
      time_a_format = gmtime(&time_a);
      char a_time[20];
      strftime(a_time, 80, "%m/%d/%y %H:%M:%S,", time_a_format);
      
      // print ctime, mtime, atime her      
      fprintf(stdout, "%s", c_time);
      fprintf(stdout, "%s", m_time);
      fprintf(stdout, "%s", a_time);      
      
      fprintf(stdout, "%d,%d",
                    inodes[i].i_size, // file size
                    inodes[i].i_blocks // number of (512 byte) blocks of disk space (decimal) taken up by this file
      );
      
      if (file_type == 'f' || file_type == 'd') {
        for(int j = 0; j < 15; j++) {
          fprintf(stdout, ",%d",
                    inodes[i].i_block[j] // file size
          );
        }
        fprintf(stdout, "\n");
      }
      
      // if this is a symbolic link
      else if (file_type == 's') {
        if (inodes[i].i_size <= 60) {}
          // do nothing
        else {
          for(int j = 0; j < 15; j++) {
            fprintf(stdout, ",%d",
                      inodes[i].i_block[j] // file size
            );
          }
        }
        fprintf(stdout, "\n");
      }
      else {
        fprintf(stdout, "\n");        
      }
      // if this is a directory, new output line
      if (file_type == 'd') {
        for (int j = 0; j < 12; j++) {
          if (inodes[i].i_block[j] != 0) {
            read_directory(i+1, inodes[i].i_block[j]);  
          }
        }
      }
      if ((file_type == 'd') || (file_type == 'f')) {
        read_indirect(inodes[i], i+1);
        read_doubly_indirect(inodes[i], i+1);
        read_triply_indirect(inodes[i], i+1);
      }
    }
  }
}

int main(int argc, char *argv[]) {
	// Checking Correct Number of Arguments
  if (argc != 2) {
    fprintf(stderr, "Error: incorrect number of arguments\n");
    exit(1);
  }
  
  // Opening Disk Image for pread()
	disk_fd = open(argv[1], O_RDONLY);
  if (disk_fd == -1) {
    fprintf(stderr, "Error: %s, could not open file\n", strerror(errno));
    exit(1);
  } 
  
  read_superblock();
  print_super_block();
	
  read_group_desc();
  print_group_desc();
  
  free_blocks();
  free_inodes();
  
  read_inodes();
  
  free(bytes_block);
  free(bytes_inode);
  free(inodes);
}
