//*****************************************//
// Heat Enclosure with the MSP430G2553 	   //
// Jack Pedicone		   				   //
// Last updated 3/5/2018				   //			
//*****************************************//

// SUBJECT TO CHANGE
#include <msp430g2553.h>

//Function Setup//
void TimerA0Init(void);
void TimerA1Init(void);
/*void ADC10Init(void);*/
void fanInit(void);
void fanControl(void);
void tempInit(void);
void tempControl(void);
void readInsideTemp(void);
void readOutsideTemp(void);

//http://www.smallbulb.net/2012/238-1-wire-and-msp430
#include "onewire.h"
#include "delay.h"

//Variable declaration//
//unsigned volatile int		adc_in 		= 0;
volatile float				tempC 		= 0;
volatile float              tempF       = 0;
volatile float				voltage 	= 0;

volatile float				tempO		= 0;
volatile float				tempI		= 0;
volatile float				temp		= 0;


int main(void)
{
	WDTCTL = WDTPW + WDTHOLD;		// Stop WDT
	fanInit();						// Fan Pin Initialization
	tempInit();						// Temperature GPIO Initialization
	TimerA0Init();
	TimerA1Init();
	onewire_t ow;
	uint8_t scratchpad[9];
	
	ow.port_out = &P2OUT;
	ow.port_in  = &P2IN;
	ow.port_ren = &P2REN;
	ow.port_dir = &P2DIR;

	//while (REFCTL0 & REFGENBUSY);           // If ref generator busy, WAIT
	//REFCTL0 |= REFON;           // Enable internal 1.2 reference

	//ADC10Init();							// ADC10 Function call

	//while (!(REFCTL0 & REFGENRDY));         // Wait for reference generator
	__enable_interrupt(); 					// Enable interrupts.

	while (1)
	{
		if (!(tempO == 0) && !(tempI == 0))
		{
			TA0CCTL0 &= ~TAIFG;
			tempControl();
		}
		__bis_SR_register(LPM0 + GIE); // Enter LPM0, interrupts enabled
	}
}

void tempInit()
{
	P2DIR |= BIT0; //set pin 2.0 and 2.1 as outputs
	P2DIR |= BIT1;
	P2OUT |= BIT0;
}

void tempControl(void)
{
	temp = tempO + 5;
	if (tempI < temp)
	{
		P1OUT |= BIT2;
	}
	else
	{
		P1OUT &= ~BIT2;
	}
}

void fanInit(void)
{
	P1DIR |= BIT1;		//Pin 1.1
}

void fanControl(void)
{
    //Not used right now

}

/*void ADC10Init(void)
{
	ADC10CTL0 = ADC10IE + ADC10SHT0 + ADC10ON + MSC;        // Set sample time for the ADC10 control register 0, turn ADC10 on
	ADC10CTL1 = ADC10SHT_0 + INCH3 + ADC10SSEL_0 + CONSEQ_2;  // Enable sample timer for the ADC10 control register 1
	ADC10AE0 |= BIT3; 
}*/

void TimerA0Init(void)  //Timer used the temperature sensor
{
	TA0CCTL0 = CCIE;					//Disable timer Interrupt
	TA0CCTL1 = OUTMOD_3;				//Set/Reset when the timer counts to the TA0CCR1 value, reset for TA0CCR0
	TA0CCR1 = 256;
	TA0CCR0 = 4096 - 1;					//Set CCR0 for a ~1kHz clock.
	TA0CTL = TASSEL_1 + MC_1 + ID_3;	//Enable Timer A with SMCLK
}

void TimerA1Init(void)  //Timer used for the fan
{
	TA1CCTL1 = OUTMOD_3;			//Set OUTMOD_3 (set/reset) for CCR1
									//Set initial values for CCR1 (255 -> 254)
	TA1CCR1 = 0xFF;				    //reset and set immediately (May change to slower clock)
	TA1CCR0 = 255 - 1;			    //Set CCR0 for a ~1kHz clock.
	TA1CTL = TASSEL_2 + MC_1;		//Enable Timer A1 with SMCLK and up mode. 1MHz
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
{
	//ADC10CTL0 |= ADC10SC | ENC;	//start ADC conversation
}

void readInsideTemp(void)
{
	ow.pin = BIT0;
 
	onewire_reset(&ow);
	onewire_write_byte(&ow, 0xcc); // skip ROM command
	onewire_write_byte(&ow, 0x44); // convert T command
	onewire_line_high(&ow);
	DELAY_MS(800); // at least 750 ms for the default 12-bit resolution
	onewire_reset(&ow);
	onewire_write_byte(&ow, 0xcc); // skip ROM command
	onewire_write_byte(&ow, 0xbe); // read scratchpad command
	for (i = 0; i < 9; i++) scratchpad[i] = onewire_read_byte(&ow);
}

void readOutsideTemp(void)
{
	ow.pin = BIT2;
 
	onewire_reset(&ow);
	onewire_write_byte(&ow, 0xcc); // skip ROM command
	onewire_write_byte(&ow, 0x44); // convert T command
	onewire_line_high(&ow);
	DELAY_MS(800); // at least 750 ms for the default 12-bit resolution
	onewire_reset(&ow);
	onewire_write_byte(&ow, 0xcc); // skip ROM command
	onewire_write_byte(&ow, 0xbe); // read scratchpad command
	for (i = 0; i < 9; i++) scratchpad[i] = onewire_read_byte(&ow);
}



/*
//ADC ISR
#pragma vector=ADC10_VECTOR
__interrupt void ADC10ISR(void)
{
	adc_in = ADC10MEM;		    // Pull the result of the ADC conversation from the memory register
	
	voltage = adc_in * 0.00029;	    	//converts ADC to voltage
	tempC = voltage / 0.01;		     	//converts voltage to Temp C
	tempF = ((9 * tempC) / 5) + 32;     //Temp C to Temp F
	
	//check to see whether pin 2.0 or 2.1 is on
	// the purpose of these if statements is to receive two temperatures on a single ADC channel
	if ((P2OUT && 0x01) == 1) 
	{
		tempO = tempF;	//Store outside temperature from pin 2.0
		P2OUT &= ~BIT0; 
		P2OUT |= BIT1;	// Flip to pin 2.1
	}
	else
	if ((P2OUT && 0x02) == 1)
	{
		tempI = tempF;	//Store outside temperature from pin 2.0
		P2OUT &= ~BIT1;
		P2OUT |= BIT0;	// Flip to pin 2.0
	}
}
*/

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
{
	readInsideTemp();
	readOutsideTemp();
}