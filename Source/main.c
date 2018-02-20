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

static uint8 count;
static uint16 temp_data;
static uint16 asamp;
static uint16 average;
static uint16 a;

static uint16 message;

// timer interrupt
void timer2 (void) interrupt 5 using 1
{
	TF2 = 0;
}

// ADC interrupt
void ADC1 (void) interrupt 6 using 2
{
	TF2 = 0;																// Reset timer flag, not done by hardware
	count++;																// Iterate the number of averages taken
	temp_data = 0;													// 
	temp_data = ADCDATAH;										// Store high byte
	temp_data = temp_data & 15;							// extract the most significant bits 
	samp = ((temp_data << 8) + ADCDATAL);	// store sample value
	average = ((samp << 2) * 3) + (average << 2);	// TODO troubleshoot avarging errors
}

void delay (uint16 delayVal)
{
	uint16 i;                 // counting variable 
	for (i = 0; i < delayVal; i++)    // repeat  
    {
		  // do nothing
    }
}	// end delay

void send_message(uint8 addr, uint8 instr)
{
	uint8 dummy;										// used to delay between transfers
	
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
}

void disp_setup()
{
	SPICON = 0x34;
	send_message(12,1);		// turn on display
	send_message(11,3);		// set 4 rightmost digits active
	send_message(10,4); 	// intensity equals number of active digits
	send_message(9,0xFF);	// put all digits in decode mode
	send_message(15,0); 	// disable test mode	
}

void main (void)
{
	a = 1;
	ADCCON1 = 0xFE;							// setup the ADC
	IE = 192;									// enable only the ADC interrut
	T2CON = 0x4;								// setup timer 2
	RCAP2L = 214;								// reload value of timer 2
	RCAP2H = 213;
	disp_setup();
	while (1)
	{
		uint8 x = 0;
		send_message(1,x);
		send_message(2,x);
		send_message(3,x);
		send_message(4,x);
		x++;
		delay(1000);
		send_message(1,x);
		send_message(2,x);
		send_message(3,x);
		send_message(4,x);
		x++;
		delay(1000);
		send_message(1,x);
		send_message(2,x);
		send_message(3,x);
		send_message(4,x);
		x++;
		delay(1000);
	}
	
		
}