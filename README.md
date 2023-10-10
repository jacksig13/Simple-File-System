# Simple File System

This program implements a simple file system in a memory mapped file. The file system allows creating, listing, adding, removing, and extracting files.

## Files

**filefs.c** - Main driver program 

- Initializes file system 
- Parses command line arguments
- Calls file system functions based on arguments

**fs.c** - Implements file system functionality

- Defines file system structures
- File system initialization and formatting
- Functions to add, remove, list, and extract files

**fs.h** - Header file with constants, prototypes, and global variables

## Usage



The program takes the following command line arguments:

```
-l: List files in file system  
-a: Add a file to file system
-r: Remove a file from file system
-e: Extract a file from file system to stdout
-f: File system file name (required)
```

For example:

```
./fs -f fs1 -l
./fs -f fs1 -a file1.txt 
./fs -f fs1 -a file2.txt 
./fs -f fs1 -r file2.txt
./fs -f fs1 -e file1.txt
```

## File System Layout

The file system format consists of:

- **Superblock** - Metadata about file system 
- **Free block bitmap** - Tracks used/free blocks
- **Inode table** - Inode for each file 
- **Data blocks** - File contents

## Implementation 

- Uses mmap() to map file system file to memory
- Manages free blocks and inodes
- Direct block addressing for files
- Recursive lookup for path resolution
- Removed files free inodes and blocks

## Limitations

- Fixed 10MB file system size
- No permissions or multiple users
- Very simple implementation for demonstration

## Possible Improvements

- Support variable file system size
- Use indirect addressing for large files
- Cache frequently used blocks
- Store inode and free bitmaps in fixed locations
- Add subdirectories
- Add file permissions and ownership

Let me know if you would like any clarification or have additional questions!