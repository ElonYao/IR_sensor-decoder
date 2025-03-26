/*Use of AI / Cognitive Assistance Software is not allowed in any evaluation, assessment or exercise.*/
/*=============================================================================
	File Name:	IR_sensor.c  
	Author:		
	Date:		
	Modified:	None
	� Fanshawe College, 2025

	Description: 
=============================================================================*/

/* Preprocessor ===============================================================
   Hardware Configuration Bits ==============================================*/
   #pragma config FOSC		= INTIO67
   #pragma config PLLCFG	= OFF
   #pragma config PRICLKEN = ON
   #pragma config FCMEN	= OFF
   #pragma config IESO		= OFF
   #pragma config PWRTEN	= OFF 
   #pragma config BOREN	= ON
   #pragma config BORV		= 285 
   #pragma config WDTEN	= OFF
   #pragma config PBADEN	= OFF
   #pragma config LVP		= OFF
   #pragma config MCLRE	= EXTMCLR
   
   // Libraries ==================================================================
   #include <p18f45k22.h>
   #include <stdio.h>
   #include <string.h>
   // Constants  =================================================================
 
   
   // Global Variables  ==========================================================
    typedef struct 
    {
        char receivedData[5];
        char dataIndex;
        char insertIndex;
        char dataByte;
        unsigned int counter;
        unsigned int lastCounter;
        unsigned int deltaCounter;
        char newKey;
        char timerOverFlow;
        char command;

    }IRsensor_t;
    IRsensor_t IRsensor={0,0,0,0,0,0,0,0,0,0};
   // Functions  =================================================================
   void isrHigh(void);
   /*>>> clockConfig: ===========================================================
   Author:		Elon
   Date:		23/05/2024
   Modified:	None
   Desc:		Config the system clock to 4MHz and wait for it becoming stable.
   Input: 		None
   Returns:	None 
    ============================================================================*/
   void clockConfig(void)
   {
       OSCCON=0x52;//set FOSC to 4Mhz and choose internal oscillator
       while(!OSCCONbits.HFIOFS);
   } // eo clockConfig::
   
   /*>>> IOInit: ===========================================================
   Author:		Elon
   Date:		23/05/2024
   Modified:	Elon 20/01/2025
   Desc:	
   Input: 		None
   Returns:	None 
    ============================================================================*/
   void IOInit(void)
   {
        ANSELB=0;
        ANSELC=0;
        TRISBbits.RB5=1;
        PORTBbits.RB5=0;
        TRISC=0XFF;

   }// eo IOInit::
   
   /*>>> uartInit: ===========================================================
   Author:		Elon
   Date:		20/01/2025
   Modified:	None
   Desc:		Serial port 1, 19200 baud rate, 8 bit data, no parity, 1 stop bit, rx and tx enabled
   Input: 		None
   Returns:	None 
    ============================================================================*/
   void uartInit(void)
   {
       BAUDCON1=0x40;
       RCSTA1=0x90;
       SPBRG1=12;
       TXSTA1=0x26;
   }// eo uartInit::
   /*>>> timer0Init: ===========================================================
   Author:		Elon
   Date:	20/01/2025
   Modified:	None
   Desc:		Timer0 on, internal clock source, 1:2 prescaler for 100ms rollover(65536-63536)*1us*2=10ms.
   Input: 		None
   Returns:	None 
    ============================================================================*/
   void timer0Init(void)
   {
       T0CON=0x90;
       TMR0H=0xF8;
       TMR0L=0x30;	
       
   }// eo timer0Init::
   /*>>> timer0Reset: ===========================================================
   Author:		Elon
   Date:	14/06/2024
   Modified:	None
   Desc:		Clear timer0 overflow flag and reload the false start value(0x3CB0) to TMR0H and TMR0L
   Input: 		None
   Returns:	None 
    ============================================================================*/
   void timer0Reset(void)
   {
       INTCONbits.TMR0IF=0;
       TMR0H=0xF8;
       TMR0L=0x30;	
       
   }// eo timer0Reset::
   void timer1Init(void)
   {
        T1GCON=0xD0;// gate enable/active high/single pulse on/gate pin
        PIR1bits.TMR1IF=0;
        PIE1bits.TMR1IE=1;
        //gate control interrupt enable 
        PIR3bits.TMR1GIF=0;
        PIE3bits.TMR1GIE=1;
        T1CON=0x03;// Fosc/4, 1:1 pv 16bit and turn on and counting frequency is 1M
   }
   unsigned int readTimer1(void)
   {
        unsigned int currentValue=0;
        currentValue=TMR1L;
        currentValue|=((TMR1H & 0xFFFF)<<8);
        return currentValue;
   }
    char dataDecoding(IRsensor_t *sensor)
    {
        if(sensor->newKey)
        {
            sensor->newKey=0;
            if(sensor->receivedData[0]==0x00 && sensor->receivedData[1]==0xFF)
            {
                if(sensor->receivedData[2]==0x18 && sensor->receivedData[3]==0xE7)
                {
                    sensor->command= 'F';
                }
                else if(sensor->receivedData[2]==0x52 && sensor->receivedData[3]==0xAD)
                {
                    sensor->command='B';
                }
                else if(sensor->receivedData[2]==0x08 && sensor->receivedData[3]==0xF7)
                {
                    sensor->command='L';
                }
                else if(sensor->receivedData[2]==0x5A && sensor->receivedData[3]==0xA5)
                {
                    sensor->command='R';
                }
                else if(sensor->receivedData[2]==0x1C && sensor->receivedData[3]==0xE3)
                {
                    sensor->command='C';
                }
                memset(sensor->receivedData,0,5);
            }
            while(!PORTBbits.RB5);
            T1GCONbits.T1GGO=1;//start the next listening
        }
        return sensor->command;
    }
   /*>>> systemInitialization: ===========================================================
   Author:		Elon
   Date:		07/06/2024
   Modified:	Elon 20/01/2025
   Desc:		Organize all initializaion functions into one function.
   Input: 		None
   Returns:	None 
    ============================================================================*/
   void systemInitialization(void)
   {
       clockConfig();
       IOInit();
       uartInit();
       timer0Init();
       timer1Init();
       INTCONbits.GIE=1;
       INTCONbits.PEIE=1;
   }// eo systemInitialization::
   
   /*=== MAIN: FUNCTION ==========================================================
    ============================================================================*/
 void main( void )
 {
     systemInitialization();
     T1GCONbits.T1GGO=1;
     IRsensor.lastCounter=readTimer1();
     while(1)
     {
         dataDecoding(&IRsensor);
         //for testing only
         if(INTCONbits.TMR0IF)
         {  
            printf("\r\n%c",IRsensor.command);
            timer0Reset();
         }
     }

}// eo main::
   
#pragma code interruptVectorHigh = 0x08
void interruptVectorHigh(void)
{
    _asm
    goto isrHigh
    _endasm
}
#pragma code
#pragma interrupt isrHigh
void isrHigh(void)
{
    if(PIR3bits.TMR1GIF&&PIE3bits.TMR1GIE)
    {
        PIR3bits.TMR1GIF=0;
        IRsensor.counter=readTimer1();
        IRsensor.deltaCounter=IRsensor.counter+65536*IRsensor.timerOverFlow-IRsensor.lastCounter;
        IRsensor.timerOverFlow=0;
        //Identify the start condition
        if(IRsensor.deltaCounter>4000 && IRsensor.deltaCounter<5000)
        {
            IRsensor.dataIndex=0;
            IRsensor.dataByte=0;
            IRsensor.insertIndex=0;
        }
        //Identify mark
        else if(IRsensor.deltaCounter>1400 && IRsensor.deltaCounter<1800)
        {
            IRsensor.dataByte>>=1;
            IRsensor.dataByte|=0x80;
            IRsensor.dataIndex++;
        }
        //Identify space
        else if(IRsensor.deltaCounter>400 && IRsensor.deltaCounter<700)
        {
            IRsensor.dataByte>>=1;
            IRsensor.dataIndex++;
        }
        if(IRsensor.dataIndex==8)
        {
            IRsensor.receivedData[IRsensor.insertIndex]=IRsensor.dataByte;
            IRsensor.insertIndex++;
            IRsensor.dataIndex=0;
            IRsensor.dataByte=0;
        }
        if(IRsensor.insertIndex>=4)
        {
            IRsensor.newKey=1;
            IRsensor.insertIndex=0;
        }
        else
        {
            T1GCONbits.T1GGO=1;//start the next listening
        }
        IRsensor.lastCounter=IRsensor.counter;
    }
    if (PIR1bits.TMR1IF && PIE1bits.TMR1IE)
    {
        IRsensor.timerOverFlow++;
		PIR1bits.TMR1IF=0;
    }   
}   
   