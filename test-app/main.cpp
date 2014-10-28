#include <iostream>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>
#include <thread>
#include <list>

#define NUMBER_OF_THREADS 10

#define MEMORY_LEAK

using namespace std;

class TestObject{
public:
    TestObject()
    {
        cout<<"TestObject constructor\n";
        i = 0;
    }

    ~TestObject()
    {
        cout<<"TestObject destructor\n";
    }
private:
    int i;
};

void memLeakFunction()
{
    cout<<"Test app start!"<<endl;
    MEMORY_LEAK TestObject *b1 = new TestObject;
    TestObject *b2 = new TestObject;
    cout<<"Object created"<<endl;

    delete b2;
    b1 = new TestObject;

    MEMORY_LEAK TestObject *testObjectArray = new TestObject[10];
    (void)testObjectArray;

    cout<<"malloc in main\n";
    int *i = (int*)malloc(sizeof(int));
    cout<<"free in main\n";
    free(i);
    int *array = (int*)calloc(100, sizeof(int));
    MEMORY_LEAK int *array2 = (int*)realloc(array, 200*(sizeof(int)));
    (void)array2;

    cout<<"Create dynamic array";
    int *array3 = new int[20];
    cout<<"Delet dynamic array\n";
    cout<<"Delete objects\n";

    delete []array3;
    delete b1;
}


int main()
{
    list<thread> threadList;

    for(int i = 0; i < NUMBER_OF_THREADS; ++i)
    {
        threadList.push_back(thread(memLeakFunction));
    }
    for(thread &thread : threadList)
    {
        thread.join();
    }

    return 0;
}

