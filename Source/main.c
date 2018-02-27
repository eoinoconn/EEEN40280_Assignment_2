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


#define LOAD T0

static uint8 mode;
static uint16 samp;
static uint16 average;
static uint16 message;
static uint16 aboveorbelow;
static uint16 count;
static uint16 completed_periods;

// timer interrupt
void timer2 (void) interrupt 5 using 1
{
	TF2 = 0;
}

// ADC interrupt
void ADC1 (void) interrupt 6 using 2
{
	TF2 = 0;																			// Reset timer flag, not done by hardware
	samp = ADCDATAH & 0xF;												// extract the most significant bits 
	samp = ((samp << 8) + ADCDATAL);							// store sample value
	if (mode == 0)
	{
		average = (samp >> 2) + ((average >> 2) * 3);	// calculate running average
	}
	else if (mode == 1)
	{
		// if a 0 crossover has not occured
		if ((samp >= 2048) & (aboveorbelow == 1) | ((samp < 2048) & (aboveorbelow == 0)))
		{
			count++;
		}
		// a zero crossover has occured
		else
		{
			completed_periods = count;
			count = 0;
			aboveorbelow = ~aboveorbelow;
		}
	}
}

// do nothing for delayVal * 100 cycles
void delay (uint16 delayVal)
{
	uint16 i, j;
	for (i = 0; i < delayVal; i++)
    {
		  for(j=0; j < 100; j++)
			{
				// nothin
			}
    }
}	// end delay

// write instruction to register address
void send_message(uint8 instr, uint8 addr)
{
	volatile uint8 dummy;	// used to delay between transfers
	LOAD = 0;							// prepare display for new data
	
	// send first byte
	SPIDAT = addr;				// load addr into shift register
	while (!ISPI)					// wait for transfer to complete
	{
		// do nothing
	}
	ISPI = 0;							// clear SPI interrupt
	dummy = 0x00;					// delay before writing next byte to shift reg
	dummy = 0xFF;
	
	// send second byte
	SPIDAT = instr;				// load instr into shift register
	while (!ISPI)					// wait for transfer to complete
	{
		// do nothing
	}
	ISPI = 0;							// clear SPI interrupt
	dummy = 0x00;					// delay before writing next byte to shift reg
	dummy = 0xFF;
	
	LOAD = 1;							// make display accept new data
}

void disp_setup()
{
	LOAD = 1;
	SPICON = 0x33;
	send_message(12,1);		// turn on display
	send_message(11,3);		// set 4 rightmost digits active
	send_message(10,4); 	// intensity equals number of active digits
	send_message(9,0xFF);	// put all digits in decode mode
	send_message(15,0); 	// disable test mode	
}

void disp_voltage(uint16 adc_val)
{
	uint16 mV = (adc_val >> 10) * 625;	// scale adc_val by 625/1024 ~= 0.61 to get voltage in mV
	uint8 i, digit;
	for (i = 1; i <= 4; i++)
	{
		digit = mV % 10;									// get value of current digit
		if(i == 4) digit = digit | 0x80;	// include decimal pt for leftmost digit to convert to V
		send_message(i,digit);						// update digit on display
		mV /= 10;													// move to next digit
	}
}

void disp_freq(uint16 adc_val)
{
	uint16 mV = (adc_val >> 10) * 625;	// scale adc_val by 625/1024 ~= 0.61 to get voltage in mV
	uint8 i, digit;
	for (i = 1; i <= 4; i++)
	{
		digit = mV % 10;									// get value of current digit
		if(i == 4) digit = digit | 0x80;	// include decimal pt for leftmost digit to convert to V
		send_message(i,digit);						// update digit on display
		mV /= 10;													// move to next digit
	}
}

void main (void)
{
	ADCCON1 = 0xFE;			// setup the ADC
	ADCCON2 = 0x01;
	IE = 192;						// enable only the ADC interrupt
	T2CON = 0x4;				// setup timer 2
	aboveorbelow = 0;		// setup global variables for frequency mesurement
	count = 0;
	completed_periods = 0;

	disp_setup();			// Call display setup function

	while (1)
	{
		if (mode == 0)					// DC mode
		{
			RCAP2L = 214;			// reload high byte of timer 2
			RCAP2H = 213;			// reload high byte of timer 2
			disp_voltage(average);		// passing the global variable straight to display, if the value is read wrong once in a blue moon it will only be wrong very griefly
			delay(65535);
		}
		else if (mode == 1)			// Frequency Mode
		{
			RCAP2L = 253;			// reload high byte of timer 2
			RCAP2H = 232;			// reload high byte of timer 2
			
			disp_freq(completed_periods);
			delay(65535);
		}
	}
	
		
}
