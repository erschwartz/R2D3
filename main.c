#include <msp430.h>
#include <stdlib.h>

asm(" .length 10000");
asm(" .width 132");

//motor control pins
#define MOTOR_R BIT2	// RIGHT MOTOR
#define MOTOR_L BIT6	// LEFT MOTOR

// ACCESSORY PINS
#define BUTTON BIT3		// BUTTON
#define LED BIT4 		// LED
#define SPEAKER BIT3 	// SPEAKER

// UART RECIEVE
#define RXD BIT1		// RECIEVE FROM BLUETOOTH

//PWM
#define PWM_TOP 1000 //for PWM DIV so DCO/PWM_TOP
#define FULL_PWM PWM_TOP
#define MID_PWM (FULL_PWM/2)

// Functions
void motor_init(void);
void init_button(void);
void init_speaker(void);
void init_uart(void);

// music_array holds the sheet music order for the notes
//const int tone_array[4] = {286, 318, 379, 478};
const int tone_array[4] = {2860, 3180, 3790, 4780};
int note_lengths[4] = {10, 20, 15, 30};

// global variables
char modes = 'x';
int playmusic;
int blink_counter;
int music_count = 0;

void main(void) {
    //WDTCTL = WDTPW + WDTHOLD; //disable watchdog
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

    // setup motors
    P1SEL |= MOTOR_L + MOTOR_R;
    P1DIR |= MOTOR_L + MOTOR_R;

    //setup led
    P1DIR |= LED;
    P1OUT &= LED;

    //setup speaker
    init_speaker();

    // setup uart recieve from bluetooth
    //init_uart();

    BCSCTL1 = CALBC1_1MHZ;          	// Running at 1 MHz
	DCOCTL = CALDCO_1MHZ;

    motor_init();							//initialize Timer_A for PWM
    init_button(); 						// initialize the button
    _bis_SR_register(GIE+LPM0_bits);	// enable general interrupts and power down CPU

}

// setup uart to recieve
void init_uart(void){
	P2DIR |= 0xFF;                            // All P2.x outputs
	P2OUT &= 0x00;                            // All P2.x reset
	P1SEL |= RXD;                    		  // P1.1 = RXD, P1.2=TXD
	P1SEL2 |= RXD;                   		  // P1.1 = RXD, P1.2=TXD
	UCA0CTL1 |= UCSSEL_2;                     // SMCLK
	UCA0BR0 = 0x08;                           // 1MHz 115200
	UCA0BR1 = 0x00;                           // 1MHz 115200
	UCA0MCTL = UCBRS2 + UCBRS0;               // Modulation UCBRSx = 5
	UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
	UC0IE |= UCA0RXIE;                        // Enable USCI_A0 RX interrupt
}

// Sound Production System
void init_speaker(void){ // initialization and start of timer

	TA1CTL |= TACLR;            	  // reset clock
	TA1CTL = TASSEL_2+ID_0+MC_1;	  // clock source = SMCLK
									  // clock divider=1

	TA1CCTL0 = 0; 					  // compare mode, output mode 0, no interrupt enabled
	P2SEL|=SPEAKER;                   // connect timer output to pin
	P2DIR|=SPEAKER;
}

// setup button
void init_button(void){
	P1OUT |= BUTTON; // pullup
	P1REN |= BUTTON; // enable resistor
	P1IES |= BUTTON; // set for 1->0 transition
	P1IFG &= ~BUTTON;// clear interrupt flag
	P1IE  |= BUTTON; // enable interrupt
}

// button interrupt handler
void interrupt button_handler()
{
	if (P1IFG & BUTTON){

		if(modes == 'f') modes = 'x';
		else {
			modes = 'f';
			playmusic = 1;
			music_count = 0;
			TA1CCR0 = tone_array[music_count];
			blink_counter = note_lengths[music_count];
		}

		P1IFG &= ~BUTTON;
		TACCTL0 ^= OUTMOD_4;
		P1OUT ^= LED;
	}
}

void motor_init(void) {
    TACCR0 = MID_PWM;       			// TACCR0 controls the PWM frequency
    TACCR1 = 0;             			// 0.0% duty cycle for PWMTop = 1000
    TACTL = TASSEL_2 + ID_0 + MC_1;		// SMCLK, div 1, Up Mode

    TACCTL1 = OUTMOD_7;     			// Reset/Set: Sets at TACCR0, resets at TACCR1
}

interrupt void WDT_interval_handler(){

	/* modes:
	 * x = stop everything
	 * f = go forward
	 * s = stop going forward
	 * r = turn right
	 * l = turn left
	 * b = turn led on
	 * t = turn led off
	 * p = play tones
	 */

	switch(modes) {
		case 'x': 	// stop everything
			P1SEL &= ~(MOTOR_L + MOTOR_R);	// diable motors
			TA1CCR0 = 0;					// turn off tone
			break;
		case 'f':	// go forward
			P1SEL |= MOTOR_L + MOTOR_R;		// enable both motors
			TA0CCR0 = MID_PWM;				// set PWN
			TA0CCR1 = 100;
			break;
		case 's':							// stop going forward
			P1SEL &= ~(MOTOR_L + MOTOR_R);	// disable motors
			break;
		case 'r':							// turn right
			P1SEL &= ~(MOTOR_R);			// disable right motor
			TA0CCR0 = MID_PWM;				// set PWM
			TA0CCR1 = 100;
			break;
		case 'l':							// turn left
			P1SEL &= ~(MOTOR_L);			// disable left motor
			TA0CCR0 = MID_PWM;				// set PWM
			TA0CCR1 = 100;
			break;
		case 'b':							// turn led on
			P1OUT |= LED;
			break;
		case 't':							// turn led off
			P1OUT &= ~LED;
			break;
		case 'p':	// play tones
			playmusic = 1;
			music_count = 0;
			blink_counter = note_lengths[music_count];
			break;
	}

	if(playmusic)
	{
		if( --blink_counter == 0){
			music_count++;

			if(music_count == 4){
				TA1CCR0 = 0;
				playmusic = 0;
				music_count = 0;
			}
			else
			{
				TA1CCR0 = tone_array[music_count];
				blink_counter = note_lengths[music_count] * 15;
			}
		}
	}
}
/*
// bluetooth reciever
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
	while(!(IFG2 & UCA0RXIFG)); //Make the CPU wait here until the receive...

    if (UCA0RXBUF != 0) // if something was recieved
    {
    	modes = UCA0RXBUF;
    }
}
*/
ISR_VECTOR(button_handler,".int02") // declare interrupt vector
ISR_VECTOR(WDT_interval_handler, ".int10")
