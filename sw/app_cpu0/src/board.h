/*
 * board.h
 *
 *  Created on: Mar 16, 2017
 *      Author: kulich
 */

#ifndef BOARD_H_
#define BOARD_H_
#include "xscugic.h"
#include <stdint.h>
#include "xparameters.h"


//#define FINALBOARD		// !!!!!! Jenom pro ladeni na finalni desce pro Microzed zakomentovat!!!!!!!!

#define LED_RED_CPU_PIN			47
#define USER_SWITCH_CPU_PIN		51
#define USB_RESET_PIN			7

#define XPAR_FABRIC_AXI_VDMA_0_S2MM_FRAME_COMPLETE_INTR_ID 61

int InitBoard( XScuGic *GicInstancePtr );
void SetLedState( uint8_t state );
uint8_t GetSwitchState();
void SetUsbResetState( uint8_t state );

#endif /* BOARD_H_ */
