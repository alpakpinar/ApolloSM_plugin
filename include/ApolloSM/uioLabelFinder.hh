#ifndef __UIO_LABEL_FINDER_HH__
#define __UIO_LABEL_FINDER_HH__
#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <string.h>
#include <boost/filesystem.hpp>

using namespace boost::filesystem;

static size_t ReadFileToBuffer(std::string const & fileName,char * buffer,size_t bufferSize){
  //open the file
  FILE * inFile = fopen(fileName.c_str(),"r");
  if(NULL == inFile){
    //failed to opn file.
    return 0;
  }

  //read file  
  if(fgets(buffer,bufferSize,inFile) == NULL){
    fclose(inFile);
    return 0;
  }
  fclose(inFile);
  return 1;
}

// A function that takes a uio label and returns the uio number
// DEPRECATED - with the kernel patch the uio name is set by the "linux,uio-name" device-tree property -> ex: "uio_K_C2C_PHY"
// /dev/uio_NAME is a symlink that points to the actual uioN device file:  /dev/uio_NAME -> /dev/uioN
int label2uio_old(std::string ilabel)
{
  size_t const bufferSize = 1024;
  char * buffer = new char[bufferSize];
  memset(buffer,0x0,bufferSize);

  bool foundValidMatch = false;
  uint32_t dtEntryAddr=0, uioEntryAddr=0;
  // search through the file system to see if there is a uio that matches the name
  std::string const uiopath = "/sys/class/uio/";
  std::string const dvtpath = "/proc/device-tree/amba_pl/";


  // traverse through the device-tree
  for (directory_iterator itDir(dvtpath); itDir!=directory_iterator(); ++itDir){
    //We only want to open label files, so we search for "/path/to/"+"label", not the file itself
    if (!is_directory(itDir->path())) {
      //This is a file, so not what we want in our search
      continue;
    }
    
    if (!exists(itDir->path()/"label")) {
      //This directory does not contain a file named "label"
      continue;
    }

    //path has a file named label in it.
    //open file and read its contents into buffer;
    if(!ReadFileToBuffer(itDir->path().native()+"/label",buffer,bufferSize)){
      //bad read
      continue;
    }

    if(!strcmp(buffer, ilabel.c_str())){
      //get dtEntryAddr from the file name
      int namesize=itDir->path().filename().native().size();
      if(namesize<10) {
	std::cout<< "directory name "<< itDir->path().filename().native().c_str() <<" has incorrect format." <<std::endl;
	continue; //expect the name to be in x@xxxxxxxx format for example myReg@0x41200000
      }
      dtEntryAddr = std::strtoul( itDir->path().filename().native().substr(namesize-8,8).c_str() , 0, 16);
      break;
    }
  }

  //Check if we found a device with the correct name
  if(dtEntryAddr==0) {
    std::cout<<"Cannot find a device that matches label "<<(ilabel).c_str()<<" device not opened!" << std::endl;
    return -1;
  }
  
  // Traverse through the /sys/class/uio directory
  for (directory_iterator itDir(uiopath); itDir!=directory_iterator(); ++itDir){
    //same kind of search as above, just looking for a uio dir with maps/map0/addr and maps/map0/size
    if (!is_directory(itDir->path())) {
      continue;
    }
    if (!exists(itDir->path()/"maps/map0/addr")) {
      continue;
    }
    if (!exists(itDir->path()/"maps/map0/size")) {
      continue;
    }

    //process address of UIO entry
    if(!ReadFileToBuffer((itDir->path()/"maps/map0/addr").native(),buffer,bufferSize)){
      //bad read
      continue;
    }
    uioEntryAddr = std::strtoul(buffer, 0, 16);

    // see if the UIO address matches the device tree address
    if (dtEntryAddr == uioEntryAddr){
      if(!ReadFileToBuffer((itDir->path().native()+"/maps/map0/size"),buffer,bufferSize)){
	//bad read
	continue;
      }
      
      //the size was in number of bytes, convert into number of uint32
      //size_t size=std::strtoul( buffer, 0, 16)/4;  
      strcpy(buffer,itDir->path().filename().native().c_str());
      foundValidMatch = true;
      break;
    }
  }

  //Did we find a 
  if (!foundValidMatch){
    return -1;
  }
  
  char* endptr;
  int uionumber = strtol(buffer+3,&endptr,10);
  if (uionumber <0){
    return uionumber;
  }
  return uionumber;
}

// new, simple uionumber finder based on kernel patch
int label2uio(std::string ilabel)
{
  std::string prefix = "/dev/";
  std::string uioname = "uio_" + ilabel;
  std::string deviceFile;
  int uionumber;

  char* UIO_DEBUG = getenv("UIO_DEBUG");
  if (NULL != UIO_DEBUG) {
    printf("searching for /dev/uio_%s symlink\n", ilabel.c_str());
  }

  for (directory_iterator itUIO(prefix); itUIO != directory_iterator(); ++itUIO) {
    if ((is_directory(itUIO->path())) || (itUIO->path().string().find(uioname)==std::string::npos)) {
      continue;
    }
    else {
      // found /dev/uio_name, resolve symlink and get the UIO number
      if (is_symlink(itUIO->path())) {
        if (NULL != UIO_DEBUG) {
          printf("resolved symlink: /dev/%s -> /dev/uioN\n", uioname.c_str());
        }
        // deviceFile will be a string of form "uioN"
        deviceFile = read_symlink(itUIO->path()).string();
      }
      else {
        if (NULL != UIO_DEBUG) {
          printf("unable to resolve symlink /dev/%s -> /dev/uioN, using legacy method\n", uioname.c_str());
        }
        return -1;
      }
    }
  }

  // get the number from any digits after "uio"
  char* endptr;
  uionumber = strtol(deviceFile.substr(3,std::string::npos).c_str(), &endptr, 10);
  if (uionumber < 0) {
    return uionumber;
  }
  return uionumber;
}

#endif
