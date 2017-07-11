/*
 * cpuComm.c
 *
 *  Created on: May 24, 2017
 *      Author: jano
 */
#include <stdint.h>
#include "cpuComm.h"
#include "xscugic.h"
#include "debug.h"
#include "xparameters.h"

#define sev() __asm__("sev")
#define wfe() __asm__("wfe")

static XScuGic *IntcInstancePtr;
static ipc_cfg_header_t *ipc_cfg_header;
static uint32_t interruptIdCpuTable[MAX_CPU_NUM] = { INTC_DEVICE_CPU0_SWINT_ID, INTC_DEVICE_CPU1_SWINT_ID };

static inline uint32_t CpuIdToIntId( uint8_t cpuId )
{
	ASSERT_PARAM( cpuId < MAX_CPU_NUM );
	return interruptIdCpuTable[cpuId];
}

static int CpuCommInitSwInterrupt( Xil_InterruptHandler handler, uint8_t cpuId )
{
	ASSERT_PARAM( handler != NULL );
	ASSERT_PARAM( cpuId < MAX_CPU_NUM );

	int Status = XST_SUCCESS;


	uint32_t intId = CpuIdToIntId(cpuId);

	/*
	 * Connect a device driver handler that will be called when an
	 * interrupt for the device occurs, the device driver handler performs
	 * the specific interrupt processing for the device
	 */
	Status = XScuGic_Connect( IntcInstancePtr, intId,
				handler,
			   (void *)&IntcInstancePtr);

	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
	XScuGic_InterruptMaptoCpu( IntcInstancePtr, cpuId, intId );
	/*
	 * Enable the interrupt for the device and then cause (simulate) an
	 * interrupt so the handlers will be called
	 */
	XScuGic_Enable( IntcInstancePtr, intId);

	return Status;
}

/*
*
* Inicialuje komunikacni framework musi se volat na kazdem CPU
*
* @param	_IntcInstancePtr ukazatel na platnou inst radice prerusenu
* @param	_ipc_cfg_header ukazatel na platnou inst informacni struktury
* @param	handler tento handler se vola pokud dostane CPU z framwroku preruseni
*
* @return	XST_SUCCESS poslan uspesne signal, jinak XST_FAILURE.
*
* @note		None.
*
*****************************************************************************/
void CpuCommInit( XScuGic *_IntcInstancePtr, ipc_cfg_header_t *_ipc_cfg_header, Xil_InterruptHandler handler)
{
	ASSERT_PARAM( _IntcInstancePtr != NULL );
	ASSERT_PARAM( handler != NULL );
	ASSERT_PARAM( _ipc_cfg_header != NULL );
	uint8_t cpuId = XPAR_CPU_ID;

	IntcInstancePtr = _IntcInstancePtr;
	ipc_cfg_header = _ipc_cfg_header;
	if( handler != NULL)
		CpuCommInitSwInterrupt( handler, cpuId );
}
/*
*
* Podle Id posle INT pro dane CPU
*
* @param	cpuId je id CPU pro ketere se ma poslat interrupt
*
* @return	XST_SUCCESS poslan uspesne signal, jinak XST_FAILURE.
*
* @note		None.
*
*****************************************************************************/
int CpuCommSendSignalIntToCpu( uint8_t cpuId )
{
	ASSERT_PARAM( cpuId < MAX_CPU_NUM );
	return XScuGic_SoftwareIntr( IntcInstancePtr,
					CpuIdToIntId(cpuId),
					(cpuId == 0) ? XSCUGIC_SPI_CPU0_MASK : XSCUGIC_SPI_CPU1_MASK );
}

/*
*
* Podle Id zapise zpravu do fronty pro dane CPU
*
* @param	dstCpuId je id cilove CPU fronty
* @param	ipc_cmd je struktura ktera se zapise do fronty
* @param	sendSignal pokud je TRUE tak se po zapisu do fronty na dane CPU posle INT signal, FALSE zapise jenom do fronty
*
* @return	XST_SUCCESS pokud se do fronty poslala zprava a byl pripadne poslan uspesne signal, jinak XST_FAILURE.
*
* @note		None.
*
*****************************************************************************/
int CpuCommSendCmd( uint8_t dstCpuId, ipc_cmd_t *ipc_cmd, uint8_t sendSignal )
{
	circ_buf *cr_buf;
	ASSERT_PARAM( ipc_cmd != NULL );
	ASSERT_PARAM( dstCpuId < MAX_CPU_NUM );

	if( dstCpuId == CPU0_ID )
		cr_buf = ipc_cfg_header->buffer_cmd_cpu0;
	else
		cr_buf = ipc_cfg_header->buffer_cmd_cpu1;

	// Interni test
	ASSERT_PARAM( cr_buf->buffer != NULL);
	ASSERT_PARAM( cr_buf->elementSize > 0);
	ASSERT_PARAM( cr_buf->size > 0);

	if( circ_buf_is_full(cr_buf) )
		return XST_FAILURE;

	circ_buf_put( cr_buf, (char *)ipc_cmd );

	if( sendSignal )
		return CpuCommSendSignalIntToCpu( dstCpuId );
	else
		return XST_SUCCESS;
}


/*
*
* Podle Id fronty vyzvedne zpravu pro dane CPU
*
* @param	srcCpuId je id zdrojove CPU fronty
* @param	ipc_cmd je struktura ktera se nacetla z fronty
*
* @return	XST_SUCCESS pokud se z fronty vycetla zprava, jinak XST_FAILURE.
*
* @note		None.
*
*****************************************************************************/
int CpuCommGetCmd( uint8_t srcCpuId, ipc_cmd_t *ipc_cmd )
{
	circ_buf *cr_buf;
	ASSERT_PARAM( ipc_cmd != NULL );
	ASSERT_PARAM( srcCpuId < MAX_CPU_NUM );

	if( srcCpuId == CPU0_ID ){
		cr_buf = ipc_cfg_header->buffer_cmd_cpu0;
	} else {
		cr_buf = ipc_cfg_header->buffer_cmd_cpu1;
	}

	// Interni test
	ASSERT_PARAM( cr_buf->buffer != NULL);
	ASSERT_PARAM( cr_buf->elementSize > 0);
	ASSERT_PARAM( cr_buf->size > 0);

	if( circ_buf_is_empty( cr_buf ) )
		return XST_FAILURE;

	circ_buf_get( cr_buf, (char *)ipc_cmd );
	return XST_SUCCESS;
}

uint8_t sendCMDtoCPU1(uint8_t cmd, uint32_t *dataU32in, uint8_t *dataIn, uint8_t dataInSize, uint8_t *dataOut, uint8_t *dataOutSize, uint8_t *outState) {
	uint8_t i;
	if(dataInSize >= 32) return 0;
	COMM_VAL3.cmd = cmd;
	COMM_VAL3.cmdState = 0;
	COMM_VAL3.dataSize = dataInSize;
	for(i = 0; i < dataInSize; i++) {
		COMM_VAL3.data[i] = dataIn[i];
	}
	for(i = 0; i < 3; i++) {
		COMM_VAL3.datau32[i] = dataU32in[i];
	}
	COMM_VAL = 1;											// mam hotovo moze pristupovat CPU1
	sev();													// prebudim CPU1
	while(COMM_VAL == 1);									// pockam pokial mi povie ze ma hotovo
	if(COMM_VAL3.cmd == cmd) {								// odpovedal na rovnaky CMD ?
		*outState = COMM_VAL3.cmdState;
		if(COMM_VAL3.cmdState == CMD_STATE_OK) {			// odpovedal ze CMD spracoval ?
			for(i = 0; i < 3; i++) {
				dataU32in[i] = COMM_VAL3.datau32[i];
			}
			*dataOutSize = COMM_VAL3.dataSize;				// ma pre nas nejake data ?
			if(COMM_VAL3.dataSize) {
				for(i = 0; i < COMM_VAL3.dataSize; i++) {	// skopirujem data
					dataOut[i] = COMM_VAL3.data[i];
				}
			}
			return 1;									// ok CMD bol uspesne vykonany
		}
	}
	return 0;
}

void recvCMDfromCPU0(cmd_st *cmdOut) {
	while(COMM_VAL == 0){};									// ma CPU1 hotovo ?
	*cmdOut = COMM_VAL3;									// skopirujem si prijaty CMD
	// tu spracujem command
	//COMM_VAL = 0;											// poviem CPU1 ze mam hotovo
	//wfe();												// uspavam sa a cakam na prebudenie, toto asi pojde inde
}

void sendCMDtoCPU0(cmd_st *cmdIn) {
	COMM_VAL3 = *cmdIn;										// skopirujem CMD/odpoved
	COMM_VAL = 0;											// poviem CPU1 ze mam hotovo
}
