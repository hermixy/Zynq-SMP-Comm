/*
 * util.c
 *
 *  Created on: Dec 11, 2016
 *      Author: kulich
 */
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "arm_dcc.h"

#include "config.h"

#define LINE_BUFFER_SIZE		128		// Jak mame dlouhy radek ve znacich
#define CONSOLE_BUFFER_SIZE		1024		// Radky muzeme sloucit do jednoho velkeho bufferu a poslat najednou (nekdy je to vyhodnejsi, ale tady nepouzivame)

volatile uint16_t max_log_length = 0;
static char line_buffer[LINE_BUFFER_SIZE];
static uint8_t console_buffer[2][CONSOLE_BUFFER_SIZE];

void log_buffer_flush()
{
#ifdef DEBUG
	printf( line_buffer );
#endif
	line_buffer[0] = 0;
}

void log_debug( const char *format, ... )
{
#ifdef DEBUG
	va_list vargs;
	va_start(vargs, format);
	strcpy( line_buffer, "\nDebug: " );
	vsprintf( line_buffer + strlen(line_buffer), format, vargs );
	strcpy( line_buffer + strlen(line_buffer), "\n" );
	va_end(vargs);
	log_buffer_flush();
#endif
}

void log_warning( const char *format, ... )
{
#ifdef DEBUG
	va_list vargs;
	va_start(vargs, format);
#ifdef CONSOLE_FORMATED_OUTPUT
	strcpy( line_buffer, "\e[33mWarning:\e[39m " );
#else
	strcpy( line_buffer, "Warning: " );
#endif
	vsprintf( line_buffer + strlen(line_buffer), format, vargs );
	strcpy( line_buffer + strlen(line_buffer), "\n" );
	va_end(vargs);
	log_buffer_flush();
#endif
}

void log_error( const char *format, ... )
{
#ifdef DEBUG
	va_list vargs;
	va_start(vargs, format);
#ifdef CONSOLE_FORMATED_OUTPUT
	strcpy( line_buffer, "\e[31mError:\e[39m " );
#else
	strcpy( line_buffer, "Error: " );
#endif
	vsprintf( line_buffer + strlen(line_buffer), format, vargs );
	strcpy( line_buffer + strlen(line_buffer), "\n" );
	va_end(vargs);
	log_buffer_flush();
#endif
}

void log_console(const char *format, ...)
{
#ifdef DEBUG
	uint16_t str_offset = strlen(line_buffer);
	va_list vargs;
	va_start(vargs, format);
	vsprintf( line_buffer + str_offset, format, vargs );
	va_end(vargs);
	str_offset = strlen(line_buffer);
	if( max_log_length < str_offset )
		max_log_length = str_offset;
	log_buffer_flush();
#endif
}

int _write (int fd, char *ptr, int len)
{
	static uint8_t buffer_index = 0;

	if( len < CONSOLE_BUFFER_SIZE) {
		memcpy( &console_buffer[buffer_index][0], ptr, len );
	} else {
		memcpy( &console_buffer[buffer_index][0], ptr, CONSOLE_BUFFER_SIZE-1 );
		console_buffer[buffer_index][CONSOLE_BUFFER_SIZE-1] = 0;
		len = CONSOLE_BUFFER_SIZE;
	}

#ifdef CONSOLE_REDIRECT_TO_DDC
	for( uint16_t i = 0 ; i < len ; i++ ){
		arm_dcc_putc( console_buffer[buffer_index][i] );
	}
#endif
	buffer_index = 1 - buffer_index;
	return len;
}

// TODO - neni implementovano
int _read (int fd, char *ptr, int len)
{
	int count = len;
	return count;
}

