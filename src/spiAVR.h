#ifndef SPIAVR_H
#define SPIAVR_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// Outputs, pin definitions
#define PIN_SCK                   PORTB5    //Port 13 on UNO
#define PIN_MOSI                  PORTB3    //Port 11 on UNO
#define PIN_SS                    PORTB2    //Port 10 on UNO

/* SPI Utility functions*/

void SPI_INIT(){
    DDRB |= (1 << PIN_SCK) | (1 << PIN_MOSI) | (1 << PIN_SS);   //initialize pins
    SPCR |= (1 << SPE) | (1 << MSTR);                           //initialize SPI comunication
}


void SPI_SEND(char data)
{
    SPDR = data;                    //set data that you want to transmit (8-bits only)
    while (!(SPSR & (1 << SPIF)));  //wait until done transmitting
}

#endif /* SPIAVR_H */