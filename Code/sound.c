/* 
sound.c
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

#include "sound.h"
#include "types.h"
#include <avr/io.h>

/* Bit bang a squarewave at a specific frequency and duration */
void playNote( uint16 period_us, uint16 duration_ms )
{
  uint8  u_StartCnt;
  uint8  u_HalfPeriod;

  uint16 w_LoopCount = ((uint32)duration_ms * 1000) / period_us;

  u_HalfPeriod = (uint8)( period_us >> 4 ); // Div 2 for half period. Div 8 for prescaler

  PORTC &= ~_BV(PORTC2);

  while( w_LoopCount-- )
  {
    u_StartCnt = TCNT0;
  	while( (uint8)(TCNT0 - u_StartCnt) < u_HalfPeriod );
    PORTC |= _BV(PORTC2);

  	u_StartCnt = TCNT0;
  	while( (uint8)(TCNT0 - u_StartCnt) < u_HalfPeriod );
    PORTC &= ~_BV(PORTC2);
  }
}

