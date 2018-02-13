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

#define READ_PORT P1_1

void main (void)
{
	ADCCON1 = 11111110b						// setup the ADC
	IE = 01000000b								// enable only the ADC interrut
	
	
	
}