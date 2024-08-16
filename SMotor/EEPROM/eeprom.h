/*
 * eeprom.h
 *
 * Created: 8/16/2024 5:53:47 PM
 *  Author: HTSANG
 */ 


#ifndef EEPROM_H_
#define EEPROM_H_
#include <avr/eeprom.h>

void EEPROM_write(unsigned int uiAddress, unsigned char ucData);
unsigned char EEPROM_read(unsigned int uiAddress);

#endif /* EEPROM_H_ */