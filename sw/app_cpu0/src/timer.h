/*
 * timer.h
 *
 *  Created on: Sep 10, 2015
 *      Author: kulich
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>
#include "xscugic.h"

typedef void (*TickCallBack)(void);

int TimerInit( TickCallBack tickCallBackHandler, XScuGic *GicInstancePtr );
void TimerTickLedInit( XScuGic *InterruptController );
void TimerTickLedGreen( uint16_t value );
void TimerTickLedRed( uint16_t value );
void TimerTick10ms( void );

#endif /* TIMER_H_ */
