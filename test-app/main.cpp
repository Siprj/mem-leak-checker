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
    TestObject *b3 = new TestObject;
    cout<<"Object created"<<endl;

    cout<<"malloc in main\n";
    int *i = (int*)malloc(sizeof(int));
    cout<<"free in main\n";
    free(i);

    cout<<"Create dynamic array";
    int *pole = new int[20];
    cout<<"Delet dynamic array\n";
    delete []pole;
    cout<<"Delete objects\n";

    delete b1;
    delete b2;
    delete b3;

    return 0;
}

