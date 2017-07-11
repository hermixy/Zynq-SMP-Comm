/*
 * util.c
 *
 *  Created on: Mar 17, 2017
 *      Author: kulich
 */

#include "util.h"

int InitInterrupt( XScuGic *GicInstancePtr )
{
	/*
	 * Initialize the interrupt controller driver so that it is ready to
	 * use.
	 * */
	int Status;
	Xil_ExceptionInit();
	XScuGic_Config *GicConfig = XScuGic_LookupConfig(XPAR_PS7_SCUGIC_0_DEVICE_ID);
	if (NULL == GicConfig) {
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize( GicInstancePtr, GicConfig,
			GicConfig->CpuBaseAddress);

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,

				(Xil_ExceptionHandler)XScuGic_InterruptHandler,

				GicInstancePtr);
	return Status;
}

void EnableInterrupt( XScuGic *GicInstancePtr )
{
	// Enable interrupts in the Processor.
	Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);
	/*
	 * Enable interrupts in the ARM
	 */
	Xil_ExceptionEnable();
}

void DisableInterrupt( XScuGic *GicInstancePtr )
{
	// Disable interrupts in the Processor.
	Xil_ExceptionDisableMask(XIL_EXCEPTION_IRQ);
	/*
	 * Disable interrupts in the ARM
	 */
	Xil_ExceptionDisable();
}
