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
static uint16 message;


// timer interrupt
void timer2 (void) interrupt 5 using 1
{
	if (EXF2)
	{
		uint32 current_samp, samp, overflow_counts;
		uint16 capture;
		
		// get sample of counts occuring between periods
		capture = RCAP2H;																		// extract the most significant bits 
		capture = ((capture << 8) + RCAP2L);								// store sample value
		
		overflow_counts = ((uint32)overflows)<<16;
		current_samp = capture-last_samp;
		samp = current_samp + overflow_counts;			// find difference between last capture and current to get number of cycles completed
		
		average = (samp >> 2) + ((average >> 2) * 3);				// calculate running average
		
		last_samp = capture;																// save the current capture to calculate the sample on the next flag
		overflows = 0;																			// reset the number of overflows
	}
	else
	{
		overflows++;																// if only the timer flag rasied an overflow has occured
	}
	TF2 = 0;																			// Reset timer flag, not done by hardware
	EXF2 = 0;																			// Reset external event flag, not done by hardware
}

// ADC interrupt
void ADC1 (void) interrupt 6 using 2
{
	TF2 = 0;																// Reset timer flag, not done by hardware
	samp = ADCDATAH & 15;							// extract the most significant bits 
	samp = ((samp << 8) + ADCDATAL);	// store sample value
	average = (samp >> 2) + ((average >> 2) * 3);	// calculate running average
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

void long_delay (uint16 delayVal)
{
	uint16 i, j, k;                 // counting variable 
	for (i = 0; i < delayVal; i++)    // repeat  
    {
		  for(j=0; j < 65500; j++)
			{
				for(k=0; k < 100; k++)
					{
						// nothin
					}
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
	
	send_message(10,4); 	// intensity equals number of active digits
	send_message(9,0xFF);	// put all digits in decode mode
	send_message(15,0); 	// disable test mode	
}


void disp_value(uint32 value)
{
	uint32 mV = (adc_val*625L) >> 10;	// scale adc_val by 625/1024 ~= 0.61 to get voltage in mV
	uint8 i, digit;
	for (i = 1; i <= 8; i++)
	{
		digit = value % 10;										// get value of current digit
		if(i == 4) digit = digit | 0x80;			// include decimal pt for leftmost digit to convert to V
		send_message(i,digit);								// update digit on display
		value /= 10;													// move to next digit
	}
}



void main (void)
{
	ADCCON1 = 0xB2;							// setup the ADC
	ADCCON2 = 0x00;
	IE = 192;										// enable only the ADC interrut
	T2CON = 0x4;								// setup timer 2
	RCAP2L = 214;								// reload high byte of timer 2
	RCAP2H = 213;								// reload high byte of timer 2
	disp_setup();								// Call display setup function

	
	while (1)
	{
		uint16 copy = average;
		disp_voltage(copy);
		delay(1000);
	}
	
		
}
