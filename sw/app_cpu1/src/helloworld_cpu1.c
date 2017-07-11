/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xscugic.h"
#include "xparameters.h"
#include "board.h"
#include "timer.h"
#include "cpuComm.h"
#include "mutex.h"
#include "barrier.h"

#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID
static XScuGic InterruptController;
static volatile uint32_t counter = 0;

/****************************************************************************/
/**
*
* This function setups the interrupt system such that interrupts can occur.
* This function is application specific since the actual system may or may not
* have an interrupt controller.  The TTC could be directly connected to a
* processor without an interrupt controller.  The user should modify this
* function to fit the application.
*
* @param	IntcDeviceID is the unique ID of the interrupt controller
* @param	IntcInstacePtr is a pointer to the interrupt controller
*		instance.
*
* @return	XST_SUCCESS if successful, otherwise XST_FAILURE.
*
* @note		None.
*
*****************************************************************************/
static int SetupInterruptSystem(u16 IntcDeviceID,
				    XScuGic *IntcInstancePtr)
{
	int Status;
	XScuGic_Config *IntcConfig; /* The configuration parameters of the
					interrupt controller */

	/*
	 * Initialize the interrupt controller driver
	 */
	IntcConfig = XScuGic_LookupConfig(IntcDeviceID);
	if (NULL == IntcConfig) {
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig,
					IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Connect the interrupt controller interrupt handler to the hardware
	 * interrupt handling logic in the ARM processor.
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
			(Xil_ExceptionHandler) XScuGic_InterruptHandler,
			IntcInstancePtr);

	/*
	 * Enable interrupts in the ARM
	 */
	Xil_ExceptionEnable();

	return XST_SUCCESS;
}

void DeviceVdmaHandler(void *CallbackRef)
{

}

void DeviceSwIntHandlerCpu1(void *CallbackRef)
{
	/*
	 * Indicate the interrupt has been processed using a shared variable
	 */
	counter++;
}

ipc_cfg_header_t __attribute__((section(".sharedMemorySectionHeader"))) ipc_cfg_header;
SHARED_VARIABLE(ipc_cmd_t, ipc_cmd, ".sharedMemorySectionCpu1");

void CpuCommSlaveInit( ipc_cfg_header_t *ipc_cfg_header )
{
	DisableOcmCache();

	// Posleme zpravu Master CPU, ze je hotova inicializace
	CpuCommCreateSimpleCmd( &ipc_cmd, IPC_CMD_INIT_DONE );
	CpuCommSendCmd( CPU_MASTER_ID, &ipc_cmd, 0 );
}

int main()
{
    // Bezpecna inicializace HW
    InitBoard( NULL );
    SetupInterruptSystem( INTC_DEVICE_ID, &InterruptController );
    CpuCommInit( &InterruptController, &ipc_cfg_header, DeviceSwIntHandlerCpu1 );

    CpuCommSlaveInit( &ipc_cfg_header );

    SetLedState( 1 );

    while(1)
    {
    	while( CpuCommGetCmd( CPU_SLAVE_ID, &ipc_cmd ) != XST_FAILURE ){
    		switch ( ipc_cmd.cmd ){
    			case IPC_CMD_SET_LED :
    				SetLedState( 1 );
    				break;
    			case IPC_CMD_CLR_LED :
    				SetLedState( 0 );
    				break;
    			default :
    				// Neznamy prikaz
    				while(1);
    		}
    	}
    	__asm("wfi");
    }

    cleanup_platform();
    return 0;
}
