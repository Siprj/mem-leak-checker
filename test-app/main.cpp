#include <iostream>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>

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


int main()
{
    cout<<"Test app start!"<<endl;
    TestObject *b1 = new TestObject;
    TestObject *b2 = new TestObject;
    cout<<"Object created"<<endl;

    delete b2;
    b1 = new TestObject;

    TestObject *testObjectArray = new TestObject[10];
    (void)testObjectArray;

    cout<<"malloc in main\n";
    int *i = (int*)malloc(sizeof(int));
    cout<<"free in main\n";
    free(i);
    int *array = (int*)calloc(100, sizeof(int));
    (void)array;

    int *array2 = (int*)realloc(array, 200*(sizeof(int)));
    (void)array2;

    cout<<"Create dynamic array";
    int *array3 = new int[20];
    cout<<"Delet dynamic array\n";
    cout<<"Delete objects\n";

    delete []array3;
    delete b1;

    return 0;
}

