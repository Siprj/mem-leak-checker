#include <iostream>
#include <getopt.h>
#include <list>
#include <sstream>
#include <cstdint>
#include <assert.h>
#include <fstream>
#include <algorithm>
#include <unistd.h>


#include "../common-headers/common-enums.h"



typedef struct __attribute__ ((packed)) {
    u_int64_t address;
    u_int64_t memorySize;
    u_int64_t backtrace;
}mallocStructure;


std::list<mallocStructure> mallocList;
std::list<u_int64_t> freeList;


struct GlobalOptions{
    std::string addr2line;
    std::string application;
    std::string file;
}globalOptions;

void printUsage(char *argv[])
{
    printf("Usage: %s -f <input file> -a <tested application> -l <platform addr2line> \n\n"
           "-a --application    application which will be used as parameter to addr2line\n"
           "-l --addr2line      addr2line specific for platform\n"
           "-f --file           file which will be used as source data\n"
           "-h --help           show this message\n", argv[0]);
}

void parseOptions(int argc, char *argv[])
{
    while (1)
    {
        static struct option long_options[] =
        {
            {"application", required_argument, NULL, 'a'},
            {"addr2line",   required_argument, NULL, 'l'},
            {"file",        required_argument, NULL, 'f'},
            {"help",        no_argument,       NULL, 'h'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        int c = getopt_long(argc, argv, "a:l:f:h", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
        case 'a':
            globalOptions.application.append(optarg);
            break;
        case 'l':
            globalOptions.addr2line.append(optarg);
            break;
        case 'f':
            globalOptions.file.append(optarg);
            break;
        case 'h':
            printUsage(argv);
            exit(0);
            break;
        default:
            abort();
        }
    }
}


int main(int argc, char *argv[])
{
    u_int8_t data[100];

    parseOptions(argc, argv);
    if(globalOptions.addr2line.empty() | globalOptions.application.empty() |
            globalOptions.file.empty())
    {
        printf("Some parameter is missing!!!!\n");
        printUsage(argv);
        exit(1);
    }


    std::ifstream file;
    file.open(globalOptions.file, std::fstream::in|std::fstream::binary);
    if (!file.is_open())
    {
        std::cerr<<"file: "<<globalOptions.file<<" can't be opened\n";
        return 1;
    }

    file.read((char*)data, 1);        // read first byte of header
    int pointerSizeInBytes = *(u_int8_t*)data;
    std::cerr<<"Pointer size: "<<pointerSizeInBytes<<std::endl;
    if(pointerSizeInBytes != 8 && pointerSizeInBytes != 4)
    {
        std::cerr<<"Wrong data format\n";
        exit(1);
    }

    // Check if all data are readed
    while(file.good())
    {
        file.read((char*)data, 1);
        switch((int)*data)
        {
        case CHUNK_TYPE_ID_MALLOC:
            mallocStructure mallocStruc;
            file.read((char*)&mallocStruc.address, pointerSizeInBytes);
            file.read((char*)&mallocStruc.memorySize, 8);
            file.read((char*)&mallocStruc.backtrace, pointerSizeInBytes);
            mallocList.push_back(mallocStruc);
            break;
        case CHUNK_TYPE_ID_FREE:
            u_int64_t freeDataAddress;
            file.read((char*)&freeDataAddress, pointerSizeInBytes);
            freeList.push_back(freeDataAddress);
            break;
        case CHUNK_TYPE_ID_CALLOC:
            std::cout<<"calloc chunk\n";
            file.read((char*)&mallocStruc.address, pointerSizeInBytes);
            u_int32_t numberOfMembers;
            u_int32_t sizeOfMember;
            file.read((char*)&numberOfMembers, sizeof(numberOfMembers));
            file.read((char*)&sizeOfMember, sizeof(sizeOfMember));
            mallocStruc.memorySize = numberOfMembers * sizeOfMember;
            file.read((char*)&mallocStruc.backtrace, pointerSizeInBytes);
            mallocList.push_back(mallocStruc);
            break;
        case CHUNK_TYPE_ID_REALLOC:
            std::cout<<"realloc chunk\n";
            file.read((char*)&mallocStruc.address, pointerSizeInBytes);
            file.read((char*)&freeDataAddress, pointerSizeInBytes);
            file.read((char*)&mallocStruc.memorySize, 8);
            file.read((char*)&mallocStruc.backtrace, pointerSizeInBytes);
            freeList.push_back(freeDataAddress);
            mallocList.push_back(mallocStruc);
            break;
        case CHUNK_TYPE_ID_MEMALIGN:
            file.read((char*)&mallocStruc.address, pointerSizeInBytes);
            file.read((char*)&mallocStruc.memorySize, 8);
            file.seekg(4, std::ios_base::cur);              // skip 4 bytes
            file.read((char*)&mallocStruc.backtrace, pointerSizeInBytes);
            mallocList.push_back(mallocStruc);
            break;
        }
    }

    std::list<u_int64_t>::iterator it;

    for(mallocStructure mallocData : mallocList)
    {
        it = std::find(freeList.begin(), freeList.end(), mallocData.address);
        if(it == freeList.end())
        {
            std::cerr<<"*******************************************************"
                       "\n";
            std::cerr<<"memory leak at address: 0x"<<std::hex<<mallocData.address
                   <<" size of memory leak: "<<std::dec<<mallocData.memorySize
                   <<" bytes\nreturn address: 0x"<<std::hex
                   <<mallocData.backtrace<<"\n\n";

            std::stringstream stream;
                stream<<globalOptions.addr2line.c_str()<<" -e "
                      <<globalOptions.application.c_str()<<" 0x"<<std::hex
                      <<mallocData.backtrace;
                std::cerr<<"system call: "<<stream.str()<<std::endl;
                std::cerr.flush();
                int ret = system(stream.str().c_str());
                (void)ret;
                usleep(50000);
            std::cerr<<"\n";
        }
        else
        {
            freeList.erase(it);
        }
    }


    //return a.exec();
}

