#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>

#include <errno.h>

#include <sys/mman.h>

#include "fs.h"

int zerosize(int fd);
void exitusage(char* pname);


int main(int argc, char** argv){
  
  int opt;
  int create = 0;
  int list = 0;
  int add = 0;
  int remove = 0;
  int extract = 0;
  char* toadd = NULL;
  char* toremove = NULL;
  char* toextract = NULL;
  char* fsname = NULL;
  int fd = -1;
  int newfs = 0;
  int filefsname = 0;
 

  /*
  Grabs input arguments using 'getopt':
    -l: list files in file system
    -a: add a file to file system
    -r: remove a file from file system
    -e: extract a file from file system
    -f: name of file system (required argument)
  */
  while ((opt = getopt(argc, argv, "la:r:e:f:")) != -1) {
    switch (opt) {
    case 'l':
      list = 1;
      break;
    case 'a':
      add = 1;
      toadd = strdup(optarg);
      break;
    case 'r':
      remove = 1;
      toremove = strdup(optarg);
      break;
    case 'e':
      extract = 1;
      toextract = strdup(optarg);
      break;
    case 'f':
      filefsname = 1;
      fsname = strdup(optarg);
      break;
    default:
      exitusage(argv[0]);
    }
  }
  
  //check for '-f' argument (file system name)
  if (!filefsname){
    printf("no fs name...\n");
    exitusage(argv[0]);
  }

  //Open file system in Read/Write mode - if new file system is defined it is created
  if ((fd = open(fsname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)) == -1){
    perror("open failed");
    exit(EXIT_FAILURE);
  }
  else{
    if (zerosize(fd)) { 
      newfs = 1;
    }
    
    //If the file system is new, lseek() and write() write a null byte to the end of the file. 
    //This is done to initialize the file system.
    if (newfs)
      if (lseek(fd, FSSIZE-1, SEEK_SET) == -1){
	      perror("seek failed");
	      exit(EXIT_FAILURE);
      }
      else{
        if(write(fd, "\0", 1) == -1){
          perror("write failed");
          exit(EXIT_FAILURE);
        }
      }
  }
  

  mapfs(fd);
  
  if (newfs){
    formatfs();
  }

  loadfs();
  
  if (add){
    addfilefs(toadd);
  }

  if (remove){
    removefilefs(toremove);
  }

  if (extract){
    extractfilefs(toextract);
  }

  if(list){
    lsfs(0, 1);
  }

  unmapfs();
  
  return 0;
}

int zerosize(int fd){
  struct stat stats;
  fstat(fd, &stats);
  if(stats.st_size == 0)
    return 1;
  return 0;
}

void exitusage(char* pname){
  fprintf(stderr, "Usage %s [-l] [-a path] [-e path] [-r path] -f name\n", pname);
  exit(EXIT_FAILURE);
}
