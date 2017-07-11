/*
 * debug.h
 *
 *  Created on: Jul 13, 2016
 *      Author: kulich
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#define ASSERT_PARAM(x)	{ if(!(x)) while(1); }
#ifdef DEBUG_PRINTF
#define debug_printf(...)	xil_printf(__VA_ARGS__)
#else
#define debug_printf(...)
#endif

#define DBG(...)		do{}while(0)
#define DBGusbCS(...)	do{}while(0)
#define DBGusb(...)		do{}while(0)

#define TIM_CLOCK			330000					// ??? netusim
#define TIME_START_MEAS 	XTime_GetTime(&xtimeS)
// TIME_END_MEAS - vysledok v ms
#define TIME_END_MEAS		do{											\
								XTime_GetTime(&xtimeE); 				\
								res = (xtimeE - xtimeS)/TIM_CLOCK;		\
							}while(0)

#define TIME_MEAS 			XTime xtimeS, xtimeE, res

void log_warning(const char *format, ...);
void log_error(const char *format, ...);
void log_debug( const char *format, ... );
void log_buffer_flush();
void log_buffer_log(const char *format, ...);

#endif /* DEBUG_H_ */
