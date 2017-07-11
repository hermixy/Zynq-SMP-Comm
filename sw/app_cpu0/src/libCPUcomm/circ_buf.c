/*
 * circ_buffer.c
 *
 *  Created on: Jul 7, 2017
 *      Author: kulich
 */

#ifndef SRC_CIRC_BUFFER_C_
#define SRC_CIRC_BUFFER_C_

#include "circ_buf.h"
#include "stdatomic.h"
#include "barrier.h"
#include "string.h"

#include "debug.h"

void circ_buf_put( circ_buf *cr_buf, char *data )
{
	//metal_mutex_acquire( &cr_buf->wr_mutex );
	ASSERT_PARAM( cr_buf != NULL );
	ASSERT_PARAM( data != NULL );

	unsigned long head = cr_buf->head;
	unsigned long tail = ACCESS_ONCE( cr_buf->tail );


	if (CIRC_SPACE(head, tail, cr_buf->size) >= 1) {
		/* insert one item into the buffer */
		memcpy( &cr_buf->buffer[head*cr_buf->elementSize], data, cr_buf->elementSize );

		smp_wmb(); /* commit the item before incrementing the head */

		cr_buf->head = (head + 1) & (cr_buf->size - 1);
	}

	//metal_mutex_release( &cr_buf->wr_mutex );
}

void circ_buf_get( circ_buf *cr_buf, char *data )
{
	//metal_mutex_acquire(&cr_buf->rd_mutex);

	unsigned long head = ACCESS_ONCE(cr_buf->head);
	unsigned long tail = cr_buf->tail;

	if (CIRC_CNT(head, tail, cr_buf->size) >= 1) {
		/* read index before reading contents at that index */
		smp_read_barrier_depends();

		/* extract one item from the buffer */
		memcpy( data, &cr_buf->buffer[tail*cr_buf->elementSize], cr_buf->elementSize );

		smp_mb(); /* finish reading descriptor before incrementing tail */

		cr_buf->tail = (tail + 1) & (cr_buf->size - 1);
	}

	//metal_mutex_release(&cr_buf->rd_mutex);
}

uint32_t circ_buf_get_counter( circ_buf *cr_buf )
{
	unsigned long head = ACCESS_ONCE(cr_buf->head);
	unsigned long tail = ACCESS_ONCE(cr_buf->tail);
	return (uint32_t)CIRC_CNT( head, tail,cr_buf->size);
}

uint32_t circ_buf_is_full( circ_buf *cr_buf )
{
	unsigned long head = ACCESS_ONCE(cr_buf->head);
	unsigned long tail = ACCESS_ONCE(cr_buf->tail);
	return (uint32_t)CIRC_SPACE( head, tail,cr_buf->size) >= 1 ? 0 : 1;
}

uint32_t circ_buf_is_empty( circ_buf *cr_buf )
{
	unsigned long head = ACCESS_ONCE(cr_buf->head);
	unsigned long tail = ACCESS_ONCE(cr_buf->tail);
	return (CIRC_CNT( head, tail,cr_buf->size) == 0 ) ? 1 : 0;
}

void circ_buf_init( circ_buf *cr_buf, char *buffer, int elementSize, int elementCount )
{
	memset( cr_buf, 0x00, sizeof(circ_buf) );
	cr_buf->buffer = buffer;
	cr_buf->elementSize = elementSize;
	cr_buf->size = elementCount;
}
#endif /* SRC_CIRC_BUFFER_C_ */
