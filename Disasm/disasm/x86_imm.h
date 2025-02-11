#ifndef x86_IMM_H
#define x86_IMM_H

#include "qword.h"
#include <stdlib.h>
#include <sys/types.h>

/* these are in the global x86 namespace but are not a part of the
 * official API */
unsigned int x86_imm_sized( unsigned char *buf, size_t buf_len, void *dest,
			    unsigned int size );

unsigned int x86_imm_signsized( unsigned char *buf, size_t buf_len, void *dest,
				unsigned int size );
#endif
