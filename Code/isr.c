/* 
isr.c
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

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>
#include "isr.h"
#include "types.h"
#include "common.h"
#include "ina219.h"

#define C_LONG_PRESS_THRESHOLD_MS 1000

static volatile uint8 u_ISRFlags = 0;

/* Returns current ISR flag values */
uint8 ISRGetFlags( void )
{
  uint8 u_ISRFlagsTemp;

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    u_ISRFlagsTemp = u_ISRFlags;
    u_ISRFlags = 0;
  }

  return ( u_ISRFlagsTemp );
}

/* Returns raw pushbutton GPIO reading */
static uint8 GetRawButtonPress( void )
{
  return( ( PINB & _BV(PORTB4) ) > 0 );
}

/* Returns raw encoder GPIO readings. Format is 000000AB */
static uint8 GetRawEncoderValue( void )
{
  return( ( ( PINB & _BV(PORTB1) ? 1:0 ) << 1 ) | 
          ( ( PINB & _BV(PORTB3) ? 1:0 ) ) );
}

/* 1Hz Timer Interrupt */
ISR ( TIMER2_COMPA_vect )
{
  u_ISRFlags |= C_ISR_FLAG_1HZ_TICK;

  /* Clear interrupt */
  TIFR2 |= _BV(OCF2A);
}

/* 1kHz Timer 1 Interrupt */
ISR ( TIMER1_COMPA_vect )
{
  static uint16  w_MsCounter = 0;
  static uint16  w_SwitchState = 0;
  static uint16  w_LongSwitchPressCounter = 0;
  static uint8   u_LastEncoderValue = 0x3; 
  uint8          u_CurrentEncoderValue;

  /* Debounce Pushbutton */
  if( w_MsCounter & 0x1 )
  {
    w_SwitchState =   (w_SwitchState << 1 ) | GetRawButtonPress() | 0xe000;

    if( w_SwitchState == 0xf000 )
    {
      u_ISRFlags |= C_ISR_FLAG_SHORT_BUTTON_PRESS;
    }
    else
    if( ( w_SwitchState == 0xe000 ) && ( w_LongSwitchPressCounter != C_LONG_PRESS_THRESHOLD_MS ) )
    {
      if( ++w_LongSwitchPressCounter == C_LONG_PRESS_THRESHOLD_MS )
      {
        u_ISRFlags |= C_ISR_FLAG_LONG_BUTTON_PRESS;
      }
    }
    else
    {
      w_LongSwitchPressCounter = 0;
    }
  }

  /* Read the state of the encoder */
  u_CurrentEncoderValue = GetRawEncoderValue();

  if ( u_CurrentEncoderValue != u_LastEncoderValue )
  {
    if( ( u_CurrentEncoderValue & 0x2 ) && !( u_LastEncoderValue & 0x2 ) )
    {
      /* Low to High Transition on A. Rotation direction depends on current state of B */
      if( u_CurrentEncoderValue & 0x1 )
      {
        /* CCW */
        u_ISRFlags |= C_ISR_FLAG_ENCODER_CCW;
      }
      else
      {
        /* CW */
        u_ISRFlags |= C_ISR_FLAG_ENCODER_CW;
      }
    }
  }

  u_LastEncoderValue = u_CurrentEncoderValue;

  if( ++w_MsCounter == 1000 )
  {
    w_MsCounter = 0;
  }

  TIFR1 |= _BV(OCF1A);
}

