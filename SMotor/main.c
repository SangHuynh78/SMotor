/*
 * SMotor.c
 * Created: 8/12/2024 3:17:12 PM
 * Author : HTSANG
 */ 

#define F_CPU		8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "main.h"

step_data_t Step_data_default = {
	L_default,
	S_default,
	Q_default};
	
step_data_t Step_data;
enum state_t {INIT_DATA = 0, REINIT_DATA, WAIT_Z, PULSEA_0, PULSEA_1};
enum state_t state = INIT_DATA;
static uint8_t flag_reinit_data = 0;
uint8_t A_pul_count;
uint8_t S_pul_count;
uint8_t A2S_table[50] = {40,};
uint8_t Z_noise = 0;

//ISR for timer 0 overflow
ISR(TIMER0_OVF_vect) {
	TCCR0B = 0;					//stop timer
	PUL_PORT &= ~(1<<PUL_PIN);	//clear pulse pin
	TCNT0 = 256 - 80;			//reinit counter 0
}

//ISR for timer 1 overflow
ISR(TIMER1_OVF_vect) {
	TCCR1B = 0;					//stop timer
	flag_reinit_data = 1;
	TCNT1 = 0;					//clear counter 1
}

int main(void)
{
	// Output pin
	PUL_DDR  |=  (1<<PUL_PIN);
	PUL_PORT &= ~(1<<PUL_PIN);
	// Input pin
	A_DDR  &= ~(1<<A_PIN);
	Z_DDR  &= ~(1<<Z_PIN);
	A_PORT |= (1<<A_PIN);
	Z_PORT |= (1<<Z_PIN);
	// Init timer 0
	TIMSK0 |= (1 << TOIE0);				// enable overflow timer 0
	TCNT0	= (256 - 80);				// pulse width is 10 us with clock of 8 Mhz
	// Init timer 1
	TIMSK1 |= (1 << TOIE1);				// enable overflow timer 1
	TCNT1 = 0;							// init counter 1
	sei();								// enable interrupt
    while (1)
    {
		switch (state)
		{
			case INIT_DATA:
				Step_data.S = Step_data_default.S;
				Step_data.Q = Step_data_default.Q;
				S_pul_count = 0;
				A_pul_count = 0;
				A2S_table[0] = 40;
				for (int i = 1; i < Step_data.S; i++)
				{
					if (i < 5) A2S_table[i] = A2S_table[i-1] + Step_data.Q + 5 - i;
					else A2S_table[i] = A2S_table[i-1] + Step_data.Q;
				}
				state = WAIT_Z;
				break;
			case REINIT_DATA:
				flag_reinit_data = 0;
				//Step_data.Q = (A_pul - (A_pul_count - 40) - 10) / ((Step_data.S - S_pul_count + 1) - 1);
				Step_data.Q = (A_pul - A_pul_count + 30) / (Step_data.S - S_pul_count);
				A2S_table[S_pul_count] = A_pul_count + 1;
				for (int i = S_pul_count + 1; i < Step_data.S; i++)
				{
					if (i < S_pul_count + 5) A2S_table[i] = A2S_table[i-1] + Step_data.Q + S_pul_count + 5 - i;
					else A2S_table[i] = A2S_table[i-1] + Step_data.Q;
				}
				state = PULSEA_0;
				break;
			case WAIT_Z:
				if (PINC & (1 << Z_PIN)) state = PULSEA_0;
// 				if (PINC & (1 << Z_PIN)) Z_noise ++;
// 				if (Z_noise > 50)
// 				{
// 					state = PULSEA_0;
// 					Z_noise = 0;
// 				}
				break;
			case PULSEA_0:
				if (flag_reinit_data) 
				{
					state = REINIT_DATA;
					break;
				}
				if (PINC & (1 << A_PIN))	
				{
					TCCR1B &= ~(1 << CS10);			// stop timer 1
					A_pul_count++;
					if (A_pul_count == A2S_table[S_pul_count])
					{
						PUL_PORT |= (1 << PUL_PIN);
						TCCR0B |= (1 << CS00);		// start timer 0
						S_pul_count ++;
					};
					if (S_pul_count < Step_data.S)  state = PULSEA_1;	
					else state = INIT_DATA;
				}
				else TCCR1B |=  (1 << CS12);				//start timer 1 with prescaler=256, overflow in 2.09s
				//else TCCR1B |=  (1 << CS11) | (1 << CS10);  //start timer 1 with prescaler=64, overflow in 0.52s
				break;
			case PULSEA_1:
			//test
			//	if (A_pul_count == 100) state = REINIT_DATA;
			//test
				if ((PINC & (1 << A_PIN)) == 0) state = PULSEA_0;
				break;
			default:
				break; 
		}
	}
}
// uint8_t table_38[38] = {40,46,51,55,58,61,64,67,70,73,76,79,82,85,88,91,94,97,100,103,106,109,112,115,118,121,124,127,130,133,136,139,142,145,148,151,155,160};
// uint8_t table[] = {/*40, Step_data.Q + 44, 2*Step_data.Q + 47, 3*Step_data.Q + 49, 4*Step_data.Q + 50,*/40,47,53,58,62,65,68,71,74,77,80,83,86,89,92,95,98,101,104,107,110,113,116,119,122,125,128,131,134,137,140,143,146,149,152,155,158,161};