/* 
state.c
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

#include "common.h"
#include <util/delay.h>
#include <util/atomic.h>
#include <string.h>
#include <avr/interrupt.h>
#include "isr.h"
#include "state.h"
#include "types.h"
#include "config.h"
#include "lcd.h"
#include "ina219.h"
#include "sound.h"

#define MIN_CELLS_NIMH 4
#define MIN_CELLS_LIPO 1
#define MAX_CELLS 6
#define CELL_CUTOFF_NIMH_FULL_DISCHARGE 900  /* mV */
#define CELL_CUTOFF_LIPO_FULL_DISCHARGE 3000 /* mV */
#define CELL_CUTOFF_NIMH_STORAGE        900  /* mV */ 
#define CELL_CUTOFF_LIPO_STORAGE        3800 /* mV */       
#define CUSTOM_CURRENT_MAX              1000 /* mA */
#define CUSTOM_CURRENT_INCREMENT        10

static enum
{
  STATE_INIT,
  STATE_CONFIG_SET_MODE,
  STATE_CONFIG_SET_TYPE,
  STATE_CONFIG_SET_NUM_CELLS,
  STATE_CONFIG_SET_CURRENT,
  STATE_CONFIG_SET_CURRENT_CUSTOM,
  STATE_WAIT_BATTERY,
  STATE_DISCHARGE,
  STATE_FINISHED
} e_State = STATE_INIT;

static const char *p_ModeStrings[] = {
  "Full Discharge  ", 
  "Storage         " };

static const char *p_CellTypeStrings[] = {
  "NIMH            ", 
  "LiPo            " };

static const char *p_DischargeCurrentStrings[] = { 
  "50mA            ", 
  "100mA           ",
  "500mA           ", 
  "1000mA          ",
  "Custom          " };

static const uint16 w_DischargeCurrentLookup[] = { 50, 100, 500, 1000 };

/* Routine to disable OpAmp to save power */
static void StateOpAmpPowerOn( void )
{
  PORTB |= _BV(PORTB5);
  _delay_ms(100);
}

/* Routine to enable OpAmp */
static void StateOpAmpPowerOff( void )
{
  PORTB &= ~_BV(PORTB5);
}

/* Display a multi-digit number on the LCD
 * w_Number - Number to display
 * u_FieldSize - Size of the field to be displayed, elements of the field not containing
 *   numerical digits will be filled with u_FillChar
 * u_DigitsAfterDecimal - Number of digits to after decimal point. If '0' then no decimal
 *   point displayed
 * u_FillChar - Character used to fill empty field positions
 */
static void StateDisplayNumber( uint16 w_Number, uint8 u_FieldSize, 
                         uint8 u_DigitsAfterDecimal, char u_FillChar )
{
  uint16 w_Temp;
  uint16 w_Divisor = 1;
  uint8 u_Count;
  uint8 u_DecimalPlaceIndex = 0;
  uint8 u_NonZeroDigitFound = FALSE;

  if( u_FieldSize > 8 || u_DigitsAfterDecimal > u_FieldSize - 2 )
    return;

  if( u_DigitsAfterDecimal )
  {
    u_DecimalPlaceIndex = u_FieldSize - u_DigitsAfterDecimal - 1;
  }

  for( u_Count = 1; u_Count < u_FieldSize; u_Count++ )
  {
    w_Divisor *= 10;
  }

  for( u_Count = 0; u_Count < u_FieldSize; u_Count++ )  
  {
    if( u_DigitsAfterDecimal && ( u_Count == u_DecimalPlaceIndex ) )
    {
      lcd_putc('.');
      u_NonZeroDigitFound = TRUE;
    }
    else
    {
      w_Temp = w_Number / w_Divisor;

      if( w_Temp )
      {
        lcd_putc( (uint8)w_Temp + 48 );
        w_Number -= w_Temp * w_Divisor;
        u_NonZeroDigitFound = TRUE;
      }
      else
      {
        if( !u_NonZeroDigitFound )
        {
          if( u_Count + 1 == u_DecimalPlaceIndex )
          {
            lcd_putc('0');
          }
          else
          {
            if( u_Count == u_FieldSize - 1 )
              lcd_putc('0');
            else
              lcd_putc( u_FillChar );
          }
        }
        else
        {
          lcd_putc('0');
        }
      }

      w_Divisor /= 10;
    }
  }
}

/* Display a formatted time on the LCD */
static void StateDispTime( uint8 u_Hours, uint8 u_Minutes, uint8 u_Seconds )
{
  /* Hours */
  StateDisplayNumber( u_Hours, 2, 0, '0' );
  lcd_puts(" : ");

  /* Minutes */
  StateDisplayNumber( u_Minutes, 2, 0, '0' );
  lcd_puts(" : ");

  /* Seconds */
  StateDisplayNumber( u_Seconds, 2, 0, '0' );
}

/* Handle transition to config state */
static void StateEnterConfig( void )
{
  /* Disable PWM */
  OCR1B = 0;

  /* Clear Status */
  memset( &z_Status, 0, sizeof( z_Status ) );
        
  lcd_clrscr();
  lcd_puts("Mode:\n");
  lcd_puts( p_ModeStrings[z_Config.e_Mode] );
  e_State = STATE_CONFIG_SET_MODE;
}

/* Handle transition to discharge state */
static void StateEnterDischarge( void )
{
  uint32 q_Temp;

  /* Store config parameters to EEPROM */
  ConfigWriteEEPROM( &z_Config );
  
  lcd_clrscr();

  if( z_Config.e_DischargeCurrent == DISCHARGE_CURRENT_CUSTOM )
    z_Status.w_DischargeCurrent = z_Config.w_DischargeCurrentCustom;
  else
    z_Status.w_DischargeCurrent = w_DischargeCurrentLookup[z_Config.e_DischargeCurrent];

  if( z_Config.e_CellType == CELL_TYPE_NIMH )
  {
    /* NIMH */
    if( z_Config.e_Mode == MODE_FULL_DISCHARGE )
    {
      /* Full Discharge */
      z_Status.w_CutoffVoltage = CELL_CUTOFF_NIMH_FULL_DISCHARGE;
    }
    else
    {
      /* Storage */
      z_Status.w_CutoffVoltage = CELL_CUTOFF_NIMH_STORAGE;
    }
  }
  else
  {
    /* LIPO */
    if( z_Config.e_Mode == MODE_FULL_DISCHARGE )
    {
      /* Full Discharge */
      z_Status.w_CutoffVoltage = CELL_CUTOFF_LIPO_FULL_DISCHARGE;
    }
    else
    {
      /* Storage */
      z_Status.w_CutoffVoltage = CELL_CUTOFF_LIPO_STORAGE;
    }
  }

  z_Status.w_CutoffVoltage *= z_Config.u_NumCells;

  /* Turn on OpAmp */
  StateOpAmpPowerOn();

  /* Set PWM to approximate current discharge. Will be fine tuned at 1Hz rate */
  q_Temp = ( ( ( uint32 ) z_Status.w_DischargeCurrent ) * 1000 ) / 1094;
  OCR1B = (uint16) q_Temp;

  e_State = STATE_DISCHARGE;
}

/* Process ISR flags */
void StateProcessFlags( uint8 u_Flags )
{
  switch ( e_State )
  {
    case STATE_INIT:
    {
      if( !ConfigReadEEPROM( &z_Config ) )
      {
        /* EEPROM empty or corrupted. Use default values */
        z_Config.e_Mode = MODE_FULL_DISCHARGE;
        z_Config.e_CellType = CELL_TYPE_NIMH;
        z_Config.u_NumCells = MIN_CELLS_LIPO;
        z_Config.e_DischargeCurrent = DISCHARGE_CURRENT_50MA;
	      z_Config.w_DischargeCurrentCustom = 0;
      }
	 
      StateEnterConfig();

      break;
    }

    case STATE_CONFIG_SET_MODE:
    {
      /* Mode Config */
      if( u_Flags & C_ISR_FLAG_SHORT_BUTTON_PRESS )
      {
        /* Go to cell type config state */
        lcd_clrscr();
        lcd_puts("Type:\n");
        lcd_puts( p_CellTypeStrings[z_Config.e_CellType] );
        e_State = STATE_CONFIG_SET_TYPE;
      }
      else
        ConfigParameter( u_Flags, &z_Config.e_Mode, 0, MODE_MAX - 1, p_ModeStrings );

      break;
    }

    case STATE_CONFIG_SET_TYPE:
    {
      /* Cell type config */
      if( u_Flags & C_ISR_FLAG_SHORT_BUTTON_PRESS )
      {
        /* Go to num cells config state */
        lcd_clrscr();
        lcd_puts("Num Cells:\n");
        lcd_putc( z_Config.u_NumCells + 48 );

        if( z_Config.e_CellType == CELL_TYPE_NIMH )
          z_Config.u_NumCells = MIN_CELLS_NIMH;

        e_State = STATE_CONFIG_SET_NUM_CELLS;
      }
      else
        ConfigParameter( u_Flags, &z_Config.e_CellType, 0, CELL_TYPE_MAX - 1, p_CellTypeStrings );
        
      break;
    }

    case STATE_CONFIG_SET_NUM_CELLS:
    {
      /* Num cells config */
      if( u_Flags & C_ISR_FLAG_SHORT_BUTTON_PRESS )
      {
        /* Go to set current state */
        lcd_clrscr();
        lcd_puts("Current:\n");
        lcd_puts( p_DischargeCurrentStrings[ z_Config.e_DischargeCurrent ] );
        e_State = STATE_CONFIG_SET_CURRENT;
      }
      else
      {
        if( z_Config.e_CellType == CELL_TYPE_LIPO )
        {
          ConfigParameter( u_Flags, &z_Config.u_NumCells, MIN_CELLS_LIPO, MAX_CELLS, NULL );
        }
        else
        {
          ConfigParameter( u_Flags, &z_Config.u_NumCells, MIN_CELLS_NIMH, MAX_CELLS, NULL );
        }
      }
 
      break;
    }

    case STATE_CONFIG_SET_CURRENT:
    {
      /* Current config */
      if( u_Flags & C_ISR_FLAG_SHORT_BUTTON_PRESS )
      {
        if( z_Config.e_DischargeCurrent == DISCHARGE_CURRENT_CUSTOM )
        {
          /* Go to custom current config state */
          lcd_clrscr();
          lcd_puts("Current:\n");
          StateDisplayNumber( z_Config.w_DischargeCurrentCustom, 4, 0, ' ');
          lcd_puts(" mA");
          e_State = STATE_CONFIG_SET_CURRENT_CUSTOM;
        }
        else
          e_State = STATE_WAIT_BATTERY;
      }
      else
        ConfigParameter( u_Flags, &z_Config.e_DischargeCurrent, 0, 
            DISCHARGE_CURRENT_MAX - 1, p_DischargeCurrentStrings );

      break;
    }

    case STATE_CONFIG_SET_CURRENT_CUSTOM:
    { 
      /* Custom current config */
      if( u_Flags & C_ISR_FLAG_ENCODER_CW )
      {
        if( z_Config.w_DischargeCurrentCustom < CUSTOM_CURRENT_MAX )
        {
          z_Config.w_DischargeCurrentCustom += CUSTOM_CURRENT_INCREMENT;
          lcd_gotoxy(0,1);
          StateDisplayNumber( z_Config.w_DischargeCurrentCustom, 4, 0, ' ' );
        }
      }

      if( u_Flags & C_ISR_FLAG_ENCODER_CCW )
      {
        if( z_Config.w_DischargeCurrentCustom > 0 )        
        {
          z_Config.w_DischargeCurrentCustom -= CUSTOM_CURRENT_INCREMENT;
          lcd_gotoxy(0,1);
          StateDisplayNumber( z_Config.w_DischargeCurrentCustom, 4, 0, ' ' );
        } 
      }

      if( u_Flags & C_ISR_FLAG_SHORT_BUTTON_PRESS )
      {
        e_State = STATE_WAIT_BATTERY;
      }

      break;
    }

    case STATE_WAIT_BATTERY:
    {
      /* Wait for battery to be connected */
      if( u_Flags & C_ISR_FLAG_1HZ_TICK )
      {
        uint16 w_Voltage = ina219_read_voltage();

        if( z_Config.e_CellType == CELL_TYPE_LIPO )
        {
          if( w_Voltage > z_Config.u_NumCells * CELL_CUTOFF_LIPO_FULL_DISCHARGE )
          {
            StateEnterDischarge();
            break;
          }
        }
        else
        {
          if( w_Voltage > z_Config.u_NumCells * CELL_CUTOFF_NIMH_FULL_DISCHARGE )
          {
            StateEnterDischarge();
            break;
          }
        }

        lcd_clrscr();
        lcd_puts("Insert Battery");
      }

      break;
    }

    case STATE_DISCHARGE:
    {
      if( u_Flags & C_ISR_FLAG_1HZ_TICK )
       {
        uint16 w_ADCCurrent;
        uint16 w_ADCBattery;

        /* Read load current and battery voltage */
        w_ADCCurrent = ina219_read_current();         /* mA */
        w_ADCBattery = ina219_read_voltage();         /* mV */

        /* Toggle LED */
        PORTC ^= _BV(PORTC3);
        
        /* Update mAh accumulator ( in mA seconds ) */
        z_Status.q_CapacityDischarged += w_ADCCurrent;

        /* Adjust PWM to match specified load current */
        if( w_ADCCurrent > z_Status.w_DischargeCurrent )
        { 
          if( OCR1B != 0 )
          {
            OCR1B--;
          }
        }
        else
        if( w_ADCCurrent < z_Status.w_DischargeCurrent )
        {
          if( OCR1B != 0xFFFF )
          {
            OCR1B++;
          }
        } 

        /* V(mv) */
        lcd_gotoxy(0,0);
        StateDisplayNumber( w_ADCBattery, 5, 2, ' ' );
        lcd_puts( "V  " );

        /* mAh */
        StateDisplayNumber( z_Status.q_CapacityDischarged / 3600, 4, 0, ' ' );
        lcd_puts( " mAh\n" );

        /* Time Elapsed */
        if( ++z_Status.u_Seconds == 60 )
        {
          z_Status.u_Seconds = 0;
          z_Status.u_Minutes++;
        }

        if( z_Status.u_Minutes == 60 )
        {
          z_Status.u_Minutes = 0;
          z_Status.u_Hours++;
        }

        lcd_puts( "  " );
        StateDispTime( z_Status.u_Hours, z_Status.u_Minutes, z_Status.u_Seconds );

        /* Stop discharge if cutoff voltage has been reached */
        if( w_ADCBattery < z_Status.w_CutoffVoltage )
        {
          e_State = STATE_FINISHED;
        }
      }

      if( u_Flags & C_ISR_FLAG_LONG_BUTTON_PRESS )
      {
        StateEnterConfig();
      }

      break;
    }

    case STATE_FINISHED:
    {
      uint8 u_Cnt = 0;

      /* Disable Interrupts */
      cli();

      /* Turn off load */
      OCR1B = 0;
      _delay_ms(500);
      StateOpAmpPowerOff();

      /* Turn off LED */
      PORTC &= ~_BV(PORTC3);

      /* Display time elapsed and mAh discharged */
      lcd_clrscr();

      /* mAh */
      StateDisplayNumber( z_Status.q_CapacityDischarged / 3600, 4, 0, ' ' );
      lcd_puts( " mAh Disch\n" );

      /* Time */
      lcd_gotoxy(0,1);
      lcd_puts( "  " );
      StateDispTime( z_Status.u_Hours, z_Status.u_Minutes, z_Status.u_Seconds );

      /* Play some tones */
      for( u_Cnt = 0; u_Cnt < 5; u_Cnt++ )
      {
        playNote( 3033, 100 );
        playNote( 2551, 100 );
        playNote( 1911, 100 );

        _delay_ms(1500);
      }
      break;
    }

    default:
    {
      break;
    }
  }
}
