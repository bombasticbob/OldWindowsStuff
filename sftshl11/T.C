#include "stdio.h"
#include "stdlib.h"
#include "memory.h"
#include "dos.h"
#include "string.h"


main()
{
static char drive[8], dir[64], name[16], ext[8];

   _splitpath("\\\\CIPHER\\DASA\\STUPID\\JUNK\\HELLO.TXT", drive, dir, name, ext);

   printf("%s\n%s\n%s\n%s\n", drive, dir, name, ext);
}
