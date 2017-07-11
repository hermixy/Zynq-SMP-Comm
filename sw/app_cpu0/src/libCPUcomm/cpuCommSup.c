/*
 * cpuCommSup.c
 *
 *  Created on: Jun 13, 2017
 *      Author: jano
 */

#include "xil_mmu.h"
#include "xpseudo_asm.h"
//#include "xstatus.h"	-- //TODO preco ho nemozem includnut ?
#include "xscugic.h"

void DisableOcmCache()
{
    //Disable cache on OCM
	Xil_SetTlbAttributes(0xFFFF0000,0x14de2);           // S=b1 TEX=b100 AP=b11, Domain=b1111, C=b0, B=b0
}

uint32_t prepare2CoresRun(void) {
#if 1
	// CPU1 ceka v tomhle miste v pameti, ze je adresa kam ma skocit
	uint32_t CPU1_START_ADRESS_REG = 0xfffffff0;

	// Adresa v pameti, kam kde je program pro CPU1.
	#warning "!!!!! Adresa musi byt stejna jako v linker skriptu !!!!!"
	uint32_t CPU1_START_ADRESS = 0x06000000;
	Xil_Out32( CPU1_START_ADRESS_REG, CPU1_START_ADRESS );

	// Ujistime se, ze se opravdu provede zapis
	/*
	 * Barrier for synchronization
	 */
		__asm(
			"dsb\n\t"
		);
	// Run CPU1, Run .... !
	__asm("sev");

#else
	int Status;
	(void)Status;

	//Disable cache on OCM
	Xil_SetTlbAttributes(0xFFFF0000,0x14de2);           // S=b1 TEX=b100 AP=b11, Domain=b1111, C=b0, B=b0

#warning "Preverit ci to je OK aj na realHW"
	//Disable cache on fsbl vector table location
	Xil_SetTlbAttributes(0x00000000,0x14de2);           // S=b1 TEX=b100 AP=b11, Domain=b1111, C=b0, B=b0

	// Initialize the SCU Interrupt Distributer (ICD)
//	Status = SetupIntrSystem(&IntcInstancePtr);
//	if (Status != XST_SUCCESS) {
//		return XST_FAILURE;
//	}

		#include "xil_misc_psreset_api.h"
		#include "xil_io.h"

    	#define A9_CPU_RST_CTRL		(XSLCR_BASEADDR + 0x244)
		#define A9_RST1_MASK 		0x00000002
		#define A9_CLKSTOP1_MASK	0x00000020
		#define CPU1_CATCH			0x00000024

		#define XSLCR_LOCK_ADDR		(XSLCR_BASEADDR + 0x4)
		#define XSLCR_LOCK_CODE		0x0000767B

		#define APP_CPU1_ADDR		0x06000000



    	u32 RegVal;

	/*
	    	 * Setup cpu1 catch address with starting address of app_cpu1. The FSBL initialized the vector table at 0x00000000
	    	 * using a boot.S that checks for cpu number and jumps to the address stored at the
	    	 * end of the vector table in cpu0_catch and cpu1_catch entries.
	    	 * Note: Cache has been disabled at the beginning of main(). Otherwise
			 * a cache flush would have to be issued after this write
	    	 */
	Xil_Out32(CPU1_CATCH, APP_CPU1_ADDR);

	/* Unlock the slcr register access lock */
	    	Xil_Out32(XSLCR_UNLOCK_ADDR, XSLCR_UNLOCK_CODE);

	    	//    the user must stop the associated clock, de-assert the reset, and then restart the clock. During a
	    	//    system or POR reset, hardware automatically takes care of this. Therefore, a CPU cannot run the code
	    	//    that applies the software reset to itself. This reset needs to be applied by the other CPU or through
	    	//    JTAG or PL. Assuming the user wants to reset CPU1, the user must to set the following fields in the
	    	//    slcr.A9_CPU_RST_CTRL (address 0xF8000244) register in the order listed:
	    	//    1. A9_RST1 = 1 to assert reset to CPU0
	    	//    2. A9_CLKSTOP1 = 1 to stop clock to CPU0
	    	//    3. A9_RST1 = 0 to release reset to CPU0
	    	//    4. A9_CLKSTOP1 = 0 to restart clock to CPU0

	    	/* Assert and deassert cpu1 reset and clkstop using above sequence*/
//	    	RegVal = 	Xil_In32(A9_CPU_RST_CTRL);
//	    	RegVal |= A9_RST1_MASK;
//	    	Xil_Out32(A9_CPU_RST_CTRL, RegVal);
//	    	RegVal |= A9_CLKSTOP1_MASK;
//	    	Xil_Out32(A9_CPU_RST_CTRL, RegVal);			// tu mi to padne ...
//	    	RegVal &= ~A9_RST1_MASK;
//			Xil_Out32(A9_CPU_RST_CTRL, RegVal);
//	    	RegVal &= ~A9_CLKSTOP1_MASK;
//			Xil_Out32(A9_CPU_RST_CTRL, RegVal);

	    	/* lock the slcr register access */
	    	Xil_Out32(XSLCR_LOCK_ADDR, XSLCR_LOCK_CODE);
#endif
	return XST_SUCCESS;
}
