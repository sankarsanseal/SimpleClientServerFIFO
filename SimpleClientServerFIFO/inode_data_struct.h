//
//  inode_data_struct.h
//  SimpleClientServerFIFO
//
//  Created by Sankarsan Seal on 29/07/16.
//  Copyright © 2016 Sankarsan Seal. All rights reserved.
//

#ifndef inode_data_struct_h
#define inode_data_struct_h
#define NO_OF_DATA_BLOCK_OFFSET_IN_INODE 10

typedef struct inode_struct
{
    int type;//0-empty 1-directory 2-regular
    time_t created;
    time_t modified;
    time_t accessed;
    int lock;//0- unlocked 1-locked
    off_t data_block_offset[NO_OF_DATA_BLOCK_OFFSET_IN_INODE];
    
} INODE;

typedef struct data_block_struct
{
    int empty; //1-empty 0- occupied
    int lock; //1-locked 0-unlocked
    off_t data_block_offset;
} DBS;

typedef struct superblock
{
    int remembered_inode;
    int remembered_data_block;
    int no_of_inode;
    int no_of_data_blocks;
    int no_of_directory_entry;
    size_t blocksize;
    size_t filesystemsize;
} SB;

typedef struct dir_struct
{
    int inode;
    char name[32];
} DIRS;

typedef struct free_dir_entry_slot
{
    int i;
    int j;
    
} DIRENTRY;

#endif /* inode_data_struct_h */
