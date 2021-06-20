#include "simplefs-ops.h"
extern struct filehandle_t file_handle_array[MAX_OPEN_FILES]; // Array for storing opened files

int simplefs_create(char *filename){
    /*
	    Create file with name `filename` from disk
	*/
	// search file
	int inodeNum = search_file(filename);

	if (inodeNum < 0)	{
		// allocate inode
		inodeNum = simplefs_allocInode();

		// if allocation is successful
		if (inodeNum > 0) {
			struct inode_t *inodePtr = (struct inode_t *)malloc(sizeof(struct inode_t));
			
			// read allocated inode
			simplefs_readInode(inodeNum, inodePtr);

			// write filename to the allocated inode
			memcpy(inodePtr->name, filename, MAX_NAME_STRLEN);
			// set inode status to in_use
			inodePtr->status = INODE_IN_USE;		

			// write filename and status to inode 
			simplefs_writeInode(inodeNum, inodePtr);

			// free inodePtr
			free(inodePtr);
		}
		// unsuccessful inode allocation
		else
			return -1;
	}
	// file already exists with name `filename`
	else
		return -1;

	return inodeNum;
}


void simplefs_delete(char *filename){
    /*
	    delete file with name `filename` from disk
	*/
	// search file and get its inode number
	int inodeNum = search_file(filename);

	if (inodeNum > 0)	{
		// get inode information of the inode number obtained
		struct inode_t *inode = (struct inode_t *)malloc(sizeof(struct inode_t));
		simplefs_readInode(inodeNum, inode);

		// set status as free and file size as 0
		inode->status = INODE_FREE;
		inode->file_size = 0;

		// superblock stores information about inodes and datablocks if they are free or in use
		struct superblock_t *superblock = (struct superblock_t *)malloc(sizeof(struct superblock_t));
		simplefs_readSuperBlock(superblock);

		for (int i = 0; i < MAX_FILE_SIZE; i++)	{
			// free all the datablocks in use and update the direct_blocks and datablock_freelist
			if (inode->direct_blocks[i] > 0) {
				int block_Num = inode->direct_blocks[i];
				assert(superblock->datablock_freelist[block_Num] == DATA_BLOCK_USED);
				superblock->datablock_freelist[block_Num] = DATA_BLOCK_FREE;
				inode->direct_blocks[i] = -1;
			}
		}

		// add the inode num to the free list
		assert(superblock->inode_freelist[inodeNum] == INODE_IN_USE);
		superblock->inode_freelist[inodeNum] = INODE_FREE;

		// update the superblock and inode struct
		simplefs_writeSuperBlock(superblock);
		simplefs_writeInode(inodeNum, inode);

		// free the allocation
		free(superblock);
		free(inode);
	}
}

int simplefs_open(char *filename){
    /*
	    open file with name `filename`
	*/
	// search file and get its inode number
	int inodeNum = search_file(filename);

	if (inodeNum > 0) {
		// if the inode number of the file is already in the file handle array, return that entry index
		for (int i = 0; i < MAX_OPEN_FILES; i++) {
			if (file_handle_array[i].inode_number == inodeNum)
				return i;
		}

		// if the file is accessed for the first time, allocate an unused file handle entry
		for (int i = 0; i < MAX_OPEN_FILES; i++) {
			if (file_handle_array[i].inode_number < 0) {
				file_handle_array[i].inode_number = inodeNum;
				return i;
			}
		}
	}
	// if file does not exist
	else
		return -1;

    return -1;
}

void simplefs_close(int fileHandle){
    /*
	    close file pointed by `fileHandle`
	*/
	// check if index is not out of range and it file is not already closed
	assert(0 <= fileHandle && fileHandle < MAX_OPEN_FILES);
	assert(file_handle_array[fileHandle].inode_number != -1);

	// close the file, set it's inode_number in file_handle_array to -1 and offset to 0
	file_handle_array[fileHandle].inode_number = -1;
	file_handle_array[fileHandle].offset = 0;
}

int simplefs_read(int fileHandle, char *buf, int nbytes){
    /*
	    read `nbytes` of data into `buf` from file pointed by `fileHandle` starting at current offset
	*/
	// check if the input data is in range
	assert(0 <= fileHandle && fileHandle <= MAX_OPEN_FILES);
	assert(nbytes > 0);

	// get inode number and offset of the file
	int fileInode = file_handle_array[fileHandle].inode_number;
	int fileOffset = file_handle_array[fileHandle].offset;

	struct inode_t *inode = (struct inode_t *)malloc(sizeof(struct inode_t));
	simplefs_readInode(fileInode, inode);
	assert(fileOffset + nbytes <= inode->file_size);

	// calculate start index and offset of the datablock
	int startBlockIndex = fileOffset / BLOCKSIZE;
	int startBlockOffset = fileOffset % BLOCKSIZE;
	int startBlockCapacity = BLOCKSIZE - startBlockOffset;
	
	// calculate no of datablocks required to read nbytes
	int requiredDatablocks = 1;
	if (nbytes > startBlockCapacity) {
		int left = nbytes - startBlockCapacity;
		requiredDatablocks += left / BLOCKSIZE;

		if (left % BLOCKSIZE > 0)
			++requiredDatablocks;
	}

	// Find the required data blocks
	int allocDatablock[requiredDatablocks];
	int allocIndex = 0;
	for (int i = startBlockIndex; allocIndex < requiredDatablocks; ++i)
		allocDatablock[allocIndex++] = inode->direct_blocks[i];

	// read data blocks
	char tempBuf[BLOCKSIZE];
	int curr_dataBlock = inode->direct_blocks[startBlockIndex];
	int curr_readSize = min(nbytes, startBlockCapacity);

	simplefs_readDataBlock(curr_dataBlock, tempBuf);
	bzero(tempBuf + curr_readSize, startBlockCapacity - curr_readSize);
	sprintf(buf, "%s", tempBuf + startBlockOffset);
	buf += curr_readSize;
	nbytes -= curr_readSize;

	while (nbytes > 0) {
		curr_dataBlock = inode->direct_blocks[++startBlockIndex];
		curr_readSize = min(nbytes, BLOCKSIZE);

		simplefs_readDataBlock(curr_dataBlock, tempBuf);
		if (curr_readSize < BLOCKSIZE)
			bzero(tempBuf + curr_readSize, BLOCKSIZE - curr_readSize);
		
		sprintf(buf, "%s", tempBuf);
		buf += curr_readSize;
		nbytes -= curr_readSize;
	}

	if (nbytes > 0)
		return -1;
	*buf = '\0';

	free(inode);

	return 0;
}


int simplefs_write(int fileHandle, char *buf, int nbytes){
    /*
	    write `nbytes` of data from `buf` to file pointed by `fileHandle` starting at current offset
	*/

	assert(nbytes > 0);

	//Check overflow
	int fileInode = file_handle_array[fileHandle].inode_number;
	int fileOffset = file_handle_array[fileHandle].offset;

	assert(fileOffset + nbytes <= MAX_FILE_SIZE * BLOCKSIZE);

	//Calculate number of data blocks required
	int startBlockIndex = fileOffset / BLOCKSIZE;
	int startBlockOffset = fileOffset % BLOCKSIZE;
	int startBlockCapacity = BLOCKSIZE - startBlockOffset;
	int requiredDatablocks = 1;

	if (nbytes > startBlockCapacity) {
		int left = nbytes - startBlockCapacity;
		requiredDatablocks += left / BLOCKSIZE;

		if (left % BLOCKSIZE > 0)
			++requiredDatablocks;
	}

	//Allocate required number of data blocks
	struct superblock_t *superblock = (struct superblock_t *)malloc(sizeof(struct superblock_t));
	struct inode_t *inode = (struct inode_t *)malloc(sizeof(struct inode_t));

	simplefs_readSuperBlock(superblock);
	simplefs_readInode(fileInode, inode);

	//Fulfill requirement from aleady allocated Inode direct blocks
	int allocDatablock[requiredDatablocks];
	int allocIndex = 0;
	for (int i = startBlockIndex; allocIndex < requiredDatablocks; ++i) {
		if (inode->direct_blocks[i] == -1)
			break;
		else
			allocDatablock[allocIndex++] = inode->direct_blocks[i];
	}

	//Fulfill requirement from data block free list
	for (int i = 0; allocIndex < requiredDatablocks && i < NUM_DATA_BLOCKS; i++) {
		if (superblock->datablock_freelist[i] == DATA_BLOCK_FREE) {
			superblock->datablock_freelist[i] = DATA_BLOCK_USED;
			allocDatablock[allocIndex++] = i;
		}
	}

	if (allocIndex != requiredDatablocks)
		return -1;

	//Write data blocks
	char tempBuf[BLOCKSIZE];
	int save = nbytes;

	allocIndex = 0;
	int curr_dataBlock = allocDatablock[allocIndex];
	int curr_WriteSize = min(nbytes, startBlockCapacity);

	//Writing first data block
	if (inode->direct_blocks[startBlockIndex] == -1) {
		bzero(tempBuf, BLOCKSIZE);
		memcpy(tempBuf, buf, curr_WriteSize);
	}
	else {
		simplefs_readDataBlock(curr_dataBlock, tempBuf);
		memcpy(tempBuf + startBlockOffset, buf, curr_WriteSize);
	}

	simplefs_writeDataBlock(curr_dataBlock, tempBuf);
	buf += curr_WriteSize;
	nbytes -= curr_WriteSize;

	//Writing subsequent blocks
	while (nbytes > 0) {
		curr_dataBlock = allocDatablock[++allocIndex];
		curr_WriteSize = min(nbytes, BLOCKSIZE);

		if (curr_WriteSize < BLOCKSIZE)	{
			if (inode->direct_blocks[startBlockIndex + allocIndex] == -1)
				bzero(tempBuf, BLOCKSIZE);
			else
				simplefs_readDataBlock(curr_dataBlock, tempBuf);
		}

		memcpy(tempBuf, buf, curr_WriteSize);
		simplefs_writeDataBlock(curr_dataBlock, tempBuf);
		buf += curr_WriteSize;
		nbytes -= curr_WriteSize;
	}

	if (buf[0] != '\0' || nbytes > 0)
		return -1;

	//Update Inode & file handle
	inode->file_size = max(inode->file_size, fileOffset + save);

	allocIndex = 0;
	for (int i = startBlockIndex; allocIndex < requiredDatablocks; ++i)
		inode->direct_blocks[i] = allocDatablock[allocIndex++];

	//Push changes to disk
	simplefs_writeSuperBlock(superblock);
	simplefs_writeInode(fileInode, inode);

	free(superblock);
	free(inode);

    return -1;
}


int simplefs_seek(int fileHandle, int nseek){
    /*
	   increase `fileHandle` offset by `nseek`
	*/

	struct inode_t *inode = (struct inode_t *)malloc(sizeof(struct inode_t));
	int fileInode = file_handle_array[fileHandle].inode_number;
	int fileOffset = file_handle_array[fileHandle].offset;

	simplefs_readInode(fileInode, inode);

	fileOffset += nseek;
	if (fileOffset < 0 || inode->file_size < fileOffset)
		return -1;

	file_handle_array[fileHandle].offset = fileOffset;

	free(inode);

    return -1;
}

int search_file(char *filename) {
	/*
	   search file with name `filename`
	*/
	struct inode_t *inode = (struct inode_t *)malloc(sizeof(struct inode_t));

	for (int i = 0; i < NUM_INODES; ++i) {
		simplefs_readInode(i, inode);
		if (strcmp(inode->name, filename) == 0)
			return i;
	}

	free(inode);
}