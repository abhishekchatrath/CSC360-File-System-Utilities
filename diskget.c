#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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
struct dir_entry_t** readDirectoryEntries(int blockNum);

/**
 * ...
 */
struct dir_entry_t* findFilenameInFDT();

/**
 * ...
 */
struct dir_entry_t** getDirectoryEntriesForFile(char* filename);

/**
 * @param filename: The new name of the file to be copied onto Linux system.
 * @param directoryEntry: Contains the starting block number of the file to be copied to Linux.
 */
void createOutputFile(char* filename, struct dir_entry_t* directoryEntry);

/**
 * Read and return a list of directory entries from root_dir or sub_dir 
 */
struct dir_entry_t** analyzeDirectory();

/**
 * Each directory entry contains the starting block number for a file, say block number X.
 * To find the next block in the file, look at ***entry X*** in the FAT.
 * Eg: file starting block   X is entry (X %% 128) in block (X / 128) of FAT
 */
__uint32_t getNextBlock(__uint32_t currentBlock);



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
    
    fp = fopen(argv[1], "rb");
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

    // extract and separate filename from the path from argv[2]
    readInputFilePath(argv[2]);
    
    // search through rootdir or subdir for the provided filename
    struct dir_entry_t* directoryEntry = findFilenameInFDT();
    
    if (directoryEntry == NULL) {
        // printf("\nError [5]: Could not find %s in .img file\n", argv[2]);
        printf("\n" ANSI_COLOUR_BLUE "File not found." ANSI_COLOUR_RESET "\n");
        // return EXIT_FAILURE;
        return 0;
    }

    // copy contents of input file to a new output file on Linux
    createOutputFile(argv[3], directoryEntry);

    // clean up and exit
    free(directoryEntry);
    free(buffer1);
    free(buffer2);
    free(buffer4);
    free(buffer8);
    fclose(fp);

    return EXIT_SUCCESS;
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


struct dir_entry_t* findFilenameInFDT() {
    char* filename = (char *) calloc(31, sizeof(char));
    strcpy(filename, (char *) &directoryPath[numTokensDirectoryPath - 1]);

    // get a list of dir entries from root_dir or sub_dir 
    struct dir_entry_t** directoryEntries = getDirectoryEntriesForFile(filename);
    if (directoryEntries == NULL) {
        return NULL;
    }

    // check if filename exists in directoryEntries
    struct dir_entry_t* directoryEntry = NULL;
    int ctr = 0;
    while (ctr < numDirEntries) {
        
        // printf("%s\n", directoryEntries[ctr]->filename);
        
        if (strcmp(filename, (char *) directoryEntries[ctr]->filename) == 0) {
            // directoryEntry = directoryEntries[ctr];
            directoryEntry = (struct dir_entry_t *) calloc(1, sizeof(struct dir_entry_t));
            memcpy(directoryEntry, directoryEntries[ctr], sizeof(struct dir_entry_t));
            // printf("found file!\n");
            break;
        }
        ctr++;
    }

    // free(directoryEntries);
    free(filename);

    return directoryEntry;
}


struct dir_entry_t** getDirectoryEntriesForFile(char* filename) {
    struct dir_entry_t** directoryEntries = NULL;
    struct dir_entry_t* rootDirectory = (struct dir_entry_t *) calloc(1, sizeof(struct dir_entry_t));
    rootDirectory->starting_block = superBlockInfo.root_dir_start_block;
    rootDirectory->block_count = superBlockInfo.root_dir_block_count;
    directoryEntries = analyzeDirectory(rootDirectory);
    free(rootDirectory);

    // get dir entries from root
    if (numTokensDirectoryPath == 1) {
        return directoryEntries;
    } else {                                                            /* get dir entries from a subdir */
        struct dir_entry_t **currentDirectoryEntries = directoryEntries;
        int dirPathCtr = 0, dirEntriesCtr = 0, match = 0;
        // int numCurrentDirEntries = numDirEntries;

        while (dirPathCtr < numTokensDirectoryPath) {
            while (dirEntriesCtr < numDirEntries) {
                if (strcmp( (char *) directoryPath[dirPathCtr], (char *) currentDirectoryEntries[dirEntriesCtr]->filename) == 0) {
                    // printf("match!\n");
                    match = 1;
                    break;
                }
                dirEntriesCtr++;
            }
            if (match == 1) {
                // check if not a directory
                if ((currentDirectoryEntries[dirEntriesCtr]->status & 0b0100) != 0b0100) {
                    printf("\nError [6]: %s is not a directory\n", currentDirectoryEntries[dirEntriesCtr]->filename);
                    return NULL;
                }
                // get next level of dir entries
                currentDirectoryEntries = analyzeDirectory(currentDirectoryEntries[dirEntriesCtr]);
                
                // check if end of path depth
                if (dirPathCtr == numTokensDirectoryPath - 1) {
                    return currentDirectoryEntries;
                }

                // reset ctr and match case
                dirEntriesCtr = 0;
                match = 0;
            } else {
                printf("\nError [7]: Subdirectory %s does not exist\n", directoryPath[dirPathCtr]);                
                return NULL;
            }
            dirPathCtr++;
        }
    }
    return NULL;
}


struct dir_entry_t** analyzeDirectory(struct dir_entry_t* searchDirectoryEntry) {
    numDirEntries = searchDirectoryEntry->block_count * 8;
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


void createOutputFile(char* filename, struct dir_entry_t* directoryEntry) {

    // printf("\nfilename: %s\n", directoryEntry->filename);
    // printf("starting_block: %d\n", directoryEntry->starting_block);
    // printf("block_count: %d\n\n", directoryEntry->block_count);

    // create a file write pointer to the current directory of the Linux system
    FILE* fpw = fopen(filename, "wb");
    if (fpw == NULL) {
        printf("\nError [3]: Could not open file to write\n");
        exit(EXIT_FAILURE);
    }

    // go through each block of directoryEntry and copy its contents into the new file
    int currentBlockAddr = directoryEntry->starting_block * blockSize;
    int blockCtr = 0, bytesTransfered = 0;
    // printf("\nblock %d: %d %x\n", blockCtr, currentBlockAddr, currentBlockAddr);
    while (blockCtr < directoryEntry->block_count) {
        fseek(fp, currentBlockAddr, SEEK_SET);
        while (bytesTransfered < blockSize) {
            fread(buffer8, sizeof(char), 8, fp);
            fwrite(buffer8, sizeof(char), 8, fpw);
            bytesTransfered += 8;
            // printf("%02d:\t%s\n", bytesTransfered / 8, buffer8);      /* this will print out file contents to stdout for dev */
        }
        // printf("\n%d\n", bytesTransfered);
        // printf("\n\n");
        currentBlockAddr = getNextBlock(currentBlockAddr / blockSize) * blockSize;          // get next block from FAT
        bytesTransfered = 0;
        blockCtr++;
        // printf("\nblock %d: %d %x\n", blockCtr, currentBlockAddr, currentBlockAddr);
    }
    
    fclose(fpw);
}
