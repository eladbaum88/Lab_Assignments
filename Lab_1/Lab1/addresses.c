#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int addr5; // global variable in data segment
int addr6; // global variable in data segment

int foo() // function in text segment
{
    return -1;
}
void point_at(void *p); // forward declaration
void foo1(); // forward declaration
char g = 'g'; // global variable in data segment
void foo2(); // forward declaration

int secondary(int x) 
{
    int addr2; // local variable stack frame
    int addr3; // local variable stack frame    
    char *yos = "ree"; // string literal in read-only memory 
    int *addr4 = (int *)(malloc(50)); // allocate 50 bytes on heap

	// T1d - pointers and arrays
	int iarray2[] = {1,2,3}; // local array on stack
    char carray2[] = {'a','b','c','d'}; // local array on stack
    int* iarray2Ptr = iarray2; 
    char* carray2Ptr = carray2; 
    int* p; // uninitialized pointer variable

    
	printf("- &addr2: %p\n", &addr2); 
    printf("- &addr3: %p\n", &addr3);
    printf("- foo: %p\n", &foo);
    printf("- &addr5: %p\n", &addr5);
	printf("Print distances:\n");
    point_at(&addr5);

    // T1b - arrays defenitions
    int iarray[2]; // local array on stack
    float farray[2]; // local array on stack
    double darray[2]; // local array on stack
    char carray[2]; // local array on stack 

    printf("Print more addresses:\n");
    printf("- &addr6: %p\n", &addr6);
    printf("- yos: %p\n", yos);
    printf("- gg: %p\n", &g);
    printf("- addr4: %p\n", addr4);
    printf("- &addr4: %p\n", &addr4);

    printf("- &foo1: %p\n", &foo1);
    printf("- &foo1: %p\n", &foo2);
    
    printf("Print another distance:\n");
    printf("- &foo2 - &foo1: %ld\n", (long) (&foo2 - &foo1));

   
    printf("Arrays Mem Layout (T1b):\n");

    /* task 1 b here */
    printf("  iarray:   %p\n", (void*)iarray);
    printf("  iarray+1: %p\n", (void*)(iarray + 1));

    printf("  farray:   %p\n", (void*)farray);
    printf("  farray+1: %p\n", (void*)(farray + 1));

    printf("  darray:   %p\n", (void*)darray);  
    printf("  darray+1: %p\n", (void*)(darray + 1));

    printf("  carray:   %p\n", (void*)carray);
    printf("  carray+1: %p\n", (void*)(carray + 1));
    
    printf("Pointers and arrays (T1d): \n");

    /* task 1 d here */
    printf("iarray2 values \n");
    for (int i = 0; i < 3; i++) {
        printf("%d \n", *(iarray2Ptr + i));
    }
    printf("carray2 values \n");
    for (int i = 0; i < 4; i++) {
        printf("%c \n", *(carray2Ptr + i));
    }
    printf("Uninitialized pointer p: %p\n", p); // prints garbage value in p
}

int main(int argc, char **argv)
{

    printf("Print function argument addresses:\n");

    printf("- &argc %p\n", &argc); // address of argc
    printf("- argv %p\n", argv); // address of argv
    printf("- &argv %p\n", &argv); // address of argv pointer
	
	secondary(0);
    
    printf("Command line arg addresses (T1e):\n");
    /* task 1 e here */
     for (int i = 0; i < argc; i++) {
        printf("  argv[%d] (pointer): %p\n", i, (void*)argv[i]);
        printf("  &argv[%d] (in argv array): %p\n", i, (void*)&argv[i]);
        printf("  argv[%d] content: %s\n", i, argv[i]);
    }
    
    return 0;
}

void point_at(void *p)
{
    int local; // local variable stack frame
    static int addr0 = 2; // static variable in data segment
    static int addr1; // static variable in data segment

    long dist1 = (size_t)&addr6 - (size_t)p; // p=&addr5
    //dist1: “How many bytes apart are the two global ints addr5 and addr6

    long dist2 = (size_t)&local - (size_t)p; // How far is the local stack variable local from the global addr5

    long dist3 = (size_t)&foo - (size_t)p; // How far is the function foo (in code) from the global addr5?”

    printf("- dist1: (size_t)&addr6 - (size_t)p: %ld\n", dist1); 
    printf("- dist2: (size_t)&local - (size_t)p: %ld\n", dist2);
    printf("- dist3: (size_t)&foo - (size_t)p:  %ld\n", dist3);
    
    printf("Check long type mem size (T1a):\n");
    /* task 1 a here */ 
    printf("- sizeof(long)  = %zu\n", sizeof(long));
    printf("- sizeof(void*) = %zu\n", sizeof(void*));

    printf("- addr0: %p\n", &addr0);
    printf("- addr1: %p\n", &addr1);
}

void foo1()
{
    printf("foo1\n");
}

void foo2()
{
    printf("foo2\n");
}