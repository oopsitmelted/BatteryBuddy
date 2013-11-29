/* 
batterybuddy.c
Copyright (C) 2010 Scott Stickeler

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA. 
*/

#define extern
#include "common.h"
#undef extern

#include <stdio.h>
#include <util/delay.h>
#include <avr/sfr_defs.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "isr.h"
#include "state.h"
#include "types.h"
#include "lcd.h"
#include "ina219.h"
#include "sound.h"

/* Initialize AVR peripherals */
static void init_hw( void )
{
  /* LCD Power */
  PORTC &= ~_BV(PORTC1);
  DDRC |= _BV(DDC1);

  /* OpAmp Power */
  PORTB &= ~_BV(PORTB5);
  DDRB |= _BV(DDB5);

  /* Pushbutton */
  DDRB &= ~_BV(DDB4);
  PORTB |= _BV(PORTB4); // Enable Pullup

  /* Encoder */
  DDRB &= ~( _BV(DDB1) | _BV(DDB3) );
  PORTB |= _BV(PORTB1) | _BV(PORTB3); // Enable Pullups

  /* PWM Output */
  PORTB &= ~_BV(PORTB2);
  DDRB |= _BV(DDB2);

  /* LED */
  PORTC &= ~_BV(PORTC3);
  DDRC |= _BV(DDC3);

  /* Speaker */
  PORTC &= ~_BV(PORTC2);
  DDRC |= _BV(DDC2);

  /* Timer 0 - 125kHz Frequency */ 
  TCCR0B = _BV(CS01); // Clk Div 8

  /* Timer 2 - 32.768kHz External Crystal */
  TCCR2A = _BV(WGM21); // CTC Mode
  OCR2A = 31; // Rollover at every 32 clocks
  TCCR2B = _BV(CS20) | _BV(CS21) | _BV(CS22); // Div 1024 Prescaler = 32Hz
  TIMSK2 = _BV(OCIE2A); // Interrupt on compare match
  ASSR = _BV(AS2); // Enable crystal oscillator

  /* Timer 1 - 1MHz Frequency */
  TCCR1A = _BV(COM1B1) | _BV(WGM10) | 
           _BV(WGM11);   // Clear on compare match, Fast PWM Mode, TOP=OCR1A, OC1B output

  TCCR1B = _BV(CS10) | _BV(WGM12) | _BV(WGM13); // Clk div 1, CTC Mode - TOP=OCR 

  OCR1A = 1000;                    // 1kHz compare match rate
  OCR1B = 0;                       // 0% duty cycle initially
  TIMSK1 = _BV(OCIE1A);            // Enable compare match interrupt
}

int main( void )
{
  uint8 u_Flags;

  /* Set system clock to 1MHz ( 8MHz div 8 ) */
  CLKPR = _BV(CLKPCE);
  CLKPR = _BV(CLKPS1) | _BV(CLKPS0);

  /* Initialize Peripherals */
  init_hw();

  /* initialize display, cursor off */
  lcd_init(LCD_DISP_ON);

  /* clear display and home cursor */
  lcd_clrscr();
        
  /* put string to display (line 1) with linefeed */
  lcd_puts(" Battery Buddy\n");

  /* cursor is now on second line, write second line */
  lcd_puts("      v1.0");
  
  /* Play opening ditty */
  playNote( 3033, 100 );
  playNote( 2551, 100 );
  playNote( 1911, 100 );

  _delay_ms(2000);

  lcd_clrscr();

  /* Initialize current monitor */
  ina219_init();

  _delay_ms(2);

  /* Enable Interrupts */
  sei();

  /* Event Loop */
  while( 1 )
  {
    if( ( u_Flags = ISRGetFlags() ) == 0 )
      continue;

    /* Main State Machine */
    StateProcessFlags( u_Flags );
  }

  return 0;
}



