/*
 * timer.c
 *
 *  Created on: Sep 10, 2015
 *      Author: kulich
 */

#define TIMER_DEVICE_ID		XPAR_TMRCTR_0_DEVICE_ID

#include "timer.h"
#include "xparameters.h"
#include "xttcps.h"
#include "util.h"
//

#define TTC_TICK_DEVICE_ID	XPAR_XTTCPS_0_DEVICE_ID
#define TTC_TICK_INTR_ID	XPAR_XTTCPS_0_INTR
#define TTCPS_CLOCK_HZ		XPAR_XTTCPS_0_CLOCK_HZ

#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID

static TickCallBack g_tickCallBackHandler = NULL;

static volatile u32 TickCount;		/* Ticker interrupts between PWM change */
static XTtcPs TtcPsInst[2];
typedef struct {
	u32 OutputHz;	/* Output frequency */
	u16 Interval;	/* Interval value */
	u8 Prescaler;	/* Prescaler value */
	u16 Options;	/* Option settings */
} TmrCntrSetup;

static TmrCntrSetup SettingsTable[2] = {
	{8, 0, 0, 0},	/* Ticker timer counter initial setup, only output freq */
	{0, 0, 0, 0},	/* Ticker timer counter initial setup, only output freq */
};

static XScuGic Intc; //GIC
static void SetupInterruptSystem(XScuGic *GicInstancePtr, XTtcPs *TtcPsInt);
static void TickHandler(void *CallBackRef);

static uint16_t ledGreenPrescaler = 0;
static uint16_t ledGreenCounter = 0;
static uint8_t ledGreenState = 0;
static uint16_t ledRedPrescaler = 0;
static uint16_t ledRedCounter = 0;
static uint8_t ledRedState = 0;

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
static void SetupInterruptSystem(XScuGic *GicInstancePtr, XTtcPs *TtcPsInt)
{

	XScuGic_Connect(GicInstancePtr, TTC_TICK_INTR_ID,
			(Xil_ExceptionHandler)TickHandler, (void *)TtcPsInt);

	XScuGic_InterruptMaptoCpu( GicInstancePtr, XPAR_CPU_ID, TTC_TICK_INTR_ID );

	XScuGic_Enable(GicInstancePtr, TTC_TICK_INTR_ID);
	XTtcPs_EnableInterrupts(TtcPsInt, XTTCPS_IXR_INTERVAL_MASK);

}

/***************************************************************************/
/**
*
* This function is the handler which handles the periodic tick interrupt.
* It updates its count, and set a flag to signal PWM timer counter to
* update its duty cycle.
*
* This handler provides an example of how to handle data for the TTC and
* is application specific.
*
* @param	CallBackRef contains a callback reference from the driver, in
*		this case it is the instance pointer for the TTC driver.
*
* @return	None.
*
* @note		None.
*
*****************************************************************************/
static void TickHandler(void *CallBackRef)
{
	u32 StatusEvent;

	/*
	 * Read the interrupt status, then write it back to clear the interrupt.
	 */
	StatusEvent = XTtcPs_GetInterruptStatus((XTtcPs *)CallBackRef);
	XTtcPs_ClearInterruptStatus((XTtcPs *)CallBackRef, StatusEvent);

	if (0 != (XTTCPS_IXR_INTERVAL_MASK & StatusEvent)) {
		//TickCount++;
		if( g_tickCallBackHandler != NULL )
			g_tickCallBackHandler();
	}

}

int SetupTimerTicker()
{
	return 0;
}

/****************************************************************************/
/**
*
* This function sets up the Ticker timer.
*
* @param	None
*
* @return	XST_SUCCESS if everything sets up well, XST_FAILURE otherwise.
*
*****************************************************************************/
int TimerInit( TickCallBack tickCallBackHandler, XScuGic *GicInstancePtr )
{
	int Status;

	g_tickCallBackHandler = tickCallBackHandler;
	/*
	 * Make sure the interrupts are disabled, in case this is being run
	 * again after a failure.
	 */

	XTtcPs_Config *Config;
	XTtcPs *Timer;
	TmrCntrSetup *TimerSetup;

	TimerSetup = &SettingsTable[0];
	Timer = &TtcPsInst[0];


	Config = XTtcPs_LookupConfig(TTC_TICK_DEVICE_ID);
	if (NULL == Config) {
		return XST_FAILURE;
	}

	XTtcPs_Stop( Timer );

	Status = XTtcPs_CfgInitialize( Timer, Config, Config->BaseAddress);
/*
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
*/
	if( !XTtcPs_IsStarted( Timer ) )
	{
	 	TimerSetup->Options |= (XTTCPS_OPTION_INTERVAL_MODE |

	 						      XTTCPS_OPTION_WAVE_DISABLE | XTTCPS_OPTION_EXTERNAL_CLK);



	 	XTtcPs_SetOptions( Timer, TimerSetup->Options);

	 	XTtcPs_CalcIntervalFromFreq( Timer, TimerSetup->OutputHz,&(TimerSetup->Interval), &(TimerSetup->Prescaler));

	    XTtcPs_SetInterval( Timer, TimerSetup->Interval);
	    XTtcPs_SetPrescaler( Timer, TimerSetup->Prescaler);

	    XTtcPs_Start( Timer );
	}
	/*
	 * Connect the Intc to the interrupt subsystem such that interrupts can
	 * occur. This function is application specific.
	 */
	SetupInterruptSystem( GicInstancePtr, Timer);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}


void TimerTick10ms( void )
{

}

void TimerTickLedGreen( uint16_t value )
{
	ledGreenPrescaler = value;
}

void TimerTickLedRed( uint16_t value )
{
	ledRedPrescaler = value;
}

void TimerTickLedInit( XScuGic *InterruptController )
{
	ledGreenPrescaler = 0;
	ledGreenCounter = 0;
	ledGreenState = 0;
	ledRedPrescaler = 0;
	ledRedCounter = 0;
	ledRedState = 0;
	TimerInit( &TimerTick10ms, InterruptController );
}

