#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "disk.h"

struct superblock_t superBlockInfo;
char *buffer1, *buffer2, *buffer4, *buffer8;
FILE* fp;

int numRootDirEntries, numCurrentDirEntries;


void readSuperBlockInfoFromImgFile();
struct dir_entry_t** readDirectoryEntries(int blockNum);
void analyzeDirectory(char* directoryName);
struct dir_entry_t** analyzeRootDirectory();
struct dir_entry_t** analyzeSubDirectory(struct dir_entry_t* directoryEntry);
void displaySuperBlockInfo();
void displayDirectoryEntries(struct dir_entry_t** directoryEntries);
int arrayLength(void* arr);
__uint32_t getNextBlock(__uint32_t currentBlock);


void readSuperBlockInfoFromImgFile() {

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


struct dir_entry_t** analyzeRootDirectory() {
    // numRootDirEntries = superBlockInfo.root_dir_block_count * 8;
    numRootDirEntries = 8;
    struct dir_entry_t** directoryEntries = readDirectoryEntries(superBlockInfo.root_dir_start_block);
    return directoryEntries;
}


/**
 *  Returns a struct dir_entry_t** directoryEntries that needs to be free()'d after use.
 */
struct dir_entry_t** analyzeRootDirectory_OLD() {

    int dirStartPoint = superBlockInfo.root_dir_start_block * blockSize;
    int dirEndPoint = (superBlockInfo.root_dir_start_block + superBlockInfo.root_dir_block_count) * blockSize;

    // Each directory entry takes up 64 bytes, which implies there are 8 directory entries per 512 byte block. (source = p3.pdf)
    // struct dir_entry_t* directoryEntries[superBlockInfo.root_dir_block_count * 8];
    numRootDirEntries = superBlockInfo.root_dir_block_count * 8;
    struct dir_entry_t** directoryEntries = calloc(numRootDirEntries, sizeof(struct dir_entry_t));
    // struct dir_entry_t** directoryEntries = calloc(5, sizeof(struct dir_entry_t));    
    struct dir_entry_t* directoryEntry;
    int arrIndex = 0;

    while (dirStartPoint < dirEndPoint) {
        directoryEntry = (struct dir_entry_t *) calloc(1, sizeof(struct dir_entry_t));

        fseek(fp, dirStartPoint, SEEK_SET);
        
        fread(buffer1, sizeof(char), 1, fp);
        directoryEntry->status = buffer1[0];
        // fileType = directoryEntry->status & 0b0010 ? 'F' : 
            // (directoryEntry->status & 0b0100 ? 'D' : 'N');
        
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
        // sprintf(date, "%04u/%02u/%02u", directoryEntry->modify_time.year, directoryEntry->modify_time.month, directoryEntry->modify_time.day);
        // sprintf(time, "%02u:%02u:%02u", directoryEntry->modify_time.hour, directoryEntry->modify_time.minute, directoryEntry->modify_time.second);

        fread(directoryEntry->filename, sizeof(char), 31, fp);
        
        fread(directoryEntry->unused, sizeof(char), 6, fp);

        dirStartPoint += 64;
        directoryEntries[arrIndex++] = directoryEntry;

        // if (directoryEntry->status == 0) {   
        //     continue;
        // }
        
        // imp so do not delete!
        // printf("%c %10u %30s %10s %8s\n", fileType, directoryEntry->size, directoryEntry->filename, date, time);
    }

    // displayDirectoryEntries(directoryEntries);

    return directoryEntries;
}


/**
 * @deprecated arrayLength() does not return correct value of length. The sizeof() calculates the size of the pointer at compile time. 
 * The only way around this apparently is to pass the number of elements as a parameter with the array.
 *  WHAT A STUPID LANGUAGE.
 */
int arrayLength(void* arr) {
    return (int) sizeof(arr) / sizeof(arr[0]);
}


void displayDirectoryEntries(struct dir_entry_t** directoryEntries) {

    char fileType;
    char* date = (char *) calloc(10, sizeof(char));
    char* time = (char *) calloc(8, sizeof(char));
    if (date == NULL || time == NULL) {
        perror("Error: Memory not allocated\n");
        exit(EXIT_FAILURE);
    }

    int ctr = 0;
    int numEntries = numRootDirEntries;

    while (ctr < numEntries) {
        if (directoryEntries[ctr]->status == 0) {   
            ctr++;
            continue;
        }
        fileType = directoryEntries[ctr]->status & 0b0010 ? 'F' : 
            (directoryEntries[ctr]->status & 0b0100 ? 'D' : 'N');
        sprintf(date, "%04u/%02u/%02u", directoryEntries[ctr]->modify_time.year, directoryEntries[ctr]->modify_time.month, directoryEntries[ctr]->modify_time.day);
        sprintf(time, "%02u:%02u:%02u", directoryEntries[ctr]->modify_time.hour, directoryEntries[ctr]->modify_time.minute, directoryEntries[ctr]->modify_time.second);
        printf("%c %10u %30s %10s %8s\n", fileType, directoryEntries[ctr]->size, directoryEntries[ctr]->filename, date, time);
        ctr++;
    }

    free(time);
    free(date);
}


/**
 * For developer notes only. Not really used for the assignment.
 */
void archiveFunc(struct dir_entry_t* searchDirectoryEntry) {
    printf("\nabout to analyze %s for dir entries\n", searchDirectoryEntry->filename);
    printf("starting block\t\t%u\n", searchDirectoryEntry->starting_block);
    printf("block count\t\t%u\n", searchDirectoryEntry->block_count);
    printf("size\t\t\t%u\n\n", searchDirectoryEntry->size);

    printf("FAT starting block\t%d\n", superBlockInfo.fat_start_block);
    printf("\nSince FAT starts at block %d in the .img file, and there are 128 FAT entries per block, we have:\n", superBlockInfo.fat_start_block);
    printf( "- file starting block   0 is entry   0 in block 0 of FAT\n" \
            "- file starting block   1 is entry   1 in block 0 of FAT\n" \
            "- file starting block   2 is entry   2 in block 0 of FAT\n" \
            "- file starting block 127 is entry 127 in block 0 of FAT\n" \
            "- file starting block 128 is entry   0 in block 1 of FAT\n" \
            "- file starting block 129 is entry   1 in block 1 of FAT\n" \
            "- file starting block 130 is entry   2 in block 1 of FAT\n" \
            "- file starting block 255 is entry 127 in block 1 of FAT\n" \
            "- file starting block   X is entry (X %% 128) in block (X / 128) of FAT\n");
    printf("\nSo, %s is starting at block %d which is entry %d block %d in the .img file\n\n", 
        searchDirectoryEntry->filename, searchDirectoryEntry->starting_block, (searchDirectoryEntry->starting_block % 128),
        (searchDirectoryEntry->starting_block / 128) );
    
    int dirEntryFATBlockNum = (searchDirectoryEntry->starting_block / 128);
    int dirEntryFATEntryNum = (searchDirectoryEntry->starting_block % 128);
    int FATEntrySize = 4;   // 4 bytes

    printf( "FATEntryPoint = ((FAT starting block + dirEntryFATBlockNum) * blockSize) + (dirEntryFATEntryNum * FATEntrySize)\n" \
            "              = ((%d + %d) * %d) + (%d * %d)\n" \
            "              = %d\n", 
        superBlockInfo.fat_start_block, dirEntryFATBlockNum, blockSize, dirEntryFATEntryNum, FATEntrySize, 
        (((superBlockInfo.fat_start_block + dirEntryFATBlockNum) * blockSize) + (dirEntryFATEntryNum * FATEntrySize)) );


    // FAT stuff
    // 
    __uint32_t FATEntryPoint = (((superBlockInfo.fat_start_block + dirEntryFATBlockNum) * blockSize) + (dirEntryFATEntryNum * FATEntrySize));
    int FATEntryValue;    
    printf("FATEntryPoint = %d\t= 0x%04x\n\n", FATEntryPoint, FATEntryPoint);

    fseek(fp, FATEntryPoint, SEEK_SET);
    printf("pointing to entry 0x%04x in FAT\n", FATEntryPoint);

    fread(buffer4, sizeof(char), 4, fp);
    printf("value at entry 0x%04x is 0x(%02hx %02hx %02hx %02hx)\n", FATEntryPoint, buffer4[0], buffer4[1], buffer4[2], buffer4[3]);
    FATEntryValue = (__uint64_t) buffer4[0] << 32 | buffer4[1] << 16 | buffer4[2] << 8 | buffer4[3];
    printf("value: %u\n", FATEntryValue);

}


/** 
 *  1. input is a directoryEntry so it has info on starting block and block count 
 *  for that specific subdir 
 *      - DONE
 *  
 *  2. start looking for files/directories in the starting block 
 *      - DONE
 * 
 *  3. check FAT if next block of a subdir exists, if yes then point to that block 
 *  and repeat step 2 for that block specifically. 
 *      - DONE
 * 
 *  4. if next block according to FAT is 0xFFFFFFFF then stop searching and return 
 *      - DONE 
 *      - (not really but using blockCtr we dont overflow on blocks)
 * 
 *  5.  BRO YOU NEED TO DO THIS FOR ROOT DIR AS WELL !!!!! OMG!
 *      - (root dir blocks are consecutive I believe ? so not needed)
 */
struct dir_entry_t** analyzeSubDirectory(struct dir_entry_t* searchDirectoryEntry) {
    
    struct dir_entry_t** directoryEntries = (struct dir_entry_t **) calloc(searchDirectoryEntry->block_count * 8, sizeof(struct dir_entry_t));
    struct dir_entry_t** currentDirectoryEntries;
    __uint32_t currentBlock = searchDirectoryEntry->starting_block;
    int blockCtr = 0;
    int offset = 0;

    while (blockCtr < searchDirectoryEntry->block_count) {
        currentDirectoryEntries = readDirectoryEntries(currentBlock);                                   // get dir entries from currentBlock (8 entries per block)        
        memcpy(directoryEntries + offset, currentDirectoryEntries, sizeof(struct dir_entry_t) * 8);     // append currentDirectoryEntries to directoryEntries
        offset += sizeof(struct dir_entry_t) * 8;                                                       // calibrate offset for append to directoryEntries using memcpy
        currentBlock = getNextBlock(currentBlock);                                                      // get next block from FAT
        blockCtr++;
        free(currentDirectoryEntries);
    }

    return directoryEntries;
}


/**
 * Each directory entry contains the starting block number for a file, say block number X.
 * To find the next block in the file, look at ***entry X*** in the FAT.
 * Eg: file starting block   X is entry (X %% 128) in block (X / 128) of FAT
 */
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


struct dir_entry_t** readDirectoryEntries(int blockNum) {

    int dirStartPoint = blockNum * blockSize;
    int dirEndPoint = (blockNum + 1) * blockSize;

    struct dir_entry_t** directoryEntries = calloc(8, sizeof(struct dir_entry_t));  // 8 dir entries per block
    struct dir_entry_t* directoryEntry;
    int arrIndex = 0;

    while (dirStartPoint < dirEndPoint) {
        directoryEntry = (struct dir_entry_t *) calloc(1, sizeof(struct dir_entry_t));

        fseek(fp, dirStartPoint, SEEK_SET);
        
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


void analyzeDirectory(char* directoryName) {
    if (directoryName == NULL) {
        directoryName = (char *) calloc(31, sizeof(char));
        if (directoryName == NULL) {
            perror("Error: Memory not allocated\n");
            exit(EXIT_FAILURE);
        }
        sprintf(directoryName, "/");
    }

    __uint8_t* token = (__uint8_t *) calloc(31, sizeof(char));
    __uint8_t directoryPath[10][31];
    // I decided for an arbitrary directory depth of 10. Can be changed if needed. 

    // obtain list of directory entries from root directory
    struct dir_entry_t** directoryEntries = analyzeRootDirectory();

    // return if root directory    
    if (strcmp(directoryName, "/") == 0) {
        displayDirectoryEntries(directoryEntries);
        free(directoryEntries);
        return;
    }
    
    /* routine to tokenize subdirectory path */
    // printf("\nanalyze directory: %s\n", directoryName);
    token = (__uint8_t *) strtok(directoryName, "/");
    int numTokensDirectoryPath = 0;
    while(token != NULL) {
        strcpy((char *) &directoryPath[numTokensDirectoryPath], (char *) token);
        // memcpy(&directoryPath[numTokensDirectoryPath], token, strlen((char *) token));
        numTokensDirectoryPath += 1;
        token = (__uint8_t *) strtok(NULL, "/");
    }
    
    /* routine to print out the subdirectory tokens */
    // printf("\ndirectoryPath:\n");
    int directoryPathCtr = 0;
    // while (directoryPathCtr < numTokensDirectoryPath) {
    //     printf("%d: %s\n", directoryPathCtr + 1, directoryPath[directoryPathCtr]);
    //     directoryPathCtr++;
    // }
    // printf("\n");

    /* routine to print out the root directory entries */
    // printf("directoryEntries:\n");
    int directoryEntriesCtr = 0;
    int numDirectoryEntries = numRootDirEntries;
    // while (directoryEntriesCtr < numDirectoryEntries) {
    //     if (directoryEntries[directoryEntriesCtr]->status != 0) {
    //         printf("%d: (%s)\n", directoryEntriesCtr + 1, directoryEntries[directoryEntriesCtr]->filename);
    //     }
    //     directoryEntriesCtr++;
    // }
    // printf("\n");

    /* routine to find if subdir exists in root directory */
    // printf("Looking for subdir...\n");
    struct dir_entry_t **currentDirectoryEntries = directoryEntries;
    directoryPathCtr = 0;
    directoryEntriesCtr = 0;
    int match = 0;
    numCurrentDirEntries = numRootDirEntries;
    while (directoryPathCtr < numTokensDirectoryPath) {
        
        // todo: get info on dir or subdir
        directoryEntriesCtr = 0;
        // numDirectoryEntries = arrayLength(currentDirectoryEntries);
        numDirectoryEntries = numCurrentDirEntries;

        while (directoryEntriesCtr < numDirectoryEntries) {
            // printf("%d: comparing (%s) with (%s)\n", directoryPathCtr, directoryPath[directoryPathCtr], directoryEntries[directoryEntriesCtr]->filename);
            if (strcmp( (char *) directoryPath[directoryPathCtr], (char *) currentDirectoryEntries[directoryEntriesCtr]->filename) == 0) {
                // printf("match!\n");
                match = 1;
                break;
            }
            directoryEntriesCtr++;
        }
        
        // todo: add conditition to check if subdir cant be found (for any case?) 
        if (match == 1) {
            // check if not a directory
            if ((currentDirectoryEntries[directoryEntriesCtr]->status & 0b0100) != 0b0100) {
                printf("\nError: %s is not a directory\n", currentDirectoryEntries[directoryEntriesCtr]->filename);
                break;
            }
            // get dir entries for current subdir
            currentDirectoryEntries = analyzeSubDirectory(currentDirectoryEntries[directoryEntriesCtr]);
            // check if end of search
            if (directoryPathCtr == numTokensDirectoryPath - 1) {
                displayDirectoryEntries(currentDirectoryEntries);
                break;
            }
            match = 0;
        } else {
            printf("\nError: Subdirectory %s does not exist\n", directoryPath[directoryPathCtr]);
            break;
        }

        directoryPathCtr++;
    }
    printf("\n");

    if (currentDirectoryEntries != directoryEntries) {
        free(currentDirectoryEntries);
    }
    free(directoryEntries);
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        perror("Please include .img file\n");
        exit(EXIT_FAILURE);
    }
    
    fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        perror("Error: Could not open file\n");
    }

    buffer1 = (char *) calloc(1, sizeof(char));
    buffer2 = (char *) calloc(2, sizeof(char));
    buffer4 = (char *) calloc(4, sizeof(char));
    buffer8 = (char *) calloc(8, sizeof(char));
    if (buffer1 == NULL || buffer2 == NULL || buffer4 == NULL || buffer8 == NULL) {
        perror("Memory not allocated!\n");
        exit(EXIT_FAILURE);
    }

    
    readSuperBlockInfoFromImgFile();
    
    // displaySuperBlockInfo();

    // analyzeRootDirectory();
    analyzeDirectory(argv[2]);


    // clean up and exit
    free(buffer1);
    free(buffer2);
    free(buffer4);
    free(buffer8);
    fclose(fp);
    return 0;
}
