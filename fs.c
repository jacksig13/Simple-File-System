#include "fs.h"
#include "stdbool.h"
#include "string.h"

//superblock contains basic info about the file system
struct superBlock {
  int numBlocks; //how many data blocks the system holds
  int blockSize; //how much data (in bytes) each block can hold
  int numInodes; //how many inodes does the system hold
};

//freeblocklist stores whether each block index is used or empty
struct freeBlockList {
  int freeBlocks[10000]; //0 = free block, 1 = used block
};

//each file gets a designated inode with info about the file
struct Inode {
  bool inUse; //track whether inode is in use or not
  int fileSize; //max file size 51200 bytes (512 * 100 blocks)
  char fileType; //'d' = directory, 'f' = file
  int dataBlocks[100]; //store reference to block index in freeblocklist
};

//datablock holds the data
struct dataBlock {
  char data[512];
};

//directory holds the filename and inode number of a file (stored in parent directory inode)
struct directory {
  char fileName[255];
  int iNode;
};

struct superBlock *SB;
struct freeBlockList *FBL;
struct Inode *INODES;
struct dataBlock *BLOCKS;
unsigned char* fs;

void mapfs(int fd){
  //map file system to 'fs' pointer (10MB)
  if ((fs = mmap(NULL, FSSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == NULL){
      perror("mmap failed");
      exit(EXIT_FAILURE);
  }
  //assign pointers to each part of the file system in mapped memory
  SB = fs;
  FBL = fs + sizeof(struct superBlock);
  INODES = fs + sizeof(struct superBlock) + sizeof(struct freeBlockList);
  BLOCKS = fs + sizeof(struct superBlock) + sizeof(struct freeBlockList) + (sizeof(struct Inode) * NUMINODES);
}


void unmapfs(){
  munmap(fs, FSSIZE);
}


void formatfs() {
  printf("New file system created\n");
  //initialize superblock data
  SB->blockSize = 512;
  SB->numBlocks = 10000;
  SB->numInodes = 100;

  //initialize freeblocklist as empty besides block 1
  for(int i = 1; i < NUMBLOCKS; i++) {
    FBL->freeBlocks[i] = 0;
  }

  //initialize inodes as empty besides inode 1
  for(int i = 1; i < NUMINODES; i++) {
    INODES[i].inUse = false;
  }

  //set first inode as 'root' directory
  FBL->freeBlocks[0] = 1;
  INODES[0].inUse = true;
  INODES[0].fileType = 'd';
  INODES[0].fileSize = 0;
  INODES[0].dataBlocks[0] = 0;
  struct directory dir;
  strcpy(dir.fileName, "root");
  dir.iNode = 0;
  memcpy(BLOCKS[0].data, &dir, sizeof(struct dataBlock));

  //print file system size and section offsets
  printf("\tsizeof SB: %d, offset: %d, blocksize: %d, numblocks: %d, numInodes: %d\n", sizeof(struct superBlock), 0, SB->blockSize, SB->numBlocks, SB->numInodes);
  printf("sizeof FBL: %d, offset: %d\n", sizeof(struct freeBlockList), sizeof(struct superBlock));
  printf("sizeof INODE: %d x 100, offset: %d\n", sizeof(struct Inode), sizeof(struct superBlock) + sizeof(struct freeBlockList));
  printf("sizeof BLOCKS: %d x 10000, offset: %d\n", sizeof(struct dataBlock), sizeof(struct superBlock) + sizeof(struct freeBlockList) + (sizeof(struct Inode) * NUMINODES));
  printf("total size: %d\n", sizeof(struct superBlock) + sizeof(struct freeBlockList) + (sizeof(struct Inode) * NUMINODES) + (sizeof(struct dataBlock) * 10000));
}

void lsfs(int index, int indent){
  int i = index;
  int dataBlock = 0;
  
  // Check if the inode is in use
  if(INODES[i].inUse == false) {
    return;
  }
  
  // If the inode index is 0, assume it corresponds to the root directory and print its name
  if(i == 0) {
    struct directory dir;
    memcpy(&dir, BLOCKS[0].data, sizeof(&dir));
    printf("-%s\n", dir.fileName);
  }
  
  // Loop through the data blocks associated with the inode and print the name, inode number, and size of each file or directory
  while(dataBlock < 100) {
    if(INODES[i].dataBlocks[dataBlock] == 0) {
      dataBlock++;
      continue;
    }
    struct directory dir;
    int curBlock = INODES[i].dataBlocks[dataBlock];
    memcpy(&dir, BLOCKS[curBlock].data, sizeof(struct directory));
    for(int itr = 0; itr < indent; itr++) {
      printf("   ");
    }
    printf("-%s, inode: %d, size: %d\n", dir.fileName, dir.iNode, INODES[dir.iNode].fileSize);
    
    // If the file is a directory, recursively call the function with the inode number of the directory and an increased indent level
    if(INODES[dir.iNode].fileType == 'd') {
      lsfs(dir.iNode, indent + 1);
    }
    dataBlock++;
  }
}

// Finds first free block in file system
int findBlock() {
  int block = 0;
  
  // Loop through all blocks
  while(block < NUMBLOCKS) {
    // Check if block is free
    if(FBL->freeBlocks[block] == 0) {
      // Mark block as used
      FBL->freeBlocks[block] = 1;
      // Return block index  
      return block;
    }
    // Increment block
    block++;
  }
  // No free blocks found
  return -1;
}

// Finds first free inode in file system 
int findINode() {
  int iNode = 1;
  // Loop through all inodes, start at 1 
  while(iNode < NUMINODES) {
    // Check if inode is free
    if(!INODES[iNode].inUse) {
      // Mark inode as used
      INODES[iNode].inUse = true;
      // Return inode index
      return iNode;
    }
    // Increment inode
    iNode++;
  }
  // No free inodes
  printf("File system full! Remove files to add more.\n");
  exit(0);
}  

// Finds first free reference block for given inode
int findReference(int iNode) {
  int referenceBlock = 0;
  // Loop through reference blocks
  while(referenceBlock < 100) {
    // Check if free
    if(INODES[iNode].dataBlocks[referenceBlock] == 0) {
      // Return index of free reference block
      return referenceBlock;
    }
    // Increment reference block
    referenceBlock++;
  }
  // No free reference blocks
  return -1;
}

// Adds a new file to the file system
void addFile(char* fname) {
  // Find the first free block, inode, and reference block
  int emptyBlock = findBlock(); //empty block index in freeblocklist of 10000
  int emptyINode = find_iNode(); //empty inode index in inode list of 100
  int emptyRefBlock = 0; //block in directory inode that will store 'dir' struct for a file
  int parentNode = 0; //parent directory to store 'dir' struct in - initially 'root' (inode 0)
  int iNodeBlock = 1;

  // Open file to add
  FILE* file;
  file = fopen(fname, "r");
  if(!file) {
    perror("Error opening file");
    exit(1);
  }

  // Get file size
  fseek(file, 0L, SEEK_END);
  int size = ftell(file);
  fseek(file, 0L, SEEK_SET);

  // Calculate number of reads needed
  int numReads = (size / 512) + 1; 

  // Parse file path
  char* filePath[100];
  int pathDepth = 0;

  while(filePath[pathDepth] = strsep(&fname, "/")) {
    pathDepth++;
  }

  // If file is in root directory
  if(pathDepth < 2) {
    int readNum = 0;
    while(numReads > 0) {
      // Read file data into block
      fread(BLOCKS[emptyBlock].data, sizeof(struct dataBlock), 1, file);
      
      // Add block reference to inode
      INODES[emptyINode].dataBlocks[readNum] = emptyBlock;
      readNum++;
      numReads--;
      
      // Get next free block
      emptyBlock = findBlock();
    }
    // Create directory entry
    struct directory dir;
    strcpy(dir.fileName, filePath[0]);
    dir.iNode = emptyINode;

    // Update inode metadata
    INODES[emptyINode].fileSize = size; 
    INODES[emptyINode].fileType = 'f';

    // Add directory entry 
    memcpy(BLOCKS[emptyBlock].data, &dir, sizeof(struct dataBlock));

    // Update parent inode with reference to 
    emptyRefBlock = findReference(parentNode);
    INODES[parentNode].dataBlocks[emptyRefBlock] = emptyBlock;
  }
  // If file is in subdirectory
  else {
    int depthIndex = 0;
    while(pathDepth > 1) {
      // Create subdirectory entry
      struct directory dir;
      strcpy(dir.fileName, filePath[depthIndex]);
      dir.iNode = emptyINode;

      // Add entry to block
      memcpy(BLOCKS[emptyBlock].data, &dir, sizeof(struct dataBlock));

      // Update inode metadata
      INODES[emptyINode].fileType = 'd';
      INODES[emptyINode].inUse = true;
      INODES[emptyINode].fileSize += size;
      
      // Update parent inode
      emptyRefBlock = findReference(parentNode);
      INODES[parentNode].dataBlocks[emptyRefBlock] = emptyBlock;

      // Update variables
      parentNode = emptyINode;
      emptyBlock = findBlock();
      emptyINode = findINode();
      depthIndex++;
      pathDepth--;
    }
    // Read file data
    int readNum = 0;
    while(numReads > 0) {
      fread(BLOCKS[emptyBlock].data, sizeof(struct dataBlock), 1, file);
      INODES[emptyINode].dataBlocks[readNum] = emptyBlock;
      readNum++;
      numReads--;
      emptyBlock = findBlock();
    }
    // Create file directory entry
    struct directory dir;
    strcpy(dir.fileName, filePath[depthIndex]);
    dir.iNode = emptyINode;

    // Update inode metadata
    INODES[emptyINode].fileSize = size;
    INODES[emptyINode].fileType = 'f';

    // Add entry
    memcpy(BLOCKS[emptyBlock].data, &dir, sizeof(struct dataBlock));

    // Update parent inode 
    emptyRefBlock = findReference(parentNode);
    INODES[parentNode].dataBlocks[emptyRefBlock] = emptyBlock;
  }
  // Close file
  fclose(file);
}

// Global variables
int node = 0; // inode of desired file 
int parentDir[100]; // array to store parent directory inodes of desired file
int initialRefBlock = 0; // initial reference block in root used to remove file directory block

// Function to find inode of desired file in file system
// curDepth = 1 initially (1-indexed) 
int findFile(int curIndex, int pathDepth, int curDepth, char* filePath[]) {
  // Start at root inode
  int i = curIndex;
  // Initialize data block counter
  int dataBlock = 0;

  // Check if current inode is in use
  if(INODES[i].inUse == false) {
    return 0;
  }

  // Loop through data blocks of inode
  while(dataBlock < 100) {
    // Break if no more data blocks
    if(INODES[i].dataBlocks[dataBlock] == 0) {
      dataBlock++;
      continue;
    }

    // Read directory structure from current data block
    struct directory dir;
    int curBlock = INODES[i].dataBlocks[dataBlock];
    memcpy(&dir, BLOCKS[curBlock].data, sizeof(struct directory));

    // Store reference block if first level directory
    if(curDepth == 1) {
      initialRefBlock = dataBlock; 
    }
    // Check if filename matches 
    if(strcmp(dir.fileName, filePath[curDepth-1]) == 0) {
      // If last component (proper directory depth), return inode
      if(curDepth == pathDepth) {
        node = dir.iNode;
        return node;
      }
      // Otherwise recurse on child inode
      else {
        parentDir[curDepth] = dir.iNode; 
        findFile(dir.iNode, pathDepth, curDepth + 1, filePath);
      }
    }
    // Increment data block counter
    dataBlock++;
  }
  // File not found
  return 0; 
}

// Function to remove file from file system
void removeFile(char* fname) {
  // Parse file path
  char* filePath[100];
  int pathDepth = 0;

  while((filePath[pathDepth] = strsep(&fname, "/")) != NULL) {
    pathDepth++;
  }

  // Find inode of file
  findFile(0, pathDepth, 1, filePath);

  // Check if file found (node cannot be 0 since inode 0 is root)
  if(node == 0) {
    printf("File not found...\\n");
    exit(0);
  }

  // Mark inode unused and free data blocks
  INODES[node].inUse = false;
  int fileSize = INODES[node].fileSize;
  INODES[node].fileSize = 0;
  for(int i = 0; i < 100; i++) {
    if(INODES[node].dataBlocks[i] == 0) {
      break;
    }
    // Free data block
    int newFreeBlock = INODES[node].dataBlocks[i];
    FBL->freeBlocks[newFreeBlock] = 0;  
    INODES[node].dataBlocks[i] = 0;
  }

  // Update file sizes of parent directories & free unused data blocks
  for(int i = pathDepth - 1; i > 0; i--) {
    INODES[parentDir[i]].fileSize -= fileSize;
    //If parent directory has 0 size, remove directory too
    if(INODES[parentDir[i]].fileSize == 0) {
      INODES[parentDir[i]].inUse = false;
      for(int j = 0; j < 100; j++) {
        if(INODES[parentDir[i]].dataBlocks[j] == 0) {
          break;  
        }
        // Free data block
        int newFreeBlock = INODES[parentDir[i]].dataBlocks[j];
        FBL->freeBlocks[newFreeBlock] = 0;
        INODES[parentDir[i]].dataBlocks[j] = 0;
      }
    }
  }

  // Free root directory block if empty
  struct directory dir;
  int initialDirBlock = INODES[0].dataBlocks[initialRefBlock];

  memcpy(&dir, BLOCKS[initialDirBlock].data, sizeof(struct directory));

  if(INODES[dir.iNode].fileSize == 0) {
    INODES[0].dataBlocks[initialRefBlock] = 0;
    FBL->freeBlocks[initialDirBlock] = 0;
  }
}

// Function to extract file from file system
void extractFile(char* fname){
  // Parse file path to find depth
  char* filePath[100];
  int pathDepth = 0;

  while((filePath[pathDepth] = strsep(&fname, "/")) != NULL) {
    pathDepth++;
  }

  // Find inode 
  findFile(0, pathDepth, 1, filePath);

  // Check if file found (node cannot be 0 since inode 0 is root)
  if(node == 0) {
    printf("File not found...\\n");
    exit(0);
  }

  // Calculate number of reads
  int numReads = (INODES[node].fileSize / 512) + 1;
  size_t size = INODES[node].fileSize;

  // Read data blocks and write to stdout
  for(int i = 0; i < numReads; i++) {
    int curBlock = INODES[node].dataBlocks[i];
    if(size < 512) {
      fwrite(BLOCKS[curBlock].data, size, 1, stdout);
    } else {
      fwrite(BLOCKS[curBlock].data, sizeof(struct dataBlock), 1, stdout);
    }
    size -= 512;
  }
}