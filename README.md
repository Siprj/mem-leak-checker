# malloc-lib

## What is this library



## How to use this library

Can by used by setting up environment variable LD_PRELOAD to path of your libmalloc-lib library.
All programs started with this variable will load first libmalloc-lib and after then it will load
everything else.

LD_PRELOAD=/path/to/lib/libmalloc-lib.so.1.0.0
