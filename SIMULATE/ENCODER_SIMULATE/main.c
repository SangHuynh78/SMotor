/*
 * ENCODER_SIMULATE.c
 *
 * Created: 8/14/2024 3:53:45 PM
 * Author : HTSANG
 */ 

#define  F_CPU		8000000UL
#include <avr/io.h>
#include <util/delay.h>

#define A_PIN		PIND7
#define A_DDR		DDRD
#define A_PORT		PORTD
#define	Z_PIN		PIND6
#define Z_DDR		DDRD
#define Z_PORT		PORTD

int main(void)
{	// Output pin
	Z_DDR  |=  (1 << Z_PIN);
	A_DDR  |=  (1 << A_PIN);
    while (1)
    {
		uint8_t i = 0;
		Z_PORT |= (1 << Z_PIN);
		_delay_us(50);
		Z_PORT &= ~(1 << Z_PIN);
		while(i < 100)
		{
			A_PORT |= (1 << A_PIN);
			_delay_us(50);
			A_PORT &= ~(1 << A_PIN);
			_delay_us(50);
			i++;
		}
		//_delay_us(1000);
		//_delay_ms(3000);
		while(i < 200)
		{
			A_PORT |= (1 << A_PIN);
			_delay_us(50);
			A_PORT &= ~(1 << A_PIN);
			_delay_us(50);
			i++;
		}
		_delay_us(100);
    }
}

