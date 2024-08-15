/*
 * main.h
 *
 * Created: 8/12/2024 3:17:42 PM
 *  Author: HTSANG
 */ 


#ifndef MAIN_H_
#define MAIN_H_

#define F_CPU		8000000UL
#define PUL_PIN		PIND7
#define PUL_DDR		DDRD
#define PUL_PORT	PORTD

#define DI1_PIN		PINC2
#define DI1_DDR		DDRC
#define DI1_PORT	PORTC

#define A_PIN		PINC5
#define A_DDR		DDRC
#define A_PORT		PORTC

#define	Z_PIN		PINC3
#define Z_DDR		DDRC
#define Z_PORT		PORTC

#define A_pul		120
#define L_default	3.2
#define S_default	37		//=(4000*L_default)/343
#define Q_default	3		//=(A_Pul-10)/(S_default-1)

typedef struct {
	float L;
	uint8_t S;
	uint8_t Q;
} step_data_t;


void adc_init(void);
uint16_t adc_read(void);
float read_L(void);

#endif /* MAIN_H_ */