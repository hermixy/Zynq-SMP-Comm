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
#include "debug.h"
#include "mutex.h"
#include "barrier.h"

#include "cpuComm.h"
#include "config.h"

metal_mutex_t mutex;

#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID
static XScuGic InterruptController;  /* Interrupt controller instance */

static volatile uint32_t counter = 0;
void DeviceSwIntHandlerCpu0(void *CallbackRef)
{
	/*
	 * Indicate the interrupt has been processed using a shared variable
	 */
	counter++;
}

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
/*
 * Promenne pro komunikaci mezu CPU0 (Master) a CPU1 (Slave)
 * Pravidla:
 * - sdilena pamet je v sekcich ".sharedMemorySectionHeader" a ".sharedMemorySectionCpu0" a ".sharedMemorySectionCpu1"
 * - ipc_cfg_header_t - jedina promenna v sekci ".sharedMemorySectionHeader" a sedi na adrese 0xFFFF0000
 * - sdilena pamet je mala (asi 60kB), takze obsahuje jenom kruhove buffery
 * - tato oblast neni cacheovana, takze se do muze primo zapisovat a neni potreba invalidovat cache
 * - na tehle oblasti nefunguji atomicke instrukce (nepouzivat mutexy a spinlocky)!!
 * -
 */

// Jedina struktura na fixni adrese, obsahuje ukazatele na struktury, ktere pouzivaji CPU pro komunikaci
ipc_cfg_header_t __attribute__((section(".sharedMemorySectionHeader"))) ipc_cfg_header;
// Data jednotlivych CPU
circ_buf_def_section( circBufferCmdCpu0, bufferCmdCpu0, ipc_cmd_t, 32, ".sharedMemorySectionCpu0" );	// Buffer zprav pro CPU0
circ_buf_def_section( circBufferCmdCpu1, bufferCmdCpu1, ipc_cmd_t, 32, ".sharedMemorySectionCpu1" );	// Buffer zprav pro CPU1
circ_buf_def_section( circBufferEncFramesCpu1, bufferDataBufOne, ipcDataBufOne_st, 128, ".sharedMemorySectionCpu1" );
SHARED_VARIABLE(ipc_cmd_t, ipc_cmd, ".sharedMemorySectionCpu0");	// Cmd, ktery pouzivame si dame do sdilene pameti

// Nemuzeme inicilazovat v sekci NOLOAD, takze si musime vse v tehle sekci inicializovat sami
void InitSharedVariable( ipc_cfg_header_t *ipc_cfg_header )
{
	memset( ipc_cfg_header, 0x00, sizeof(ipc_cfg_header_t) );
	circ_buf_init( &circBufferCmdCpu0, (char *)bufferCmdCpu0, sizeof(ipc_cmd_t), sizeof(bufferCmdCpu0) / sizeof(ipc_cmd_t) );
	circ_buf_init( &circBufferCmdCpu1, (char *)bufferCmdCpu1, sizeof(ipc_cmd_t), sizeof(bufferCmdCpu1) / sizeof(ipc_cmd_t) );
	circ_buf_init( &circBufferEncFramesCpu1, (char *)bufferDataBufOne, sizeof(ipcDataBufOne_st), sizeof(bufferDataBufOne) / sizeof(ipcDataBufOne_st) );
	ipc_cfg_header->buffer_cmd_cpu1 = &circBufferCmdCpu1;
	ipc_cfg_header->buffer_cmd_cpu0 = &circBufferCmdCpu0;
	ipc_cfg_header->buffer_enc_frames_cpu1_to_cpu0 = &circBufferEncFramesCpu1;
}

uint8_t CpuCommMasterInit( ipc_cfg_header_t *ipc_cfg_header )
{
	DisableOcmCache();

	InitSharedVariable( ipc_cfg_header );

	smp_wmb();

	prepare2CoresRun();

	int timeout = 10;
	do
	{
		if( CpuCommGetCmd( CPU_MASTER_ID, &ipc_cmd ) == XST_SUCCESS ){
			if( ipc_cmd.cmd == IPC_CMD_INIT_DONE )
				break;
		}
		usleep( 1000 );
	}while( timeout-- );

	if( !timeout )
		while(1);	// TODO - dodelat rozumnou reakci, kdyz selze CPU1
}

int main()
{
    init_platform();
    InitBoard( NULL );

    SetupInterruptSystem( INTC_DEVICE_ID, &InterruptController );
    CpuCommInit( &InterruptController, &ipc_cfg_header, DeviceSwIntHandlerCpu0);
    CpuCommMasterInit( &ipc_cfg_header );

    while(1){
    	CpuCommCreateSimpleCmd( &ipc_cmd, IPC_CMD_SET_LED );
    	CpuCommSendCmd( CPU_SLAVE_ID, &ipc_cmd, 1 );
    	CpuCommSendCmd( CPU_MASTER_ID, &ipc_cmd, 1 );
    	usleep(500000);

    	CpuCommCreateSimpleCmd( &ipc_cmd, IPC_CMD_CLR_LED );
    	CpuCommSendCmd( CPU_SLAVE_ID, &ipc_cmd, 1 );
    	usleep(500000);
    }

    cleanup_platform();
    return 0;
}
