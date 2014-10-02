#ifndef ORIGINALMEMORYFNC_H
#define ORIGINALMEMORYFNC_H

// Pointers to function from libc (this calls are "replaced" by our functions).
// This is initialized by initMem Leak Checker.
// Variables definitions are in memory-hooks.cpp
extern void* (*libcCalloc)(size_t nmemb, size_t size);
extern void* (*libcMalloc)(size_t size);
extern void  (*libcFree)(void *ptr);
extern void* (*libcRealloc)(void *ptr, size_t size);
extern void* (*libcMemalign)(size_t blocksize, size_t bytes);

#endif // ORIGINALMEMORYFNC_H
