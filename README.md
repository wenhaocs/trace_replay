# TraceReplayer
gcc replay.c -lrt

error:	  aio.cc:(.text+0x156): undefined reference to `aio_read'
solution: add "-lrt" to link command. -lrt is necessary here. 

error:	  ‘O_DIRECT’ undeclared (first use in this function)
solution: add #define _GNU_SOURCE before #include <fcntl.h>
