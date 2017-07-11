/*
 * board.c
 *
 *  Created on: Mar 16, 2017
 *      Author: kulich
 */
#include "xparameters.h"
#include "xgpiops.h"
#include "board.h"

#define IN_PS_PIN		0
#define OUT_PS_PIN		1

XGpioPs GpioPs; /* The driver instance for GPIO Device configured as O/P */

int InitBoard( XScuGic *GicInstancePtr )
{
#ifndef FINALBOARD
	XGpioPs_Config * ConfigPtr = XGpioPs_LookupConfig(XPAR_PS7_GPIO_0_DEVICE_ID);
	XGpioPs_CfgInitialize(&GpioPs, ConfigPtr, ConfigPtr->BaseAddr);

	XGpioPs_SetDirectionPin(&GpioPs, LED_RED_CPU_PIN, OUT_PS_PIN);
	XGpioPs_SetOutputEnablePin(&GpioPs, LED_RED_CPU_PIN, 1);

	XGpioPs_SetDirectionPin(&GpioPs, USER_SWITCH_CPU_PIN, IN_PS_PIN);
	XGpioPs_SetOutputEnablePin(&GpioPs, USER_SWITCH_CPU_PIN, 0);

	XGpioPs_SetDirectionPin(&GpioPs, USB_RESET_PIN, OUT_PS_PIN);
	XGpioPs_SetOutputEnablePin(&GpioPs, USB_RESET_PIN, 1);
#endif
	return 1;
}

void SetLedState( uint8_t state )
{
#ifndef FINALBOARD
	XGpioPs_WritePin(&GpioPs, LED_RED_CPU_PIN, state ? 1 : 0 );
#endif
}


uint8_t GetSwitchState()
{
#ifndef FINALBOARD
	return XGpioPs_ReadPin(&GpioPs, USER_SWITCH_CPU_PIN );
#else
	return 0;
#endif
}

void SetUsbResetState( uint8_t state )
{
	// Inverted
	XGpioPs_WritePin(&GpioPs, USB_RESET_PIN, state ? 0 : 1 );
}

