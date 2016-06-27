//*****************************************************************************
//
// Datalogger Application for the Stellaris M4F Microcontroller.
// 
// This Code is part of the practical Work of the Lecture "Mikrocontroller"
// at the University of Bayreuth. For more Information see the technical Report
//
// Copyright 2016, Anna Baumann, Andreas Braun, Marcel Fraas
//
//*****************************************************************************

// General Libraries
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "inc/hw_memmap.h"

// Watchdog Libraries
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "driverlib/watchdog.h"

// UART Libraries
#include "driverlib/uart.h"
#include "utils/uartstdio.h"

// Button Libraries
#include "driverlib/rom_map.h"
#include "buttons.h"
#include "buttons.c" // Irregular behaviour when missing

// SD-Card / FatFS Libraries
#include "fatfs/src/ff.h"
#include "fatfs/src/diskio.h"

// Timer / Hibernate / ADC Libraries
#include "driverlib/adc.h"
#include "driverlib/hibernate.h"

// Graphics Libraries
#include "grlib/grlib.h"
#include "drivers/cfal96x64x16.h"
#include "driverlib/pin_map.h"



//*****************************************************************************
//
// The clock rate for the SysTick interrupt and a counter of system clock
// ticks.  The SysTick interrupt is used for basic timing in the application.
// (Needed for Buttons)
//
//*****************************************************************************
#define CLOCK_RATE              100
#define MS_PER_SYSTICK          (1000 / CLOCK_RATE)
static volatile uint32_t g_ui32TickCount;
uint32_t g_ui32LastTick = 0;

//*****************************************************************************
//
// A temporary data buffer used when manipulating file paths, or reading data
// from the SD card. The Filepath and Name must not exceed 80
//
//*****************************************************************************
static char g_pcTmpBuf[82];

//*****************************************************************************
//
// The buffer that holds the Time sent from the PC. 
//
//*****************************************************************************
static char g_cTimeBuf[64];

//*****************************************************************************
//
// The buffer that holds the Temperature as String. 
//
//*****************************************************************************
char cTempBuf[30];

//*****************************************************************************
//
// The buffer that holds the Time as String. 
//
//*****************************************************************************
char cTimeBuf[30];

//*****************************************************************************
//
// The buffer that holds CSV Data for Writing to SD Card 
//
//*****************************************************************************
char cCSVBuf[64];

//*****************************************************************************
//
// The following are data structures used by FatFs.
//
//*****************************************************************************
static FATFS g_sFatFs;
static FIL g_sFileObject;
static char cFileName[] = "data.txt";

//*****************************************************************************
//
// Flag to control the Live-Feed of the Data
//
//*****************************************************************************
char cLive[] = "NO";

//------------------------------------------------------------------------------------
// Beginning of FatFS Error Block
//------------------------------------------------------------------------------------

//*****************************************************************************
//
// A structure that holds a mapping between an FRESULT numerical code, and a
// string representation.  FRESULT codes are returned from the FatFs FAT file
// system driver.
//
//*****************************************************************************
typedef struct
{
    FRESULT iFResult;
    char *pcResultStr;
}
tFResultString;

//*****************************************************************************
//
// A macro to make it easy to add result codes to the table.
//
//*****************************************************************************
#define FRESULT_ENTRY(f)        { (f), (#f) }

//*****************************************************************************
//
// A table that holds a mapping between the numerical FRESULT code and it's
// name as a string.  This is used for looking up error codes for printing to
// the console.
//
//*****************************************************************************
tFResultString g_psFResultStrings[] =
{
    FRESULT_ENTRY(FR_OK),
    FRESULT_ENTRY(FR_DISK_ERR),
    FRESULT_ENTRY(FR_INT_ERR),
    FRESULT_ENTRY(FR_NOT_READY),
    FRESULT_ENTRY(FR_NO_FILE),
    FRESULT_ENTRY(FR_NO_PATH),
    FRESULT_ENTRY(FR_INVALID_NAME),
    FRESULT_ENTRY(FR_DENIED),
    FRESULT_ENTRY(FR_EXIST),
    FRESULT_ENTRY(FR_INVALID_OBJECT),
    FRESULT_ENTRY(FR_WRITE_PROTECTED),
    FRESULT_ENTRY(FR_INVALID_DRIVE),
    FRESULT_ENTRY(FR_NOT_ENABLED),
    FRESULT_ENTRY(FR_NO_FILESYSTEM),
    FRESULT_ENTRY(FR_MKFS_ABORTED),
    FRESULT_ENTRY(FR_TIMEOUT),
    FRESULT_ENTRY(FR_LOCKED),
    FRESULT_ENTRY(FR_NOT_ENOUGH_CORE),
    FRESULT_ENTRY(FR_TOO_MANY_OPEN_FILES),
    FRESULT_ENTRY(FR_INVALID_PARAMETER),
};

//*****************************************************************************
//
// A macro that holds the number of result codes.
//
//*****************************************************************************
#define NUM_FRESULT_CODES       (sizeof(g_psFResultStrings) /                 \
                                 sizeof(tFResultString))

FRESULT iFResult;
																 
//*****************************************************************************
//
// This function returns a string representation of an error code that was
// returned from a function call to FatFs.  It can be used for printing human
// readable error messages.
//
//*****************************************************************************
const char *
StringFromFResult(FRESULT iFResult)
{
    uint_fast8_t ui8Idx;

    //
    // Enter a loop to search the error code table for a matching error code.
    //
    for(ui8Idx = 0; ui8Idx < NUM_FRESULT_CODES; ui8Idx++)
    {
        //
        // If a match is found, then return the string name of the error code.
        //
        if(g_psFResultStrings[ui8Idx].iFResult == iFResult)
        {
            return(g_psFResultStrings[ui8Idx].pcResultStr);
        }
    }

    //
    // At this point no matching code was found, so return a string indicating
    // an unknown error.
    //
    return("UNKNOWN ERROR CODE");
}

//------------------------------------------------------------------------------------
// End of FatFS Error Block
//------------------------------------------------------------------------------------

//****************************************************************************
//
// This Varibale is used to define the time before the Watchdog resets the
// Controller
//
//****************************************************************************
unsigned long ulWDTimeSet = 1000000;

//*****************************************************************************
//
// Variable for the Graphics Context
//
//*****************************************************************************
tContext sContext;

//*****************************************************************************
//
// This is the handler for this SysTick interrupt. FatFs requires a timer tick
// every 10 ms for internal timing purposes.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    //
    // Call the FatFs tick timer.
    //
    disk_timerproc();
	
		//
    // Increment the tick count.
    //
    g_ui32TickCount++;
}


//*****************************************************************************
//
// This function reads the contents of a file and sends it over UART. 
// This should only be used on text files.  If it is used on a binary file, 
// then a bunch of garbage is likely to printed on the console.
//
//*****************************************************************************
int
ReadFile(char str[])
{
    FRESULT iFResult;
		TCHAR* path = str;

    //
    // Open the file for reading.
    //
    iFResult = f_open(&g_sFileObject, path, FA_READ);

    //
    // If there was some problem opening the file, then return an error.
		// Else enter a loop to repeatedly read data from the file and send
		// it over UART until the end of the file is reached
    //
    if(iFResult != FR_OK)
    {
        return((int)iFResult);
    }
		else
		{
				while (f_gets(g_pcTmpBuf, sizeof g_pcTmpBuf, &g_sFileObject))
				{
						WatchdogReloadSet(WATCHDOG0_BASE, ulWDTimeSet);
						UARTprintf(g_pcTmpBuf);
				}
		}

    //
		// Close the file
		//
    f_close(&g_sFileObject);
		
		//
    // Return success.
    //
    return(0);
}

//*****************************************************************************
//
// This function writes Text-Strings to a specified File.
// If the File does not exist, it will be created.
//
//*****************************************************************************
int
WriteString(char string[], char name[])
{
		TCHAR* str = string;
		TCHAR* path = name;
		FRESULT iFResult;
		//uint32_t ui32BytesRead;

    //
    // Open the file for reading.
    //
    iFResult = f_open(&g_sFileObject, path, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);

    //
    // If there was some problem opening the file, then return an error.
		// Else move the Write-Pointer to the end of the File and append the 
		// Text-String to the File
    //
    if(iFResult != FR_OK)
    {
        return((int)iFResult);
    }
		else
		{
				iFResult = f_lseek(&g_sFileObject, f_size(&g_sFileObject));
				//f_printf(&g_sFileObject, "%s", str);
				f_puts(str, &g_sFileObject);
		}
		
		//
		// Close the file
		//
		f_close(&g_sFileObject);
		
		//
    // Return success.
    //
		return(0);
}


//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// Configure the UART and its pins.  This must be called before UARTprintf().
// The UART Interface is configured to use 115200 baud.
//
//*****************************************************************************
void
ConfigureUART(void)
{
    //
    // Enable the GPIO Peripheral used by the UART.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Enable UART0
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Configure GPIO Pins for UART mode.
    //
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, 115200, ROM_SysCtlClockGet());
}



//*****************************************************************************
//
// Configure the ADC Hardware and tell it to sample the external Temeprature
// Sensor (ADC_CTL_CH20)
//
//*****************************************************************************
void
ConfigureADC(void)
{
		//
    // The ADC0 peripheral must be enabled for use.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
	
		//
    // Select the external reference for greatest accuracy.
    //
		ROM_ADCReferenceSet(ADC0_BASE, ADC_REF_EXT_3V);
	
		//
    // Enable sample sequence 3 with a processor signal trigger.  Sequence 3
    // will do a single sample when the processor sends a singal to start the
    // conversion.
    //
    ROM_ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);
	
		//
    // Configure step 0 on sequence 3.  Sample the temperature sensor
    // (ADC_CTL_CH20) and configure the interrupt flag (ADC_CTL_IE) to be set
    // when the sample is done.  Tell the ADC logic that this is the last
    // conversion on sequence 3 (ADC_CTL_END). Since we are only doing a single
    // conversion using sequence 3 we will only configure step 0.
    //
    ROM_ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH20 | ADC_CTL_IE |
                             ADC_CTL_END);
														 
		//
    // Since sample sequence 3 is now configured, it must be enabled.
    //
    ROM_ADCSequenceEnable(ADC0_BASE, 3);
		
		//
    // Clear the interrupt status flag.  This is done to make sure the
    // interrupt flag is cleared before we sample.
    //
    ROM_ADCIntClear(ADC0_BASE, 3);
}

//*****************************************************************************
//
// Configuration Function for the SD Card Read/Write Feature.
// It enables the needed Peripherals and mounts the SD-Card with the Fat
// File System.
//
//*****************************************************************************
int
ConfigureSD(void)
{
		//
    // Enable SSI.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);

    //
    // Configure SysTick for a 100Hz interrupt.  The FatFs driver wants a 10 ms
    // tick.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / 100);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();
	
		//
    // Mount the file system, using logical disk 0.
		// Then Create a blank File or truncate and overwrite
		// the old one
    //
    iFResult = f_mount(0, &g_sFatFs);
		iFResult = f_open(&g_sFileObject, cFileName, FA_CREATE_ALWAYS);
		f_close(&g_sFileObject);
		
    if(iFResult != FR_OK)
    {
				return(iFResult);
        //UARTprintf("f_mount error: %s\n", StringFromFResult(iFResult));
    }
		
		//
		// Return without errors
		//
		return(0);
}

//*****************************************************************************
//
// Configuration Function for the Real-Time-Clock.
// It enables the Hibernate Module and sets the RTC to the given Timestamp
//
//*****************************************************************************
void
ConfigureTime(unsigned long starttime)
{
		//
    // Enable the Hibernate module to run.
    //
    HibernateEnableExpClk(SysCtlClockGet());
		
		//
		// Enable the RTC
		//
		HibernateRTCEnable();
	
		//
		// Set the Starttime
		//
		HibernateRTCSet(starttime);
}


//*****************************************************************************
//
// Helper-Function for resetting the Display. Basically it draws a black
// Rectangle across the whole Display.
//
//*****************************************************************************
void
ClearDisplay(void)
{
		tRectangle sRect;
	
		sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&sContext);
    sRect.i16YMax = GrContextDpyHeightGet(&sContext);
	
		GrContextForegroundSet(&sContext, ClrBlack);
		GrRectFill(&sContext, &sRect);
	
		GrFlush(&sContext);
		GrContextForegroundSet(&sContext, ClrWhite);
}


//*****************************************************************************
//
// The program main function.  It performs initialization, then runs a command
// processing loop to read and save the Temperatures. If the SELECT-Button is
// pressed, all of the Data is sent via UART.
// With pressing the UP-Button the "Live-Mode" can be activated. With that
// the Mikrocontroller is sending the Data in Realtime via UART. To deactivate
// the Live-Mode press the DOWN-Button.
//
//*****************************************************************************
int
main(void)
{
	
		//
		// These Variables are used for the Button-Events
		//
		uint8_t ui8ButtonState, ui8ButtonChanged;
	  uint32_t ui32SysClock, ui32LastTickCount;
	
		//
    // This array is used for storing the data read from the ADC FIFO. It
    // must be as large as the FIFO for the sequencer in use.  This example
    // uses sequence 3 which has a FIFO depth of 1.
    //
    uint32_t uiADC0_Value[1];

    //
    // This variable is used to store the temperature conversion.
    //
    long lTemp_ValueC;
	
		//
		// This variable is used to store the current Time
		//
		long lCTime;


    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    ROM_FPULazyStackingEnable();

    //
    // Set the system clock to run at 50MHz from the PLL.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Enable Interrupts
    //
    ROM_IntMasterEnable();
		
		//
    // Initialize the display driver.
    //
    CFAL96x64x16Init();

    //
    // Initialize the graphics context.
    //
    GrContextInit(&sContext, &g_sCFAL96x64x16);
		
		//
		// Initialize the SD-Card
		//
		ConfigureSD();

    //
    // Initialize the UART as a console for text I/O.
    //
    ConfigureUART();

		//
		// Initialize the ADC Hardware
		//
		ConfigureADC();
		
		
		//
    // Initialize locals. Needed for button event processing.
    //
    ui32LastTickCount = 0;
		ui32SysClock = MAP_SysCtlClockGet();
		
		//
    // Initialize the buttons driver.
    //
    ButtonsInit();
		
		
		//
    // Enable the peripherals used by this example.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG0);
		

		//
		// Enable the Watchdog Interrupt
		//
		ROM_IntEnable(INT_WATCHDOG);
		
		/*Registschutz aufheben falls aktiv*/
		if (WatchdogLockState(WATCHDOG0_BASE) == true) 
		{
			WatchdogUnlock(WATCHDOG0_BASE);
		}
		
		WatchdogReloadSet(WATCHDOG0_BASE, ulWDTimeSet);
		WatchdogResetEnable(WATCHDOG0_BASE);
		WatchdogEnable(WATCHDOG0_BASE);
		
		//
    // Configure SysTick to periodically interrupt.
    //
    g_ui32TickCount = 0;
    MAP_SysTickPeriodSet(ui32SysClock / CLOCK_RATE);
    MAP_SysTickIntEnable();
    MAP_SysTickEnable();
		
    //
    // Set the Foreground Color to white
    //
    GrContextForegroundSet(&sContext, ClrWhite);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&sContext, g_psFontCm12);
    GrStringDraw(&sContext, "Waiting for", -1,
                         0, 0, 0);
		GrStringDraw(&sContext, "Time Signal", -1,
                         0, 15, 0);
		GrStringDraw(&sContext, "from PC", -1,
                         0, 30, 0);
		
		//
    // Flush any cached drawing operations.
    //
    GrFlush(&sContext);
	
		//
    // Get a line of text from the user.
    //
    UARTgets(g_cTimeBuf, sizeof(g_cTimeBuf));
		
		ConfigureTime(atol(g_cTimeBuf));
		
    //
    // Enter an infinite loop for reading and processing Temperature data
    //
    while(1)
    {
				//
				// Flush any cached drawing operations.
				//
				GrFlush(&sContext);
			
				//
				// Clear the Display
				//
				ClearDisplay();
			
				//
				// Write the Headline with Line underneath
				//
				GrStringDraw(&sContext, "Reading...", -1, 0, 0, 0);
				GrLineDrawH(&sContext, 0, GrContextDpyWidthGet(&sContext), 13);
			
				//
				// Reset Watchdog Timer
				//
				WatchdogReloadSet(WATCHDOG0_BASE, ulWDTimeSet);
			

				//
        // Each time the timer tick occurs, process any button events.
        //
        if(g_ui32TickCount != ui32LastTickCount)
        {		
						
            //
            // Remember last tick count
            //
            ui32LastTickCount = g_ui32TickCount;

            //
            // Read the debounced state of the buttons.
            //
            ui8ButtonState = ButtonsPoll(&ui8ButtonChanged, 0);
					
            if(BUTTON_PRESSED(SELECT_BUTTON, ui8ButtonState, ui8ButtonChanged))
            {
								//
								// Clear the Display
								//
								ClearDisplay();
							
								//
								// Write the Headline with Line underneath
								//
								GrStringDraw(&sContext, "Sending to PC", -1, 0, 0, 0);
								GrLineDrawH(&sContext, 0, GrContextDpyWidthGet(&sContext), 13);
							
							  ReadFile(cFileName);
            }
						
						if(BUTTON_PRESSED(UP_BUTTON, ui8ButtonState, ui8ButtonChanged))
            {
								strncpy(cLive, "YES", sizeof(cLive));
            }
						
						if(BUTTON_PRESSED(DOWN_BUTTON, ui8ButtonState, ui8ButtonChanged))
            {
								strncpy(cLive, "NO", sizeof(cLive));
            }
            
        }
			
				//
				// Get the current Time from the Hibernate Module
				// to generate a timestamp
				//
				lCTime = HibernateRTCGet();
			
        //
        // Trigger the ADC conversion.
        //
        ROM_ADCProcessorTrigger(ADC0_BASE, 3);

        //
        // Wait for conversion to be completed.
        //
        while(!ADCIntStatus(ADC0_BASE, 3, false))
        {
        }

        //
        // Clear the ADC interrupt flag.
        //
        ROM_ADCIntClear(ADC0_BASE, 3);

        //
        // Read ADC Value.
        //
        ROM_ADCSequenceDataGet(ADC0_BASE, 3, &uiADC0_Value[0]);
				
				//
        // Use non-calibrated conversion provided in the data sheet.
				// Divide last to avoid dropout.
        //
				lTemp_ValueC = (1866300 - ((200000 * uiADC0_Value[0]) /
                               273)) / 1169;
				
				//
				// Write the current Temperature Value to the Display
				//
				sprintf(cTempBuf, "%ld", lTemp_ValueC);
				GrStringDraw(&sContext, "Temp.:", -1, 0, 20, 0);
				GrStringDraw(&sContext, cTempBuf, -1, 40, 20, 0);
				sprintf(cTimeBuf, "%ld", lCTime);
				GrStringDraw(&sContext, "Time:", -1, 0, 35, 0);
				GrStringDraw(&sContext, cTimeBuf, -1, 40, 35, 0);
				GrStringDraw(&sContext, "Live:", -1, 0, 50, 0);
				GrStringDraw(&sContext, cLive, -1, 40, 50, 0); 
				
				sprintf(cCSVBuf, "%ld,%ld\n", lCTime, lTemp_ValueC);
				WriteString(cCSVBuf, cFileName);
				if(strncmp(cLive, "NO", sizeof(cLive)))
				{
						UARTprintf(cCSVBuf);
				}
				//UARTprintf(cCSVBuf);
				
				//
        // This function provides a means of generating a constant length
        // delay. This is used to generate a Value every second.
        //
        SysCtlDelay(SysCtlClockGet() / 3.0);
				
			}
    
}
