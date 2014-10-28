#include <iostream>
#include <getopt.h>
#include <list>
#include <sstream>
#include <cstdint>
#include <assert.h>
#include <fstream>
#include <algorithm>
#include <unistd.h>
#include <regex>


#include "../common-headers/common-enums.h"



typedef struct{
    u_int64_t address;
    u_int64_t memorySize;
    u_int64_t backtrace;
}mallocStructure;


struct GlobalOptions{
    std::string addr2line;
    std::string application;
    std::string file;
    bool reduceOutput;
}globalOptions;


void printUsage(char *argv[])
{
    printf("Usage: %s -f <input file> -a <tested application> -l <platform addr2line> \n\n"
           "-a --application    application which will be used as parameter to addr2line\n"
           "-l --addr2line      addr2line specific for platform\n"
           "-f --file           file which will be used as source data\n"
           "-h --help           show this message\n"
           "-r --reduced        reduce output (no duplicates)\n", argv[0]);
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
            {"reduce",      no_argument,       NULL, 'r'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        int c = getopt_long(argc, argv, "a:l:f:hr", long_options, &option_index);

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
        case 'r':
            globalOptions.reduceOutput = true;
            break;
        default:
            std::cerr<<"Unknown argument\n";
            abort();
        }
    }
}


// Unique cheker for mallocStructure.
bool sameBacktrace(mallocStructure &first, mallocStructure &second)
{
    return first.backtrace == second.backtrace;
}


// Comparison backtrace from mallocStructure.
bool compareBacktrace(mallocStructure &first, mallocStructure &second)
{
    return first.backtrace < second.backtrace;
}


int getFileAndLine(std::string &line, int32_t number, u_int64_t backtrace)
{
    std::stringstream stream;
    stream<<globalOptions.addr2line.c_str()<<" -e "
          <<globalOptions.application.c_str()<<" 0x"<<std::hex
          <<backtrace;

    FILE *f = popen(stream.str().c_str(), "r");
    if(f == 0)
    {
        std::cerr<<"Could not execute\n";
        return 1;
    }
    const int BUFSIZE = 1000;
    char buf[BUFSIZE];

    std::cmatch match;
    std::cerr<<"kvakva\n";
    std::cerr.flush();
    //^.*(cpp|c|h|hpp):[[:digit:]].*$
    std::regex regularExpresion("^.*(cpp|c|h|hpp):[[:digit:]].*$");
    while(fgets(buf, BUFSIZE, f))
    {
        std::cerr<<(stdout, "%s\nahoj:\n", buf);
        std::cerr.flush();
        std::regex_search(buf, match, regularExpresion);
        for(auto out : match)
        {
            std::cout<<out<<"\n\n";
        }
    }
    pclose(f);
    std::cerr<<"\n";
    return 0;
}


int readData(std::list<mallocStructure> &mallocList,
             std::list<u_int64_t> &freeList)
{
    u_int8_t data[100];
    std::ifstream file;
    file.open(globalOptions.file, std::fstream::in|std::fstream::binary);
    if (!file.is_open())
    {
        std::cerr<<"file: "<<globalOptions.file<<" can't be opened\n";
        return 1;
    }

    file.read((char*)data, 1);        // read first byte of header
    int pointerSizeInBytes = *(u_int8_t*)data;
    if(pointerSizeInBytes != 8 && pointerSizeInBytes != 4)
    {
        std::cerr<<"Wrong data format\n";
        return 1;
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
    return 0;
}


int findLeaks(std::list<mallocStructure> &mallocList,
              std::list<u_int64_t> &freeList,
              std::list<mallocStructure> &leakList)
{
    std::list<u_int64_t>::iterator it;
    for(mallocStructure mallocData : mallocList)
    {
        it = std::find(freeList.begin(), freeList.end(), mallocData.address);
        if(it == freeList.end())
            leakList.push_back(mallocData);
        else
            freeList.erase(it);
    }
    return 0;
}


int printOutput(std::list<mallocStructure> &leakList)
{
    for(mallocStructure leakMallocStructure : leakList)
    {
        std::cerr<<"*******************************************************"
                   "\n";
        std::cerr<<"memory leak at address: 0x"<<std::hex<<leakMallocStructure.address
               <<" size of memory leak: "<<std::dec<<leakMallocStructure.memorySize
               <<" bytes\nreturn address: 0x"<<std::hex
               <<leakMallocStructure.backtrace<<"\n\n";
        std::string line;
        int32_t lineNumber;
        if(getFileAndLine(line, lineNumber, leakMallocStructure.backtrace))
            return 1;

    }
}


int printReducedOutput(std::list<mallocStructure> &leakList)
{
    leakList.sort(compareBacktrace);
}


int main(int argc, char *argv[])
{
    std::list<mallocStructure> mallocList;
    std::list<mallocStructure> leakList;
    std::list<u_int64_t> freeList;

    parseOptions(argc, argv);
    if(globalOptions.addr2line.empty() | globalOptions.application.empty() |
            globalOptions.file.empty())
    {
        printf("Some parameter is missing!!!!\n");
        printUsage(argv);
        return 1;
    }

    if(readData(mallocList, freeList))
        return 1;

    if(findLeaks(mallocList, freeList, leakList))
        return 1;

//    leakList.unique(sameBacktrace);

    if(globalOptions.reduceOutput)
        printReducedOutput(leakList);
    else
        printOutput(leakList);


    //return a.exec();
}

