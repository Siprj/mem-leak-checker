#include <QCoreApplication>
#include <QStringList>
#include <iostream>

#include <QByteArray>
#include <QFile>
#include <QList>

#include "../common-headers/common-enums.h"


typedef struct __attribute__ ((packed)) {
    void* address;
    int memorySize;
    void* backtrace[4];
}mallocStructure;


QList<mallocStructure> mallocList;
QList<void*> freeList;


#include <QDebug>
#include <sstream>


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QStringList argumentList = a.arguments();

    for(int i = 0; i < argumentList.size(); i++)
    {
        std::cout<<argumentList[i].toStdString()<<std::endl;
    }


    QFile file("/mnt/Programing/build-mem-leak-checker-Desktop-Debug/test-app/m"
               "em-leak-analysis.bin");
    if (!file.open(QIODevice::ReadOnly))
    {
        qCritical()<<"file can(t be opened\n";
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
            file.read((char*)&mallocStruc.memorySize, 4);
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
                stream<<"addr2line -e /mnt/Programing/build-mem-leak-checker-De"
                        "sktop-Debug/test-app/test-app "
                      <<mallocList.at(i).backtrace[y];
                system(stream.str().c_str());
            }
            qDebug()<<"\n";
        }
        freeList.removeOne(mallocList.at(i).address);
    }


    //return a.exec();
}
