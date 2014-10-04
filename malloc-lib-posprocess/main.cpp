#include <QCoreApplication>
#include <QStringList>
#include <iostream>
#include <getopt.h>

#include <QByteArray>
#include <QFile>
#include <QList>

#include "../common-headers/common-enums.h"



typedef struct __attribute__ ((packed)) {
    void* address;
    u_int64_t memorySize;
    void* backtrace[4];
}mallocStructure;


QList<mallocStructure> mallocList;
QList<void*> freeList;


#include <QDebug>
#include <sstream>

struct GlobalOptions{
    std::string addr2line;
    std::string application;
    std::string file;
}globalOptions;

void parseOptions(int argc, char *argv[])
{
    while (1)
    {
        static struct option long_options[] =
        {
            /* These options set a flag. */
            {"application", required_argument, NULL, 'a'},
            {"addr2line",   required_argument, NULL, 'l'},
            {"file",        required_argument, NULL, 'f'},
            /* These options don’t set a flag.
                     We distinguish them by their indices. */
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
            printf("option -a\n%s\n", optarg);
            globalOptions.application.append(optarg);
            break;
        case 'l':
            globalOptions.addr2line.append(optarg);
            printf("option -l\n%s\n", optarg);
            break;
        case 'f':
            globalOptions.file.append(optarg);
            printf("option -f\n%s\n", optarg);
            break;
        case 'h':
            printf("Usage: %s -f <input file> -a <tested application> -l <platform addr2line> \n\n"
                   "-a --application    application which will be used as parameter to addr2line\n"
                   "-l --addr2line      addr2line specific for platform\n"
                   "-f --file           file which will be used as source data\n"
                   "-h --help           show this message\n", argv[0]);
            exit(0);
            break;
        default:
            abort();
        }
    }
}


int main(int argc, char *argv[])
{
    parseOptions(argc, argv);


    QFile file(globalOptions.file.c_str());
    if (!file.open(QIODevice::ReadOnly))
    {
        qCritical()<<"file can't be opened\n";
        return 1;
    }

    QByteArray array = file.read(1);        // read first byte of header
    int pointerSizeInBytes = *(u_int8_t*)array.data();
    qDebug()<<"Pointer size: "<<pointerSizeInBytes;
    array = file.read(1);                   // read second byte of header
    u_int8_t backtraceLength = *(u_int8_t*)array.data();
    qDebug()<<"Backtrace lenght: "<<backtraceLength;

    while(!file.atEnd())
    {
        QByteArray type = file.read(1);
        switch((int)*type.data())
        {
        case CHUNK_TYPE_ID_MALLOC:
            mallocStructure mallocStruc;
            file.read((char*)&mallocStruc.address, pointerSizeInBytes);
            file.read((char*)&mallocStruc.memorySize, 8);
            file.read((char*)mallocStruc.backtrace,
                      backtraceLength*pointerSizeInBytes);
            mallocList.append(mallocStruc);
            break;
        case CHUNK_TYPE_ID_FREE:
            void *address;
            file.read((char*)&address, pointerSizeInBytes);
            freeList.append(address);
            break;
        }
    }

    for(int i = 0; i < mallocList.size(); i++)
    {
        if(!freeList.contains(mallocList.at(i).address))
        {
            qDebug()<<"*******************************************************";
            qDebug()<<"memory leak at address: "<<mallocList.at(i).address
                   <<" size of memory leak: "<< mallocList.at(i).memorySize
                   <<" bytes\nback trace:\n";
            for(int y = 0; y < backtraceLength; y++)
            {
                std::stringstream stream;
                stream<<globalOptions.addr2line.c_str()<<" -e "
                     <<globalOptions.application.c_str()<<" "
                     <<mallocList.at(i).backtrace[y];
                system(stream.str().c_str());
            }
            qDebug()<<"\n";
        }
        freeList.removeOne(mallocList.at(i).address);
    }


    //return a.exec();
}
