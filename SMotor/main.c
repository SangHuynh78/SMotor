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

step_data_t Step_data;
enum state_t {WAIT_Z = 0, PULSEA_0, PULSEA_1, WAIT_A, REINIT};
enum state_t state = WAIT_Z;
enum state_t pre_state = WAIT_Z;  

static uint8_t flag_wait_A = 0;
uint8_t A_pul_count;
uint8_t S_pul_count;
uint8_t A2S_table[60] = {40,};
uint8_t A2S_table_wait_A[60];
uint8_t *p_A2S_table;
uint8_t remain = 0;
uint8_t DI1_noise = 0;
uint8_t Z_noise = 0;
uint8_t A_noise = 0;

//ISR for timer 0 overflow
ISR(TIMER0_OVF_vect) {
	TCCR0B = 0;					// stop timer
	PUL_PORT &= ~(1<<PUL_PIN);	// clear pulse pin
	TCNT0 = 256 - 80;			// reinit counter 0
}

//ISR for timer 1 overflow
ISR(TIMER1_OVF_vect) {
	TCCR1B = 0;					// stop timer
	TCNT1 = 65536 - 15625;				// REinit counter 1 with 0.5s
	flag_wait_A = 1;
}
		
int main(void)
{
	// Init data
	Step_data.L = L_default;
	Step_data.S = S_default;
	Step_data.Q = Q_default;
	A2S_table[0] = 40;
	for (int i = 1; i < Step_data.S; i++)
	{
		if (i < 5) A2S_table[i] = A2S_table[i-1] + Step_data.Q + 5 - i;
		else A2S_table[i] = A2S_table[i-1] + Step_data.Q;
	}
	remain = (uint8_t)(160-A2S_table[Step_data.S -1]);
	for (int i = 4; i < Step_data.S; i++)
	{
		if (i < 4 + remain) A2S_table[i] = A2S_table[i-1] + Step_data.Q + 1;
		else A2S_table[i] = A2S_table[i-1] + Step_data.Q;
	}
	p_A2S_table = A2S_table;
	// Output pin
	PUL_DDR  |=  (1<<PUL_PIN);
	PUL_PORT &= ~(1<<PUL_PIN);
	// Input pin
	DI1_DDR&= ~(1<<DI1_PIN);
	A_DDR  &= ~(1<<A_PIN);
	Z_DDR  &= ~(1<<Z_PIN);
	//DI1_PORT |= (1<<DI1_PIN);
	A_PORT   |= (1<<A_PIN);
	Z_PORT	 |= (1<<Z_PIN);
	// ADC // UART
	adc_init();
	// Init timer 0
	TIMSK0 |= (1 << TOIE0);				// enable overflow timer 0
	TCNT0	= (256 - 80);				// pulse width is 10 us with clock of 8 Mhz
	// Init timer 1
	TIMSK1 |= (1 << TOIE1);				// enable overflow timer 1
	TCNT1 = 65536 - 15625;				// init counter 1 with 0.5s
	sei();								// enable interrupt
    while (1)
    {
		switch (state)
		{
 			case WAIT_Z:
				// Kien tra trang thai nut DI1
				if (PINC & (1 << DI1_PIN))
				{
					if (++ DI1_noise > 50)
					{
						state = REINIT;
						DI1_noise = 0;
						break;
					}
				}
				else DI1_noise = 0;
				// Kiem tra xung Z
				if (PINC & (1 << Z_PIN))
				{
					if (++ Z_noise > 50)
					{
						S_pul_count = 0;
						A_pul_count = 0;
						p_A2S_table = A2S_table;
						state = PULSEA_0;
						Z_noise	= 0;
						break;
					}
				}
				else Z_noise = 0;
 				break;
			case PULSEA_0:
				// Kien tra trang thai nut DI1
				if (PINC & (1 << DI1_PIN))
				{
					if (++ DI1_noise > 50)
					{
						state = REINIT;
						DI1_noise = 0;
						break;
					}
				}
				else DI1_noise = 0;
				// Kiem tra co wait_A
				if (flag_wait_A)
				{
					TCCR1B = 0;	// stop timeout timer
					pre_state = PULSEA_0;
					state = WAIT_A;
					break;
				}
				if (PINC & (1 << A_PIN))
				{
					A_noise ++;
				}	
				else
				{
					A_noise = 0;
					break;
				}
				
				if (A_noise >= 5)						// pulse = 1 in 5 continous read
				{
					TCCR1B = 0;							// stop timer 1
					TCNT1 = 65536 - 15625;				// init counter 1 with 0.5s
					if (S_pul_count) TCCR1B |=  (1 << CS11) | (1 << CS10);		// restart timer 1
					A_pul_count++;
					if (A_pul_count == *(p_A2S_table + S_pul_count))
					{
						PUL_PORT |= (1 << PUL_PIN);
						TCCR0B |= (1 << CS00);		// start timer 0
						S_pul_count ++;
					};
					if (S_pul_count < Step_data.S)  state = PULSEA_1;
					else 
					{
						state = WAIT_Z;
						TCCR1B = 0;		// stop pulse A time out timer
						flag_wait_A = 0;
					}
					A_noise = 0;
				}
				break;
			case PULSEA_1:
				// Kien tra trang thai nut DI1
				if (PINC & (1 << DI1_PIN))
				{
					if (++ DI1_noise > 50)
					{
						state = REINIT;
						DI1_noise = 0;
						break;
					}
				}
				else DI1_noise = 0;
				// Kiem tra co wait_A
				if (flag_wait_A)
				{	
					TCCR1B = 0;	// stop timeout timer
					flag_wait_A = 0;
					pre_state = PULSEA_1;
					state = WAIT_A;
					break;
				}
				if ((PINC & (1 << A_PIN)) == 0) 
				{
					A_noise ++;
				}
				else 
				{
					A_noise = 0;
					break;
				}
				if (A_noise >= 5)	// pulse = 0 in 5 consecutive read
				{
					//restart A pulse timeout timer
					TCCR1B = 0;										// stop timer 1
					TCNT1 = 65536 - 15625;				// init counter 1 with 0.5s
					if (S_pul_count) TCCR1B |=  (1 << CS11) | (1 << CS10);		// restart timer 1
					state = PULSEA_0;
				}
				break;
			case WAIT_A:
				// Kien tra trang thai nut DI1
				if (PINC & (1 << DI1_PIN))
				{
					if (++ DI1_noise > 50)
					{
						state = REINIT;
						DI1_noise = 0;
						break;
					}
				}
				else DI1_noise = 0;
				flag_wait_A = 0;
				//Step_data.Q = (A_pul - (A_pul_count - 40) - 10) / ((Step_data.S - S_pul_count + 1) - 1);
				Step_data.Q = (A_pul - A_pul_count + 30) / (Step_data.S - S_pul_count);
				A2S_table_wait_A[S_pul_count] = A_pul_count + 1;
				for (int i = S_pul_count + 1; i < Step_data.S; i++)
				{
					if (i < S_pul_count + 5) A2S_table_wait_A[i] = A2S_table_wait_A[i-1] + Step_data.Q + S_pul_count + 5 - i;
					else A2S_table_wait_A[i] = A2S_table_wait_A[i-1] + Step_data.Q;
				}
				remain = (uint8_t)(160-A2S_table[Step_data.S -1]);
				for (int i = S_pul_count + 4; i < Step_data.S; i++)
				{
					if (i < S_pul_count + 4 + remain) A2S_table[i] = A2S_table[i-1] + Step_data.Q + 1;
					else A2S_table[i] = A2S_table[i-1] + Step_data.Q;
				}
				p_A2S_table = A2S_table_wait_A;
				state = pre_state;
				break;
			case REINIT:
				if (!(PINC & (1 << DI1_PIN)))
				{
					if (++ DI1_noise > 50)
					{
						state = WAIT_Z;
						DI1_noise = 0;
						break;
					}
				}
				else DI1_noise = 0;				
				Step_data.L = (float)read_L();
				Step_data.S = (4000*Step_data.L)/343;
				Step_data.Q = (A_pul - 10)/(Step_data.S - 1);
				A2S_table[0] = 40;
				for (int i = 1; i < Step_data.S; i++)
				{
					if (i < 5) A2S_table[i] = A2S_table[i-1] + Step_data.Q + 5 - i;
					else A2S_table[i] = A2S_table[i-1] + Step_data.Q;
				}
				remain = (uint8_t)(160-A2S_table[Step_data.S -1]);
				for (int i = 4; i < Step_data.S; i++)
				{
					if (i < 4 + remain) A2S_table[i] = A2S_table[i-1] + Step_data.Q + 1;
					else A2S_table[i] = A2S_table[i-1] + Step_data.Q;
				}
				break;
			default:
				break; 
		}
	}
}

void adc_init(void)
{
	ADMUX |= (1 << REFS0) | (0 << REFS1);
	ADMUX &= ~(1 << MUX0); // Ch?n kênh ADC0 (PC0)
	ADCSRA |= (1 << ADEN); // B?t ADC
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Prescaler = 128
}

uint16_t adc_read(void) {
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return (ADCL | (ADCH << 8));
}

float read_L(void)
{
	uint16_t adcVal = adc_read();
	return (float)(adcVal*3/682 + 0.5f);
}