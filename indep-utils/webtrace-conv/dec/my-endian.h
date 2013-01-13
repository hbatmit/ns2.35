#ifndef _ENDIAN_H_
#define _ENDIAN_H_

#include "proxytrace.h"

/* detects endian-ness */
int IsLittleEndian(void);

/* changes endian-ness */
void ToOtherEndian(TEntry *e);

#endif
