#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "disk.h"

struct SuperBlockInfo superBlockInfo;
char *buffer1, *buffer2, *buffer4, *buffer8;
FILE* fp;

int numFreeBlocks = 0, numReservedBlocks = 0, numAllocatedBlocks = 0;


void readSuperBlockInfoFromImgFile() {

    // read file system identifier
    fread(buffer8, sizeof(char), 8, fp);
    memcpy(superBlockInfo.fileSystemIdentifier, buffer8, sizeof(char) * 8);
    
    // read block size
    fread(buffer2, sizeof(char), 2, fp);
    superBlockInfo.blockSize = buffer2[0] << 8 | buffer2[1];

    // read block count
    fread(buffer4, sizeof(char), 4, fp);
    superBlockInfo.blockCount = (__uint64_t) buffer4[0] << 32 | buffer4[1] << 16 | buffer4[2] << 8 | buffer4[3];

    // read FAT starts
    fread(buffer4, sizeof(char), 4, fp);
    superBlockInfo.FATBlockStart = (__uint64_t) buffer4[0] << 32 | buffer4[1] << 16 | buffer4[2] << 8 | buffer4[3];

    // read FAT block count
    fread(buffer4, sizeof(char), 4, fp);
    superBlockInfo.FATBlocks = (__uint64_t) buffer4[0] << 32 | buffer4[1] << 16 | buffer4[2] << 8 | buffer4[3];

    // read read dir starts
    fread(buffer4, sizeof(char), 4, fp);
    superBlockInfo.rootBlockStart = (__uint64_t) buffer4[0] << 32 | buffer4[1] << 16 | buffer4[2] << 8 | buffer4[3];

    // read read dir block count
    fread(buffer4, sizeof(char), 4, fp);
    superBlockInfo.rootBlocks = (__uint64_t) buffer4[0] << 32 | buffer4[1] << 16 | buffer4[2] << 8 | buffer4[3];
    
}

// TODO: replace FATEntryValue data type to uint possibly?
void analyzeFAT() {
    int FATEntryValue;
    
    int FATEntryAddress = superBlockInfo.FATBlockStart * blockSize;
    int FATExitPoint = (superBlockInfo.FATBlockStart + superBlockInfo.FATBlocks) * blockSize;

    // loop superBlockInfo.FATBlocks - 1 number of times
    while (FATEntryAddress < FATExitPoint) {
        // point fp to FAT entry
        fseek(fp, FATEntryAddress, SEEK_SET);
        // read FAT entry
        fread(buffer4, sizeof(char), 4, fp);
        FATEntryValue = (__uint64_t) buffer4[0] << 32 | buffer4[1] << 16 | buffer4[2] << 8 | buffer4[3];
        if (FATEntryValue == 0) {
            numFreeBlocks++;
        } else if (FATEntryValue == 1) {
            numReservedBlocks++;
        } else {
            numAllocatedBlocks++;
            // printf("FAT entry(0x%08lx0): 0x%08x\n", ftell(fp)/16, FATEntryValue);
        }
        FATEntryAddress += 4;
    }
    
}


void displaySuperBlockInfo() {
    printf("Super block information:\n");
    printf("Block size: %d\n", superBlockInfo.blockSize);
    printf("Block count: %d\n", superBlockInfo.blockCount);
    printf("FAT starts: %d\n", superBlockInfo.FATBlockStart);
    printf("FAT blocks: %d\n", superBlockInfo.FATBlocks);
    printf("Root directory start: %d\n", superBlockInfo.rootBlockStart);
    printf("Root directory blocks: %d\n", superBlockInfo.rootBlocks);
    printf("\nFAT information:\n");
    printf("Free blocks: %d\n", numFreeBlocks);
    printf("Reserved blocks: %d\n", numReservedBlocks);
    printf("Allocated blocks: %d\n", numAllocatedBlocks);
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

    // read .img file
    readSuperBlockInfoFromImgFile();

    // analyze FAT for block value status
    analyzeFAT();

    // print super block info
    displaySuperBlockInfo();

    // clean up and exit
    free(buffer1);
    free(buffer2);
    free(buffer4);
    free(buffer8);
    fclose(fp);
    return 0;
}
