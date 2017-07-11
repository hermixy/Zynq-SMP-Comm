#ifndef _LINUX_CIRC_BUF_H
#define _LINUX_CIRC_BUF_H 1

#include "mutex.h"
#include <stdint.h>

typedef struct {
	char *buffer;
	int head;
	int tail;
	int size;
	int elementSize;
	metal_mutex_t rd_mutex;
	metal_mutex_t wr_mutex;
} circ_buf;


#define circ_buf_global_def(TYPE,SUBTYPE,INST,SUBINST,SIZE,MEMSECTION)	\
SUBTYPE __attribute__((section(MEMSECTION))) SUBINST[SIZE]; \
TYPE __attribute__((section(MEMSECTION))) INST = { \
	.buffer = (char *)SUBINST, \
	.head = 0, \
	.tail = 0, \
	.size = SIZE, \
	.elementSize = sizeof(SUBTYPE), \
	.rd_mutex.v = 0, \
	.wr_mutex.v = 0, \
};

#define circ_buf_def(INST,SUBINST,SUBTYPE,SIZE)		circ_buf_global_def(circ_buf,SUBTYPE,INST,SUBINST,SIZE,".data")
#define circ_buf_def_section(INST,SUBINST,SUBTYPE,SIZE,MEMSECTION)		circ_buf_global_def(circ_buf,SUBTYPE,INST,SUBINST,SIZE,MEMSECTION)

/* Return count in buffer.  */
#define CIRC_CNT(head,tail,size) (((head) - (tail)) & ((size)-1))

/* Return space available, 0..size-1.  We always leave one free char
   as a completely full buffer has head == tail, which is the same as
   empty.  */
#define CIRC_SPACE(head,tail,size) CIRC_CNT((tail),((head)+1),(size))

/* Return count up to the end of the buffer.  Carefully avoid
   accessing head and tail more than once, so they can change
   underneath us without returning inconsistent results.  */
#define CIRC_CNT_TO_END(head,tail,size) \
	({int end = (size) - (tail); \
	  int n = ((head) + end) & ((size)-1); \
	  n < end ? n : end;})

/* Return space available up to the end of the buffer.  */
#define CIRC_SPACE_TO_END(head,tail,size) \
	({int end = (size) - 1 - (head); \
	  int n = (end + (tail)) & ((size)-1); \
	  n <= end ? n : end+1;})

void circ_buf_init( circ_buf *cr_buf, char *buffer, int elementSize, int elementCount );
void circ_buf_put( circ_buf *cr_buf, char *data );
void circ_buf_get( circ_buf *cr_buf, char *data );
uint32_t circ_buf_get_counter( circ_buf *cr_buf );
uint32_t circ_buf_is_empty( circ_buf *cr_buf );
uint32_t circ_buf_is_full( circ_buf *cr_buf );

#endif /* _LINUX_CIRC_BUF_H  */
