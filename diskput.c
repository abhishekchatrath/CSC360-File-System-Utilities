#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include "disk.h"

#define ANSI_COLOUR_BLUE    "\x1b[34m"
#define ANSI_COLOUR_RESET   "\x1b[0m"


/* ~~~~~ Global variable declarations ~~~~~ */
struct superblock_t superBlockInfo;
char *buffer1, *buffer2, *buffer4, *buffer8;
FILE* fp;

__uint8_t directoryPath[10][31];
int numTokensDirectoryPath = 0;
int numDirEntries = 0;

__uint32_t* directoryEntryPointers;
int directoryEntryPointersCtr = 0;

int fd;
struct stat buf;


/* ~~~~~ Function declarations ~~~~~ */

/**
 * Reads and stores the attributes of the superblock from the .img file into superBlockInfo.
 */
void readSuperBlockInfo();

/**
 * ...
 */
void displaySuperBlockInfo();

/**
 * ...
 */
void readInputFilePath(char* src);

/**
 * ...
 */
void copyFileToFS(char* filename);

/**
 * This function does the foloowing tasks as a part of the preparation required to transfer a file from the Linux FS to the .img FS
 * - Find and select an empty directory entry slot, and
 * - Find and select empty and available cells in the FAT, and
 * - Update the selected empty directory entry, and
 * - Update the selected empty cells in the FAT.
 */
__uint32_t* prepareFileTransferToFS(__uint32_t filesize);

/**
 * Read and return a list of directory entries from root_dir or sub_dir 
 */
struct dir_entry_t** analyzeDirectory();

/**
 * ...
 */
struct dir_entry_t** readDirectoryEntries(int blockNum);

/**
 * Each directory entry contains the starting block number for a file, say block number X.
 * To find the next block in the file, look at ***entry X*** in the FAT.
 * Eg: file starting block   X is entry (X %% 128) in block (X / 128) of FAT
 */
__uint32_t getNextBlock(__uint32_t currentBlock);

/**
 * ...
 */
void checkEmptyBlocksInFAT(int numFATEmptyBlocks, __uint32_t* FATEmptyBlocks);

/**
 * update empty dir entry slot in the root dir (or sub dir) with new file details
*/
void updateEmptyDirectoryEntry(__uint32_t chosenDirEntryAddr, __uint32_t startingBlock, __uint32_t blockCount, __uint32_t filesize);

/**
 * ...
 */
void updateFATEmptyBlocks(int numFATEmptyBlocks, __uint32_t FATEmptyBlocks[]);

/**
 * @param FATEmptyBlock: The byte address of a cell in the FAT that represents an empty and available block. 
 * It represents the number of bytes from the beginning of the file
 */
__uint32_t getBlockNum(__uint32_t FATEmptyBlock);




/* ~~~~~ Main program ~~~~~ */

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("\nError [0]: Please include a .img file as the first argument\n");
        return EXIT_FAILURE;
    }
    
    if (argc < 3) {
        printf("\nError [1]: Please include the INPUT filename you want to copy from the .img file as the second argument\n");
        return EXIT_FAILURE;
    }

    if (argc < 4) {
        printf("\nError [2]: Please include the OUTPUT filename for the copied file from the .img file as the third argument\n");
        return EXIT_FAILURE;
    }
    
    fp = fopen(argv[1], "r+b");
    if (fp == NULL) {
        printf("\nError [3]: Could not open file\n");
        return EXIT_FAILURE;
    }

    buffer1 = (char *) calloc(1, sizeof(char));
    buffer2 = (char *) calloc(2, sizeof(char));
    buffer4 = (char *) calloc(4, sizeof(char));
    buffer8 = (char *) calloc(8, sizeof(char));
    if (buffer1 == NULL || buffer2 == NULL || buffer4 == NULL || buffer8 == NULL) {
        printf("\nError [4]: Memory not allocated!\n");
        return EXIT_FAILURE;
    }

    // initialize superBlockInfo  
    readSuperBlockInfo();
    
    // for dev use only
    // printf("\n");
    // displaySuperBlockInfo();

    // extract and separate filename from the path from argv[3]
    readInputFilePath(argv[3]);

    copyFileToFS(argv[2]);

    return 0;
}




/* ~~~~~ Function implementations ~~~~~ */


void readSuperBlockInfo() {

    // read file system identifier
    fread(buffer8, sizeof(char), 8, fp);
    memcpy(superBlockInfo.fs_id, buffer8, sizeof(char) * 8);
    
    // read block size
    fread(buffer2, sizeof(char), 2, fp);
    superBlockInfo.block_size = buffer2[0] << 8 | buffer2[1];

    // read block count
    fread(buffer4, sizeof(char), 4, fp);
    superBlockInfo.file_system_block_count = (__uint64_t) buffer4[0] << 32 | buffer4[1] << 16 | buffer4[2] << 8 | buffer4[3];

    // read FAT starts
    fread(buffer4, sizeof(char), 4, fp);
    superBlockInfo.fat_start_block = (__uint64_t) buffer4[0] << 32 | buffer4[1] << 16 | buffer4[2] << 8 | buffer4[3];

    // read FAT block count
    fread(buffer4, sizeof(char), 4, fp);
    superBlockInfo.fat_block_count = (__uint64_t) buffer4[0] << 32 | buffer4[1] << 16 | buffer4[2] << 8 | buffer4[3];

    // read read dir starts
    fread(buffer4, sizeof(char), 4, fp);
    superBlockInfo.root_dir_start_block = (__uint64_t) buffer4[0] << 32 | buffer4[1] << 16 | buffer4[2] << 8 | buffer4[3];

    // read read dir block count
    fread(buffer4, sizeof(char), 4, fp);
    superBlockInfo.root_dir_block_count = (__uint64_t) buffer4[0] << 32 | buffer4[1] << 16 | buffer4[2] << 8 | buffer4[3];
    
}


void displaySuperBlockInfo() {
    printf("Super block information:\n");
    printf("Block size: %d\n", superBlockInfo.block_size);
    printf("Block count: %d\n", superBlockInfo.file_system_block_count);
    printf("FAT starts: %d\n", superBlockInfo.fat_start_block);
    printf("FAT blocks: %d\n", superBlockInfo.fat_block_count);
    printf("Root directory start: %d\n", superBlockInfo.root_dir_start_block);
    printf("Root directory blocks: %d\n", superBlockInfo.root_dir_block_count);
    printf("\n");
}


void readInputFilePath(char* src) {
    __uint8_t* token = (__uint8_t *) calloc(31, sizeof(char));
    token = (__uint8_t *) strtok(src, "/");
    
    while (token != NULL) {
        strcpy((char *) &directoryPath[numTokensDirectoryPath], (char *) token);
        numTokensDirectoryPath++;
        token = (__uint8_t *) strtok(NULL, "/");
    }

    // code below should be commented out
    int directoryPathCtr = 0;
    while (directoryPathCtr < numTokensDirectoryPath) {
        // printf("%d: %s\n", directoryPathCtr + 1, directoryPath[directoryPathCtr]);
        directoryPathCtr++;
    }
    // printf("\n");

}


void copyFileToFS(char* filename) {
    // fpr will read contents of argv[2] from Linux
    FILE* fpr = fopen(filename, "rb");
    if (fpr == NULL) {
        // printf("\nError [5]: Could not open file\n");
        printf("\n" ANSI_COLOUR_BLUE "File not found." ANSI_COLOUR_RESET "\n");
        exit(EXIT_SUCCESS);
    }

    // used to check create and modify time later
    stat(filename, &buf);

    // chose a different approach to get file size, also impl'd this earlier
    fseek(fpr, 0L, SEEK_END);
    __uint32_t fileSize = ftell(fpr);
    // printf("fileSize: %d bytes\n\n", fileSize);
    fseek(fpr, 0L, SEEK_SET);

    // update an empty dir entry and empty FAT cells with the relevant info of the file to be copied
    __uint32_t* FATEmptyBlocks = prepareFileTransferToFS(fileSize);

    // copy-paste magic!
    // printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    
    if (fileSize == 0) {
        // do nothing
    } else if (fileSize < 8) {                             /* copying files under 8 bytes */
        fseek(fp, getBlockNum(FATEmptyBlocks[0]) * blockSize, SEEK_SET);        
        // printf("copying to block num %d\n", getBlockNum(FATEmptyBlocks[0]));
        switch (fileSize) {
            case 7:
                fread(buffer1, sizeof(char), 1, fpr);
                // printf("%s", buffer1);
                fwrite(buffer1, sizeof(char), 1, fp);    /* todo: activate! */
                // break;
            case 6:
                fread(buffer1, sizeof(char), 1, fpr);
                // printf("%s", buffer1);
                fwrite(buffer1, sizeof(char), 1, fp);    /* todo: activate! */
                // break;
            case 5:
                fread(buffer1, sizeof(char), 1, fpr);
                // printf("%s", buffer1);
                fwrite(buffer1, sizeof(char), 1, fp);    /* todo: activate! */
                // break;
            case 4:
                fread(buffer1, sizeof(char), 1, fpr);
                // printf("%s", buffer1);
                fwrite(buffer1, sizeof(char), 1, fp);    /* todo: activate! */
                // break;
            case 3:
                fread(buffer1, sizeof(char), 1, fpr);
                // printf("%s", buffer1);
                fwrite(buffer1, sizeof(char), 1, fp);    /* todo: activate! */
                // break;
            case 2:
                fread(buffer1, sizeof(char), 1, fpr);
                // printf("%s", buffer1);
                fwrite(buffer1, sizeof(char), 1, fp);    /* todo: activate! */
                // break;
            case 1:
                fread(buffer1, sizeof(char), 1, fpr);
                // printf("%s", buffer1);
                fwrite(buffer1, sizeof(char), 1, fp);    /* todo: activate! */
            default:
                break;
        }
    } else {
        int ctr = 0, blockCtr = 0;
        while (ctr < fileSize - 8) {
            if (ctr % blockSize == 0) {
                fseek(fp, getBlockNum(FATEmptyBlocks[blockCtr]) * blockSize, SEEK_SET);
                // printf("copying to block num %d\n", getBlockNum(FATEmptyBlocks[blockCtr]));
                blockCtr++;
            }
            fread(buffer8, sizeof(char), 8, fpr);
            // printf("%s", buffer8);
            fwrite(buffer8, sizeof(char), 8, fp);    /* todo: activate! */
            ctr += 8;
        }
        switch ((fileSize - ctr) % 8) {
            case 7:
                fread(buffer1, sizeof(char), 1, fpr);
                // printf("%s", buffer1);
                fwrite(buffer1, sizeof(char), 1, fp);    /* todo: activate! */
                // break;
            case 6:
                fread(buffer1, sizeof(char), 1, fpr);
                // printf("%s", buffer1);
                fwrite(buffer1, sizeof(char), 1, fp);    /* todo: activate! */
                // break;
            case 5:
                fread(buffer1, sizeof(char), 1, fpr);
                // printf("%s", buffer1);
                fwrite(buffer1, sizeof(char), 1, fp);    /* todo: activate! */
                // break;
            case 4:
                fread(buffer1, sizeof(char), 1, fpr);
                // printf("%s", buffer1);
                fwrite(buffer1, sizeof(char), 1, fp);    /* todo: activate! */
                // break;
            case 3:
                fread(buffer1, sizeof(char), 1, fpr);
                // printf("%s", buffer1);
                fwrite(buffer1, sizeof(char), 1, fp);    /* todo: activate! */
                // break;
            case 2:
                fread(buffer1, sizeof(char), 1, fpr);
                // printf("%s", buffer1);
                fwrite(buffer1, sizeof(char), 1, fp);    /* todo: activate! */
                // break;
            case 1:
                fread(buffer1, sizeof(char), 1, fpr);
                // printf("%s", buffer1);
                fwrite(buffer1, sizeof(char), 1, fp);    /* todo: activate! */
            default:
                break;
        }
    }
    // printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

    // clean up
    if (FATEmptyBlocks != NULL) {
        free(FATEmptyBlocks);
    }
    // close(fd);
    fclose(fpr);
}


__uint32_t* prepareFileTransferToFS(__uint32_t filesize) {
    struct dir_entry_t** directoryEntries = NULL;
    struct dir_entry_t* rootDirectory = (struct dir_entry_t *) calloc(1, sizeof(struct dir_entry_t));
    rootDirectory->starting_block = superBlockInfo.root_dir_start_block;
    rootDirectory->block_count = superBlockInfo.root_dir_block_count;
    __uint32_t chosenDirEntryAddr;
    int dirEntryCtr = 0;

    // check root dir (or sub dir) for an empty dir entry slot (eg: No_file)
    if (numTokensDirectoryPath == 1) {                          /* root dir */
        directoryEntries = analyzeDirectory(rootDirectory);
        while (dirEntryCtr < numDirEntries) {
            // printf("status: %u %x\n", directoryEntries[dirEntryCtr]->status, directoryEntries[dirEntryCtr]->status);
            if (directoryEntries[dirEntryCtr]->status == 0) {
                chosenDirEntryAddr = directoryEntryPointers[dirEntryCtr];
                break;
            }
            dirEntryCtr++;
        }
        if (dirEntryCtr == numDirEntries) {
            printf("Error [6]: Could not find an empty slot to save the file in the given directory\n");
            exit(EXIT_FAILURE);
        }
    } else {                                                    /* sub dir */
        // todo: impl
        printf("Cannot do subdir right now.\n");
        exit(0);
        // ...
    }
    
    // check if FAT has total number of available blocks needed for file transfer
    int numFATEmptyBlocks = (filesize / blockSize) + (filesize % blockSize == 0 ? 0 : 1);
    __uint32_t* FATEmptyBlocks = (__uint32_t *) calloc(numFATEmptyBlocks, sizeof(__uint32_t));       /* array of byte addresses of empty and available blocks in FAT in the .img file */
    checkEmptyBlocksInFAT(numFATEmptyBlocks, FATEmptyBlocks);


    // update empty dir entry slot in the root dir (or sub dir) with new file details
    updateEmptyDirectoryEntry(chosenDirEntryAddr, FATEmptyBlocks[0], numFATEmptyBlocks, filesize);


    // update list of available blocks in FAT to make a linked list for the new file
    updateFATEmptyBlocks(numFATEmptyBlocks, FATEmptyBlocks);
    

    // point fp to starting block of the new file
    // printf("\nseeking to block num %d (0x%x) which is at 0x%x\n\n", getBlockNum(FATEmptyBlocks[0]), getBlockNum(FATEmptyBlocks[0]), getBlockNum(FATEmptyBlocks[0]) * blockSize);
    // fseek(fp, getBlockNum(FATEmptyBlocks[0]) * blockSize, SEEK_SET);
    

    free(rootDirectory);
    // free(directoryEntryPointers);

    return FATEmptyBlocks;
}


struct dir_entry_t** analyzeDirectory(struct dir_entry_t* searchDirectoryEntry) {
    numDirEntries = searchDirectoryEntry->block_count * 8;
    
    directoryEntryPointers = (__uint32_t *) calloc(numDirEntries, sizeof(__uint32_t));

    struct dir_entry_t** directoryEntries = (struct dir_entry_t **) calloc(numDirEntries, sizeof(struct dir_entry_t));
    struct dir_entry_t** currentDirectoryEntries;
    __uint32_t currentBlock = searchDirectoryEntry->starting_block;
    int blockCtr = 0;
    int offset = 0;

    // int debugCtr = 8;
    
    while (blockCtr < searchDirectoryEntry->block_count) {
        currentDirectoryEntries = readDirectoryEntries(currentBlock);                                   // get dir entries from currentBlock (8 entries per block)        
        
        // printf("currentDirectoryEntries:\n");
        // for (int i = 0; i < 8; i++) {
        //     printf("%d: %s\n", i, currentDirectoryEntries[i]->filename);
        // }
        // printf("\n");

        memcpy(&directoryEntries[offset], currentDirectoryEntries, sizeof(struct dir_entry_t) * 8);     // append currentDirectoryEntries to directoryEntries
        
        // printf("directoryEntries:\n");
        // for (int i = 0; i < debugCtr; i++) {
        //     printf("%d: %s\n", i, directoryEntries[i]->filename);
        // }
        // debugCtr += 8;
        // printf("\n");
        
        offset += 8;                                                                                    // calibrate offset for append to directoryEntries using memcpy
        currentBlock = getNextBlock(currentBlock);                                                      // get next block from FAT
        blockCtr++;
        free(currentDirectoryEntries);
    }
    
    return directoryEntries;
}


struct dir_entry_t** readDirectoryEntries(int blockNum) {

    int dirStartPoint = blockNum * blockSize;
    int dirEndPoint = (blockNum + 1) * blockSize;

    struct dir_entry_t** directoryEntries = calloc(8, sizeof(struct dir_entry_t));  // 8 dir entries per block
    struct dir_entry_t* directoryEntry;
    int arrIndex = 0;

    while (dirStartPoint < dirEndPoint) {
        directoryEntry = (struct dir_entry_t *) calloc(1, sizeof(struct dir_entry_t));

        fseek(fp, dirStartPoint, SEEK_SET);
        directoryEntryPointers[directoryEntryPointersCtr++] = ftell(fp);

        fread(buffer1, sizeof(char), 1, fp);
        directoryEntry->status = buffer1[0];
        
        fread(buffer4, sizeof(char), 4, fp);
        directoryEntry->starting_block = (__uint64_t) buffer4[0] << 32 | buffer4[1] << 16 | buffer4[2] << 8 | buffer4[3];
        
        fread(buffer4, sizeof(char), 4, fp);
        directoryEntry->block_count = (__uint64_t) buffer4[0] << 32 | buffer4[1] << 16 | buffer4[2] << 8 | buffer4[3];

        fread(buffer4, sizeof(char), 4, fp);
        directoryEntry->size = (__uint64_t) buffer4[0] << 32 | buffer4[1] << 16 | buffer4[2] << 8 | (buffer4[3] & 0b11111111);

        fread(buffer8, sizeof(char), 7, fp);
        directoryEntry->create_time.year = buffer8[0] << 8 | (buffer8[1] & 0xFF);
        directoryEntry->create_time.month = buffer8[2];
        directoryEntry->create_time.day = buffer8[3];
        directoryEntry->create_time.hour = buffer8[4];
        directoryEntry->create_time.minute = buffer8[5];
        directoryEntry->create_time.second = buffer8[6];

        fread(buffer8, sizeof(char), 7, fp);
        directoryEntry->modify_time.year = buffer8[0] << 8 | (buffer8[1] & 0xFF);
        directoryEntry->modify_time.month = buffer8[2];
        directoryEntry->modify_time.day = buffer8[3];
        directoryEntry->modify_time.hour = buffer8[4];
        directoryEntry->modify_time.minute = buffer8[5];
        directoryEntry->modify_time.second = buffer8[6];

        fread(directoryEntry->filename, sizeof(char), 31, fp);
        
        fread(directoryEntry->unused, sizeof(char), 6, fp);
    
        dirStartPoint += 64;
        directoryEntries[arrIndex++] = directoryEntry;
    }
    
    return directoryEntries;
}


__uint32_t getNextBlock(__uint32_t currentBlock) {
    __uint32_t dirEntryFATBlockNum = (currentBlock / 128);
    __uint32_t dirEntryFATEntryNum = (currentBlock % 128);
    __uint32_t FATEntrySize = 4;   // 4 bytes
    __uint32_t FATEntryPoint = (((superBlockInfo.fat_start_block + dirEntryFATBlockNum) * blockSize) + (dirEntryFATEntryNum * FATEntrySize));
    __uint32_t FATEntryValue;

    fseek(fp, FATEntryPoint, SEEK_SET);
    fread(buffer4, sizeof(char), 4, fp);
    FATEntryValue = (__uint64_t) buffer4[0] << 32 | buffer4[1] << 16 | buffer4[2] << 8 | buffer4[3];
    
    return FATEntryValue;
}


void checkEmptyBlocksInFAT(int numFATEmptyBlocks, __uint32_t* FATEmptyBlocks) {
    __uint32_t FATEntryValue;
    int FATBlockCtr = 0;

    fseek(fp, superBlockInfo.fat_start_block * blockSize, SEEK_SET);
    
    while (FATBlockCtr < numFATEmptyBlocks) {
        fread(buffer4, sizeof(char), 4, fp);
        FATEntryValue = (__uint64_t) buffer4[0] << 32 | buffer4[1] << 16 | buffer4[2] << 8 | (buffer4[3] & 0xFF);
        if (FATEntryValue == 0) {
            // printf("just read 4 worthy bytes and now at %x: %x\n", ftell(fp), (ftell(fp) - 4) );
            FATEmptyBlocks[FATBlockCtr] = ftell(fp) - 4;
            // printf("Empty block in FAT found at point %x (block num %d 0x%x)\n", FATEmptyBlocks[FATBlockCtr], 
                // (FATEmptyBlocks[FATBlockCtr] - (superBlockInfo.fat_start_block * blockSize) ) / 4,
                // (FATEmptyBlocks[FATBlockCtr] - (superBlockInfo.fat_start_block * blockSize) ) / 4);
            FATBlockCtr++;
        } 
    }
}


void updateEmptyDirectoryEntry(__uint32_t chosenDirEntryAddr, __uint32_t startingBlock, __uint32_t blockCount, __uint32_t filesize) {

    // printf("\nchosen dir entry addr %x\n\n", chosenDirEntryAddr);
    fseek(fp, chosenDirEntryAddr, SEEK_SET);
    
    /* status */
    buffer1[0] = (__uint8_t) 0x03;              /* set status to in use and file` */
    fwrite(buffer1, sizeof(char), 1, fp);                    /* todo: activate! */
    // printf("status: %x\n\n", buffer1[0]);

    /* starting block */
    // __uint32_t startingBlock = FATEmptyBlocks[0];
    buffer4[0] = ( (__uint64_t) startingBlock & 0xFF000000) >> 32;
    buffer4[1] = ( (__uint64_t) startingBlock & 0x00FF0000) >> 16;
    buffer4[2] = ( (__uint64_t) startingBlock & 0x0000FF00) >> 8;
    buffer4[3] =   (__uint8_t ) startingBlock & 0x000000FF;
    // printf("startingBlock: 0x%08x\n", startingBlock);
    // printf("buffer4: %02x %02x %02x %02x\n\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);
    // printf("buffer4 to startingBlock: %x \n", (__uint64_t) buffer4[0] << 32 | buffer4[1] << 16 | buffer4[2] << 8 | buffer4[3]);
    fwrite(buffer4, sizeof(char), 4, fp);                    /* todo: activate! */

    /* block count */
    // __uint32_t blockCount = numFATEmptyBlocks;
    buffer4[0] = ( (__uint64_t) blockCount & 0xFF000000) >> 32;
    buffer4[1] = ( (__uint64_t) blockCount & 0x00FF0000) >> 16;
    buffer4[2] = ( (__uint64_t) blockCount & 0x0000FF00) >> 8;
    buffer4[3] =   (__uint8_t ) blockCount & 0x000000FF;
    // printf("blockCount: 0x%08x\n", blockCount);
    // printf("buffer4: %02x %02x %02x %02x\n\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);
    fwrite(buffer4, sizeof(char), 4, fp);                    /* todo: activate! */

    /* file size */
    buffer4[0] = ( (__uint64_t) filesize & 0xFF000000) >> 32;
    buffer4[1] = ( (__uint64_t) filesize & 0x00FF0000) >> 16;
    buffer4[2] = ( (__uint64_t) filesize & 0x0000FF00) >> 8;
    buffer4[3] =   (__uint8_t ) filesize & 0x000000FF;
    // printf("filesize: %u 0x%08x\n", filesize, filesize);
    // printf("buffer4: %02x %02x %02x %hhx (%u + %hhu)\n\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3], buffer4[2] * 256, buffer4[3]);
    fwrite(buffer4, sizeof(char), 4, fp);                    /* todo: activate! */

    /* create time */
    struct tm* changeTime = localtime(&buf.st_ctime);
    // printf("%s", asctime(changeTime));
    // printf("%4d/%02d/%02d ", changeTime->tm_year + 1900, changeTime->tm_mon + 1, changeTime->tm_mday);
    // printf("%02d:%02d:%02d\n\n", changeTime->tm_hour, changeTime->tm_min, changeTime->tm_sec);
    buffer8[0] = ((changeTime->tm_year + 1900) & 0xFF00) >> 8;
    buffer8[1] = ((changeTime->tm_year + 1900) & 0x00FF);
    buffer8[2] = (__uint8_t) (changeTime->tm_mon + 1);
    buffer8[3] = (__uint8_t) (changeTime->tm_mday);
    buffer8[4] = (__uint8_t) (changeTime->tm_hour);
    buffer8[5] = (__uint8_t) (changeTime->tm_min);
    buffer8[6] = (__uint8_t) (changeTime->tm_sec);
    // printf("reconstruct create date and time:\n");
    // printf("%04u/%02u/%02u ", buffer8[0] << 8 | (buffer8[1] & 0xFF), buffer8[2], buffer8[3]);
    // printf("%02u:%02u:%02u\n\n", buffer8[4], buffer8[5], buffer8[6]);
    fwrite(buffer8, sizeof(char), 7, fp);                    /* todo: activate! */

    /* modify time */
    struct tm* modifyTime = localtime(&buf.st_mtime);
    // printf("%s", asctime(modifyTime));
    // printf("%4d/%02d/%02d ", modifyTime->tm_year + 1900, modifyTime->tm_mon + 1, modifyTime->tm_mday);
    // printf("%02d:%02d:%02d\n\n", modifyTime->tm_hour, modifyTime->tm_min, modifyTime->tm_sec);
    buffer8[0] = ((modifyTime->tm_year + 1900) & 0xFF00) >> 8;
    buffer8[1] = ((modifyTime->tm_year + 1900) & 0x00FF);
    buffer8[2] = (__uint8_t) (modifyTime->tm_mon + 1);
    buffer8[3] = (__uint8_t) (modifyTime->tm_mday);
    buffer8[4] = (__uint8_t) (modifyTime->tm_hour);
    buffer8[5] = (__uint8_t) (modifyTime->tm_min);
    buffer8[6] = (__uint8_t) (modifyTime->tm_sec);
    // printf("reconstruct modify date and time:\n");
    // printf("%04u/%02u/%02u ", buffer8[0] << 8 | (buffer8[1] & 0xFF), buffer8[2], buffer8[3]);
    // printf("%02u:%02u:%02u\n\n", buffer8[4], buffer8[5], buffer8[6]);
    fwrite(buffer8, sizeof(char), 7, fp);                    /* todo: activate! */

    /* filename */
    char* filename = (char *) calloc(31 , sizeof(char));
    strcpy(filename, (char *) directoryPath[numTokensDirectoryPath - 1]);
    // printf("directoryPath[numTokensDirectoryPath - 1]: %s\n", directoryPath[numTokensDirectoryPath - 1]);
    // printf("%d\n", strlen(directoryPath[numTokensDirectoryPath - 1]));
    // printf("filename: (%s)\n", filename);
    // printf("filename length = %ld\n", strlen(filename));
    fwrite(filename, sizeof(char), strlen(filename), fp);    /* todo: activate! */
    buffer1[0] = (__uint8_t) 0;
    fwrite(buffer1, sizeof(char), 1, fp);                   /* todo: activate! */
    free(filename);


}


void updateFATEmptyBlocks(int numFATEmptyBlocks, __uint32_t FATEmptyBlocks[]) {
    int nextBlockNum;
    int FATBlockCtr = 0;
    while (FATBlockCtr < numFATEmptyBlocks - 1) {
        fseek(fp, FATEmptyBlocks[FATBlockCtr], SEEK_SET);
        // nextBlockNum = (FATEmptyBlocks[FATBlockCtr + 1] - (superBlockInfo.fat_start_block * blockSize)) / 4;
        nextBlockNum = getBlockNum(FATEmptyBlocks[FATBlockCtr + 1]);
        buffer4[0] = ( (__uint64_t) nextBlockNum & 0xFF000000) >> 32;
        buffer4[1] = ( (__uint64_t) nextBlockNum & 0x00FF0000) >> 16;
        buffer4[2] = ( (__uint64_t) nextBlockNum & 0x0000FF00) >> 8;
        buffer4[3] =   (__uint8_t) (nextBlockNum & 0x000000FF);
        // printf("Current point: %x\n", FATEmptyBlocks[FATBlockCtr]);
        // printf("nextBlockNum: %d (0x%x = %x %x %x %x)\n", nextBlockNum, nextBlockNum, buffer4[0], buffer4[1], buffer4[2], buffer4[3]);
        fwrite(buffer4, sizeof(char), 4, fp);                /* todo: activate! */
        FATBlockCtr++;
    }
    fseek(fp, FATEmptyBlocks[FATBlockCtr], SEEK_SET);
    buffer4[0] = (__uint8_t) 0xFF;
    buffer4[1] = (__uint8_t) 0xFF;
    buffer4[2] = (__uint8_t) 0xFF;
    buffer4[3] = (__uint8_t) 0xFF;
    fwrite(buffer4, sizeof(char), 4, fp);                    /* todo: activate! */
}


__uint32_t getBlockNum(__uint32_t FATEmptyBlock) {
    // nextBlockNum = (FATEmptyBlocks[FATBlockCtr + 1] - (superBlockInfo.fat_start_block * blockSize)) / 4;
    return (FATEmptyBlock - (superBlockInfo.fat_start_block * blockSize) ) / 4;
}

