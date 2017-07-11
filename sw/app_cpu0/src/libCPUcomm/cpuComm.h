/*
 * cpuComm.h
 *
 *  Created on: May 24, 2017
 *      Author: jano
 */

#ifndef SRC_LIBCPUCOMM_CPUCOMM_H_
#define SRC_LIBCPUCOMM_CPUCOMM_H_

#include "xpseudo_asm.h"
#include "mutex.h"
#include "xil_exception.h"
#include "xscugic.h"

#include "debug.h"
#include "circ_buf.h"
#include "cpuCommSup.h"


#define CPU0_ID		0
#define CPU1_ID		1
#define MAX_CPU_NUM	2

#define CPU_MASTER_ID	CPU0_ID
#define CPU_SLAVE_ID	CPU1_ID
// Jake cisla kanalu preruseni ma jake CPU
#define INTC_DEVICE_CPU1_SWINT_ID	0x0E	// Int CPU1
#define INTC_DEVICE_CPU0_SWINT_ID	0x0D	// Int CPU0

#define CMD_DATA_SIZE		32

#define COMM_VAL    	(*(volatile unsigned long *)(0xFFFF0000))
#define COMM_VAL2    	(*(volatile unsigned long *)(0xFFFF0004))
#define COMM_VAL3    	(*(volatile cmd_st *)(0xFFFF0008))
#define cbD				(*(volatile ipcDataBuf_st *)(0xFFFF0008 + sizeof(cmd_st)))

typedef struct {
	uint32_t cmd;
	uint32_t cmdState;
	uint8_t dataSize;
	uint8_t data[CMD_DATA_SIZE];
	uint32_t datau32[3];
}cmd_st;

extern uint8_t sendCMDtoCPU1(uint8_t cmd, uint32_t *dataU32in, uint8_t *dataIn, uint8_t dataInSize, uint8_t *dataOut, uint8_t *dataOutSize, uint8_t *outState);
extern void recvCMDfromCPU0(cmd_st *cmdOut);
extern void sendCMDtoCPU0(cmd_st *cmdIn);

typedef enum {
	IPC_CMD_GET_INFO = 0,
	IPC_CMD_INC_VALUE = 1,
	IPC_CMD_SEND_VDMA_DATA = 4,
	IPC_CMD_GET_JPG = 5,
	// Nove - debug
	IPC_CMD_SET_LED = 0xF0,
	IPC_CMD_CLR_LED = 0xF1,
	IPC_CMD_INIT_DONE = 0xF2,
}IPC_CMD;

typedef enum {
	CMD_STATE_OK = 1,
	CMD_STATE_UNKNOWN = 2,
	CMD_STATE_ERR = 3
}CMD_STATE;

typedef struct {
	uint32_t state;
	unsigned char *mem;
	uint32_t memSize;
}ipcDataBufOne_st;

#define IPC_DATA_SIZE 		64
#define IPC_DATA_MASK		(IPC_DATA_SIZE - 1)

typedef struct {
	ipcDataBufOne_st i[IPC_DATA_SIZE];
	uint32_t start;
	uint32_t end;
}ipcDataBuf_st;


///////////////////////////////////////////////////////////////////////
// Novy framework
#define CPU1_CFG_VALID_MASK		(1<<0)

// Prikazy mezi CPU
typedef struct {
	uint32_t cmd;
	uint32_t *buf;
	uint32_t length;
} ipc_cmd_t;

// Vymena struktura sedici na pevne adrese a cpu zde najde konfigurace a vymene fronty
typedef struct {
	uint32_t status;
	metal_mutex_t hw_cfg_mutex;
	circ_buf *buffer_cmd_cpu0;	// Komunikacni fronta CPU0
	circ_buf *buffer_cmd_cpu1;	// Komunikacni fronta CPU1
	circ_buf *buffer_enc_frames_cpu1_to_cpu0;	// Fronta enkodovanych frames CPU1 -> CPU0
} ipc_cfg_header_t;

static inline void CpuCommCreateSimpleCmd( ipc_cmd_t *ipc_cmd, IPC_CMD cmd )
{
	ipc_cmd->cmd = cmd;
	ipc_cmd->buf = NULL;
	ipc_cmd->length = 1;
}

#define SHARED_VARIABLE(type,name,section_name)	type __attribute__((section(section_name))) name

void CpuCommInit( XScuGic *_IntcInstancePtr, ipc_cfg_header_t *_ipc_cfg_header, Xil_InterruptHandler handler);
int CpuCommSendSignalIntToCpu( uint8_t cpuId );
int CpuCommSendCmd( uint8_t dstCpuId, ipc_cmd_t *ipc_cmd ,uint8_t sendSignal );
int CpuCommGetCmd( uint8_t srcCpuId, ipc_cmd_t *ipc_cmd );
#endif /* SRC_LIBCPUCOMM_CPUCOMM_H_ */
