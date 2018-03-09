/* Digital and Embedded Systems 
	 Generates a square wave on P3.6 with adjustable frequency.
	 Switches on P2.2 to P2.0 set the frequency, by setting the 
	 reload value for timer 2 according to values in a look-up table.
	 Switch on P2.7 enables interrupts, so enabling this output.
	 In parallel, generates a slow square wave on P3.4 to flash an
	 LED - this uses a software delay, independent of interrupts.
	 Main program configures timer and then loops, checking switches,
	 setting reload value, enabling interrupts and flashing LED.
	 Timer 2 ISR changes state of ouptut on P3.6 and clears flag.
	 
	 Frequency calculation, based on clock frequency 11.0592 MHz:
	 For 100 Hz output, want interrupts at 200 Hz, or every 
	 55296 clock cycles.  So set timer 2 reload value to 
	 (65536 - 55296) = 10240. 
	 */
	 
#include <ADUC841.H>
// define some useful variable types

typedef unsigned char uint8;				// 8-bit unsigned integer
typedef unsigned short int uint16;	// 16-bit unsigned integer
typedef unsigned long int uint32;


#define LOAD T0

static uint8 overflows;
static uint16 last_samp;
static uint32 average;
static uint16 min_average;
static uint16 max_average;
static uint16 message;
static uint8 upper_flag;
static uint16 min_samp;
static uint16 max_samp;
static uint8 mode;

// timer interrupt
void timer2 (void) interrupt 5 using 1
{
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
}

// ADC interrupt
void ADC1 (void) interrupt 6 using 2
{
	uint16 samp;
	
	// get sample of ADC
	samp = ADCDATAH & 0xF;													// extract the most significant bits 
	samp = ((samp << 8) + ADCDATAL);								// store sample value
	
	
	if (mode == 0x00)
	{
		average = (samp >> 3) + ((average >> 3) * 7);	// calculate running average
	}
	else if (mode == 0x02 | mode == 0x03)
	{
		if (samp >= 2015)
		{
			if(upper_flag == 0)
			{
				min_average = (min_samp >> 4) + ((min_average >> 4) * 15);
				min_samp = 2015;
			}
			if(samp > max_samp)
			{
				max_samp = samp;
				upper_flag = 1;
			}			
		}
		else if (samp < 2015)
		{
			if (upper_flag == 1)
			{
				max_average = (max_samp >> 4) + ((max_average >> 4) * 15);
				max_samp = 2015;
			}
			if (samp < min_samp)
			{
				min_samp = samp;
				upper_flag = 0;
			}
		}
	}
	
	TF2 = 0;																			// Reset timer flag, not done by hardware
}


void delay (uint16 delayVal)
{
	uint16 i, j;                 // counting variable 
	for (i = 0; i < delayVal; i++)    // repeat  
    {
		  for(j=0; j < 100; j++)
			{
				// nothin
			}
    }
}	// end delay


void send_message(uint8 addr, uint8 instr)
{
	volatile uint8 dummy;										// used to delay between transfers
	LOAD = 0;
	
	SPIDAT = addr & 0xF;						// load addr into shift register
	while (!ISPI)										// wait for transfer to complete
	{
		// do nothing
	}
	ISPI = 0;												// clear SPI interrupt
	dummy = 0x00;										// delay before writing next byte to shift reg
	dummy = 0xFF;
	
	SPIDAT = instr;									// load instr into shift register
	while (!ISPI)										// wait for transfer to complete
	{
		// do nothing
	}
	ISPI = 0;												// clear SPI interrupt
	dummy = 0x00;										// delay before writing next byte to shift reg
	dummy = 0xFF;
	LOAD = 1;
}

void disp_setup()
{
	LOAD = 1;
	SPICON = 0x33;
	send_message(12,1);		// turn on display
	send_message(9,0xFF);	// put all digits in decode mode
	send_message(11,4);		// set 4 rightmost digits active
	send_message(10,5); 	// intensity equals number of active digits
	send_message(15,0); 	// disable test mode	
}


void disp_value(uint32 value)
{
	uint8 i, digit;
	for (i = 1; i <= 8; i++)
	{
		digit = value % 10;										// get value of current digit
		if(i == 4) digit = digit | 0x80;			// include decimal pt for leftmost digit to convert to V
		send_message(i,digit);								// update digit on display
		value /= 10;													// move to next digit
	}
}

void disp_error()
{
	send_message(5,15);		// send " "
	send_message(4,12);		// send H
	send_message(3,11);		// send E
	send_message(2,13);		// send L
	send_message(1,14);		// send P
}

void main (void)
{
	uint32 display_value;
	uint16 copy_min;
	uint16 copy_max;
	
	P2 = 0xFF;
	average = 0;
	overflows = 0;
	max_samp = 2015;
	min_samp = 2015;
	min_average = 0;
	max_average = 0;
	upper_flag = 0;
	
	
	disp_setup();			// Call display setup function
	
	while (1)
	{
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
			
			if ((average < 850) || (average > 55296))
			{
				disp_error();
			}
			else
			{
				// calculate value to display
				disp_value((uint32)11059200/average);
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

		delay(3310);
	}
	
		
}
