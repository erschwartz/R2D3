
#include <msp430g2253.h>


//motor control pins
#define MOTOR_R BIT2	// RIGHT MOTOR P1.2
#define MOTOR_L BIT6	// LEFT MOTOR P1.6

// MISC GPIO
#define BUTTON BIT3 	// button P1.3
#define LED BIT5 		// LED P1.5
#define SPEAKER BIT3 	// SPEAKER P2.3

// ADC for photoresistor
#define ADC_Left 0x10  // P1.4 (A4)
#define ADC_Right BIT7 // P1.7 (A7)
#define ADC_INCH4 INCH_4
#define ADC_INCH7 INCH_7
#define ADC_INCH INCH_4

//PWM
#define PWM_TOP 800 //for PWM DIV so DCO/PWM_TOP
#define MID_PWM (PWM_TOP/2)
#define QUARTER_PWM (MID_PWM/2)

// UART RECIEVE
#define RXD BIT1		// RECIEVE FROM BLUETOOTH P1.1

// declarations of functions
void init_adc(void);			// initialize adc
void start_conversion_left(void); 	// converts photoresitor input
void start_conversion_right(void); 	// converts photoresitor input
int get_result(void); 			// obtains result for adc
void init_speaker(void); 		// routine to setup the timer
void init_button(void);  		// routine to setup the button
void init_motor(void);	 		// initializes timer for servo motor
void init_uart(void);			// initiliazes uart for recieve from bluetooth module
void exec_cmd(void);			// executes command sent from bluetooth module
void exec_auto(void);			// executes autopilot mode

// global variables
unsigned char modes = 'x';		// modes determines what robot does recieved by uart
int playmusic;			// mode determines whether to play music
int blink_counter;		// counter for music
int music_count = 0;	// counter for indexing music array
int autopilot = 0; 		// 1 means autopilot, 0 means controlled by app
unsigned delay_hits=0;	// counter to estimate the busy wait time
volatile int latest_result; 	// stores latest adc result
int leftval; 			// adc value for left photoresistor
int rightval; 			// adc value for right photoresistor
int auto_on = 0;

// Arrays for Speaker
// holds time period for notes
const int tone_array[37] = {0,1275,0,1275,0,1275,0,1607,0,1072,0,1275,// G G G Eb Bb G
								0,1607,0,1072,0,1275,0,						// Eb Bb G
								851,0,851,0,851,0,							// Bb Bb Bb
								803,0,1072,0,1351,0,1607,0,1072,0,1275,0};	// Eb Bb Gb Eb Bb G
// holds lengths for each notes
const float note_lengths[37] = {1,4,0.5,4,0.5,4,0.5,2,0.5,1,0.5,4, // G G G Eb Bb G
									0.5,2,0.5,1,0.5,8,0.5,				// Eb Bb G
									4,0.5,4,0.5,4,0.5,					// Bb Bb Bb
									2,0.5,1,0.5,4,0.5,2,0.5,1,0.5,8,8};	// Eb Bb Gb Eb Bb G

unsigned int count = 0;						// Used for the flashing LED demonstration
void init_uart(void);

int main(void)
{
	/*** Set-up system clocks ***/
	 // setup the watchdog timer as an interval timer
	  WDTCTL =(WDTPW + // (bits 15-8) password
					   // bit 7=0 => watchdog timer on
					   // bit 6=0 => NMI on rising edge (not used here)
					   // bit 5=0 => RST/NMI pin does a reset (not used here)
			   WDTTMSEL + // (bit 4) select interval timer mode
			   WDTCNTCL +  // (bit 3) clear watchdog timer counter
					  0 // bit 2=0 => SMCLK is the source
					  +1 // bits 1-0 = 01 => source/8K
			   );
	  IE1 |= WDTIE;		// enable the WDT interrupt (in the system interrupt register IE1)

	P1OUT = 0; //turn off all outputs
	P2OUT = 0; // turn off all outputs


	DCOCTL = 0;								// Select lowest DCOx and MODx settings
	BCSCTL1 = CALBC1_1MHZ;					// Set DCO
	DCOCTL = CALDCO_1MHZ;

	// setup motors
	P1SEL |= MOTOR_L + MOTOR_R;
	P1DIR |= MOTOR_L + MOTOR_R;

	//setup led
	P1DIR |= LED;
	P1OUT &= LED;

	init_speaker();  // initialize timer for speaker
	init_button(); 	 // initialize the button
	init_adc();		 // initialize adc for light seeking
	init_motor();	 // initialize Timer_A for PWM
	init_uart();	 // setup uart recieve from bluetooth

	_bis_SR_register(GIE+LPM0_bits);// enable general interrupts and power down CPU

}

// initiliaze uart for bluetooth
void init_uart()
{
	P1SEL = RXD;					// P1.1 = RXD, P1.2=TXD
	P1SEL2 = RXD;					// P1.1 = RXD, P1.2=TXD
	UCA0CTL1 |= UCSSEL_2;					// SMCLK
	UCA0BR0 = 104;							// 1MHz 9600
	UCA0BR1 = 0;							// 1MHz 9600
	UCA0MCTL = UCBRS0;						// Modulation UCBRSx = 1
	UCA0CTL1 &= ~UCSWRST;					// Initialize USCI state machine
	IE2 |= UCA0RXIE;						// Enable USCI_A0 RX interrupt
}

// +++++++++++++++++++++++++++
// Sound Production System
void init_speaker(){              // initialization and start of timer
	TA1CTL |= TACLR;            // reset clock
	TA1CTL = TASSEL_2+ID_0+MC_1;// clock source = SMCLK
	                            // clock divider=1
	                            // UP mode
	                            // timer A interrupt off
	TA1CCTL0 = OUTMOD_4;     	// Reset/Set
	TA1CCR0 = 0;

	P2DIR |= SPEAKER;
	P2OUT |= SPEAKER;
}

void init_motor(void) {

    TA0CCR0 = MID_PWM;       			// TACCR0 controls the PWM frequency
    TA0CCR1 = 0;             			// 0.0% duty cycle for PWMTop = 1000
    TA0CTL = TASSEL_2 + ID_0 + MC_1;		// SMCLK, div 1, Up Mode

    TA0CCTL1 = OUTMOD_7;     			// Reset/Set: Sets at TACCR0, resets at TACCR1

}

 /* basic adc operations */

void init_adc(){
 	ADC10CTL1= ADC_INCH4	//input channel bit field
			  +SHS_0 //use ADC10SC bit to trigger sampling
			  +ADC10DIV_4 // ADC10 clock/5
			  +ADC10SSEL_0 // Clock Source=ADC10OSC
			  +CONSEQ_0; // single channel, single conversion
			  ;
	ADC10AE0 = ADC_Left; // enable analog input
	ADC10CTL0= SREF_0	//reference voltages are Vss and Vcc
	          +ADC10SHT_3 //64 ADC10 Clocks for sample and hold time (slowest)
	          +ADC10ON	//turn on ADC10
	          +ENC		//enable (but not yet start) conversions
	          ;
}

 void start_conversion_left(){
	 ADC10CTL0 &= ~ENC;
	 //set to input pin A4 (P1.4) to get left photoresistor values
	 ADC10CTL1= ADC_INCH4	//input channel bit field
	  			  +SHS_0 //use ADC10SC bit to trigger sampling
	  			  +ADC10DIV_4 // ADC10 clock/5
	  			  +ADC10SSEL_0 // Clock Source=ADC10OSC
	  			  +CONSEQ_0; // single channel, single conversion
	  			  ;

	 ADC10AE0 = ADC_Left; // enable analog input

	 ADC10CTL0 |= ENC;


	 ADC10CTL0 |= ADC10SC;
 }

 void start_conversion_right(){
	 ADC10CTL0 &= ~ENC;
	 //set to input pin A7 (P1.7) to get right photoresistor values
	 ADC10CTL1= ADC_INCH7	//input channel bit field
	  			  +SHS_0 //use ADC10SC bit to trigger sampling
	  			  +ADC10DIV_4 // ADC10 clock/5
	  			  +ADC10SSEL_0 // Clock Source=ADC10OSC
	  			  +CONSEQ_0; // single channel, single conversion
	  			  ;
	 ADC10AE0=ADC_Right; // enable analog input

	 ADC10CTL0 |= ENC;

	 ADC10CTL0 |= ADC10SC;
 }

int get_result(){
 	delay_hits=0;
 	while (ADC10CTL1 & ADC10BUSY) {++delay_hits;}// busy wait for busy flag off
 	return ADC10MEM;
 }

void init_button(){
  P1OUT |= BUTTON; // pullup
  P1REN |= BUTTON; // enable resistor
  P1IES |= BUTTON; // set for 1->0 transition
  P1IFG &= ~(BUTTON);// clear interrupt flag
  P1IE  |= BUTTON; // enable interrupt
}

void interrupt button_handler(){

	if (P1IFG & BUTTON){

		autopilot = 1; 	// autopilot means not running with bluetooth app
		auto_on ^= 1;	// toggles turning off and on autopilot

		P1OUT ^= LED;			// toggle led
		P2SEL ^= SPEAKER;		// toggle speaker

		P1IFG &= ~BUTTON;		// clear flag
		TACCTL0 ^= OUTMOD_4;
 	 }

}

interrupt void WDT_interval_handler(){

	if( autopilot ){ // use light seeking for autopilot (not controlled by bluetooth)
		exec_auto();
	}
	else exec_cmd(); // carries out bluetooth commands

	if(playmusic)
	{
		if( --blink_counter == 0){
			music_count++;

			if(music_count == 37){	// if end of music array
				TA1CCR0 = 0;		// don't play a tone
				playmusic = 0;		// stop playing music
				music_count = 0;	// reset count
			}
			else
			{
				TA1CCR0 = tone_array[music_count];					// set to next tone
				blink_counter = note_lengths[music_count] * 15;		// set to next note length
			}
		}
	}

}

void exec_auto(){
	// executes autopilot mode
	if( auto_on ){ // turn everything on
				// get value of left photoresistor
				start_conversion_left();		// convert adc of left photoresistor
				latest_result = get_result();				// store result
				leftval = latest_result;

				// get value of right photoresistor
				start_conversion_right();		// convert adc of right photoresistor
				latest_result = get_result();				// store result
				rightval = latest_result;

				// compare the values to determine which direction the robot should head towards
				if(leftval < rightval){   		// if left is darker, turn right
					P1SEL &= ~(MOTOR_R);			// disable right motor to turn right
					P1SEL |= MOTOR_L;				// enable left motor
					TA0CCR0 = QUARTER_PWM;			// set PWM
					TA0CCR1 = 100;
				}
				else if( rightval < leftval ){	// if right is darker, turn left
					P1SEL &= ~(MOTOR_L);			// disable left motor
					P1SEL |= MOTOR_R;				// enable right motor
					TA0CCR0 = QUARTER_PWM;		// set PWM
					TA0CCR1 = 100;
				}
				else{							// if both values are equal, go straight
					P1SEL |= MOTOR_L + MOTOR_R;		// enable both motors
					TA0CCR0 = MID_PWM;				// set PWN
					TA0CCR1 = 100;
				}

				if(!playmusic){
					playmusic = 1;										// mode to play music
					music_count = 0;									// reset music count
					blink_counter = note_lengths[music_count];			// set note length
					TA1CCR0 = tone_array[music_count];					// set to next tone
				}

			}
			else{
				// turn motors off
				P1SEL &= ~(MOTOR_L + MOTOR_R);	// diable motors
				TA1CCR0 = 0;					// set pwm for motor to 0
				playmusic = 0;					// turn off music
			}
}

// carries out bluetooth commands
void exec_cmd(){
	switch (modes)
	{
		case 'X': 	// stop everything
			P1SEL &= ~(MOTOR_L + MOTOR_R);	// diable motors
			TA1CCR0 = 0;					// turn off tone
			P2SEL &= ~SPEAKER;				// turn off speaker
			P1OUT &= ~LED;					// turn led off
			playmusic = 0;
			modes = 'O';
			break;
		case 'F':	// go forward
			P1SEL |= MOTOR_L + MOTOR_R;		// enable both motors
			TA0CCR0 = QUARTER_PWM;			// set PWN
			TA0CCR1 = 100;
			modes = 'O';
			break;
		case 'S':							// stop going forward
			P1SEL &= ~(MOTOR_L + MOTOR_R);	// disable motors
			modes = 'O';
			break;
		case 'R':							// turn right
			P1SEL &= ~(MOTOR_R);			// disable right motor
			P1SEL |= MOTOR_L;				// enable left motor
			TA0CCR0 = QUARTER_PWM;			// set PWM
			TA0CCR1 = 100;
			modes = 'O';
			break;
		case 'L':							// turn left
			P1SEL &= ~(MOTOR_L);			// disable left motor
			P1SEL |= MOTOR_R;				// enable left motor
			TA0CCR0 = QUARTER_PWM;			// set PWM
			TA0CCR1 = 100;
			modes = 'O';
			break;
		case 'B':							// turn led on
			P1OUT |= LED;
			modes = 'O';
			break;
		case 'T':							// turn led off
			P1OUT &= ~LED;
			modes = 'O';
			break;
		case 'P':	// play tones
			P2SEL |= SPEAKER;				// enable speaker
			playmusic = 1;					// set mode to play music
			music_count = 0;				// reset music count
			blink_counter = note_lengths[music_count];			// set note length
			TA1CCR0 = tone_array[music_count];					// set to next tone
			modes = 'O';
			break;
		case 'O': break; // mode to continue doing previous command
		default: break;
	}
}


#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void){

	modes = UCA0RXBUF;					// Assign received byte to Rx_Data
	autopilot = 0;						// turns off autopilot
}


ISR_VECTOR(button_handler,".int02") // declare interrupt vector for button
// +++++++++++++++++++++++++++
// DECLARE function WDT_interval_handler as handler for interrupt 10
// using a macro defined in the msp430g2553.h include file
ISR_VECTOR(WDT_interval_handler, ".int10")
