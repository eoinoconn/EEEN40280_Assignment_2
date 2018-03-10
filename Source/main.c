/*Digital and Embedded Systems Assignment 2 - Signal Measurements
	Conor Martin - 13334846
	Eoin O'Connell - 
	
	Insert description of program here
*/
	 
#include <ADUC841.H>
// define some useful variable types

typedef unsigned char uint8;				// 8-bit unsigned integer
typedef unsigned short int uint16;	// 16-bit unsigned integer
typedef unsigned long int uint32;		// 32-bit unsigned integer


#define LOAD T0							// pin 3.4, flashes LED to see when messages are sent to display

static uint8 overflows;			// number of times Timer 2 has overflowed between EXF2 interrupts
static uint16 last_samp;		// memory of counts performed by Timer 2
static uint32 average;			// average DC voltage on ADC0 or frequency of signal on ADC1, depending on mode of operation
static uint16 min_average;	// average min voltage of signal on ADC2
static uint16 max_average;	// average max voltage of signal on ADC2
static uint8 upper_flag;		// flag set if sample of sinusoidal signal on ADC2 is positive
static uint16 min_samp;			// current lowest sample from sinusoidal signal on ADC2
static uint16 max_samp;			// current highest sample from sinusoidal signal on ADC2
static uint8 mode;					// mode of operation: 00: DC voltage, 01: freq of sinusoid, 10: zero-to-peak of periodic, 11: peak-to-peak of periodic

void timer2 (void) interrupt 5 using 1
{
	/*
	Timer 2 ISR
	*/
	
	if (EXF2)
	{
		uint32 samp;
		uint16 capture;
		
		// get sample of counts occuring between periods
		capture = RCAP2H;																		// extract the most significant bits 
		capture = ((capture << 8) + RCAP2L);								// store sample value
		
		// find the number of counts as a result of the overflow
		// find the number of counts between this samp and the last
		// The number of counts is the result fo these two alues
		
		samp = ((uint32)(capture - last_samp) + ((uint32)(overflows << 16)));
		
		average = (samp >> 3) + ((uint32)(average >> 3) * 7);				// calculate the running average of these values
		
		last_samp = capture;																// save the current capture to calculate the sample on the next flag
		overflows = 0;																			// reset the number of overflows
	}
	else
	{
		overflows++;																				// if only the timer flag is rasied then an overflow has occured
	}
	TF2 = 0;																							// Reset timer flag, not done by hardware
	EXF2 = 0;																							// Reset external event flag, not done by hardware
} // end timer2

void ADC1 (void) interrupt 6 using 2
{
	/*
	ADC ISR
	*/
	
	uint16 samp;
	
	// get sample from ADC
	samp = ADCDATAH & 0xF;													// extract the most significant bits 
	samp = ((samp << 8) + ADCDATAL);								// store sample value
	
	
	if (mode == 0x00)
	{
		// get DC voltage, samp is sourced from ADC0
		average = (samp >> 3) + ((average >> 3) * 7);	// calculate running average
	}
	else if (mode == 0x02 | mode == 0x03)
	{
		// get max and min voltage of sinusoid, samp is sourced from ADC2
		if (samp >= 2015)
		{
			// voltage is positive
			if(upper_flag == 0)
			{
				// update min_average
				min_average = (min_samp >> 3) + ((min_average >> 3) * 7); // calculate running average
				min_samp = 2015; // reset min_samp to 'zero'
			} 
			if(samp > max_samp)
			{
				// overwrite max_samp if samp is bigger
				max_samp = samp;
				upper_flag = 1;		// flag is set when voltage is positive
			}			
		}
		
		else if (samp < 2015)
		{
			// voltage is negative
			if (upper_flag == 1)
			{
				// update max_average
				max_average = (max_samp >> 3) + ((max_average >> 3) * 7); // calculate running average
				max_samp = 2015;	// reset max_samp to 'zero'
			}
			if (samp < min_samp)
			{
				// overwrite min_samp if samp is smaller
				min_samp = samp;
				upper_flag = 0; // flag is reset when voltage is negative
			}
		}
	}
	TF2 = 0;	// reset timer flag, not done by hardware
} // end ADC1


void delay (uint16 delayVal)
{
	/*
	Software delay for the processor

	Parameters
		delayVal: delay for delayVal*100 cycles
	*/

	uint16 i, j;
	for (i = 0; i < delayVal; i++)
    {
		  for(j=0; j < 100; j++)
			{
				// nothing
			}
    }
}	// end delay


void send_message(uint8 addr, uint8 instr)
	/* 
	Write data to an address in the 8 digit display

	Parameters:
		addr: address to write data to
		intr: data to write
	*/
{
	volatile uint8 dummy;	// used to delay between transfers
	LOAD = 0;							// prepare the display to accept the new data
	
	SPIDAT = addr & 0xF;	// load addr into shift register
	while (!ISPI)					// wait for transfer to complete
	{
		// do nothing
	}
	ISPI = 0;							// clear SPI interrupt
	dummy = 0x00;					// delay before writing next byte to shift reg
	dummy = 0xFF;					// delay before writing next byte to shift reg
	
	SPIDAT = instr;				// load instr into shift register
	while (!ISPI)					// wait for transfer to complete
	{
		// do nothing
	}
	ISPI = 0;							// clear SPI interrupt
	dummy = 0x00;					// delay before writing next byte to shift reg
	dummy = 0xFF;					// delay before writing next byte to shift reg
	LOAD = 1;							// tell the display to accept the new data
} // end send_message

void disp_setup()
{
	/*
	Setup the 8 digit display before use
	*/
	
	LOAD = 1;							// initialise value
	SPICON = 0x33;				// 
	send_message(12,1);		// turn on display
	send_message(9,0xFF);	// put all digits in decode mode
	send_message(11,4);		// set 5 rightmost digits active
	send_message(10,5); 	// intensity equals number of active digits
	send_message(15,0); 	// disable test mode	
} // end disp_setup


void disp_value(uint32 value)
{
	/*
	Display a 5 digit value on the screen
	
	Parameters:
		value: value to display 
	*/
	uint8 i, digit;
	for (i = 1; i <= 8; i++)
	{
		digit = value % 10;										// get value of i-th digit from left
		if(i == 4) digit = digit | 0x80;			// include decimal pt for fourth digit from left to convert to V/kHz
		send_message(i,digit);								// write digit to display
		value /= 10;													// move to next digit
	}
} // end disp_value

void disp_error()
{
	/*
	Display "HELP" on the screen to show that the frequency of the sinusoidal input is out of range
	*/
	send_message(5,15);		// send " "
	send_message(4,12);		// send H
	send_message(3,11);		// send E
	send_message(2,13);		// send L
	send_message(1,14);		// send P
} // end disp_error

void main (void)
{
	/*
	Setup section, only runs once
	*/
	uint32 display_value;
	uint16 copy_min;
	uint16 copy_max;
	
	P2 = 0xFF;				// write all 1's to P2 so switches can be used as inputs
	average = 0;			// initialise value
	overflows = 0;		// initialise value
	max_samp = 2015;	// initialise value
	min_samp = 2015;	// initialise value
	min_average = 0;	// initialise value
	max_average = 0;	// initialise value
	upper_flag = 0;		// initialise value
	
	
	disp_setup();			// Call display setup function
	
	while (1)
	{
		/*
		Main loop, runs forever
		*/
		mode = P2 & 0x07;
		////////////////////////////////////////////////////
		// DC mode
		////////////////////////////////////////////////////
		if (mode == 0x00)					
		{
			ADCCON1 = 0xB2;				// setup the ADC
			ADCCON2 = 0x00;				// sample from ADC0
			IE = 192;							// enable only the ADC interrupt
			RCAP2L = 214;					// reload high byte of timer 2
			RCAP2H = 213;					// reload high byte of timer 2
			T2CON = 0x4;					// setup timer 2

			display_value = (average*625L) >> 10;	// scale adc_val by 625/1024 ~= 0.61 to get voltage in mV
			disp_value(display_value);
		}
		
		////////////////////////////////////////////////////
		// Frequency Mode
		////////////////////////////////////////////////////
		else if (mode == 0x01)			
		{
			ADCCON1 = 0;					// setup the ADC
			IE = 160;							// enable only the ADC interrupt
			T2CON = 0xD;					// setup timer 2
			T2EX = 0;							// set the input as digital
			
			if ((average < 850) || (average > 55296))						// If the average value is out of range
			{
				disp_error();																			// Error message will be displayed
			}
			else
			{
				disp_value((uint32)11059200/average);							// otherwise, calculate value to display
			}
		}
		
		////////////////////////////////////////////////////
		// Zero-to-Peak Mode
		////////////////////////////////////////////////////
		else if (mode == 0x02)			
		{
			ADCCON1 = 0xB2;
			ADCCON2 = 0x02;				// sample from ADC2
			IE = 192;							// enable only the ADC interrupt
			RCAP2L = 234;					// reload high byte of timer 2
			RCAP2H = 192;					// reload high byte of timer 2
			T2CON = 0x04;					// setup timer 2
			
			copy_max = max_average;
			
			if (copy_max < 2015)
			{
				disp_error();
			}
			else
			{
				display_value = copy_max - 2015;
				display_value = ((uint32)display_value*625) >> 9;
				disp_value(display_value);
			}
		}		
		
		////////////////////////////////////////////////////
		// Peak-to-Peak Mode
		////////////////////////////////////////////////////
		else if (mode == 0x03)			
		{
			ADCCON1 = 0xB2;
			ADCCON2 = 0x02;				// sample from ADC2
			IE = 192;							// enable only the ADC interrupt
			RCAP2L = 234;					// reload high byte of timer 2
			RCAP2H = 192;					// reload high byte of timer 2
			T2CON = 0x04;					// setup timer 2
			
			copy_max = max_average;
			copy_min = min_average;
			
			if (copy_max < copy_min)
			{
				disp_error();
			}
			else
			{
				display_value = copy_max - copy_min;
				display_value = (display_value*625L) >> 9;
				disp_value(display_value);
			}
		}

		delay(131000);
	}
	
		
}
