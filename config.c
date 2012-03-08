/* 
config.c
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
#include <avr/eeprom.h>
#include "types.h"
#include "isr.h"
#include "lcd.h"

#define extern
#include "config.h"
#undef extern

/* Write configuration parameters to EEPROM 
 *   Returns TRUE if success, FALSE if failure
 */
uint8 ConfigWriteEEPROM( z_ConfigStructType *p_Config )
{
  uint16 w_Checksum = 0;
  uint8 *p_Current = (uint8 *)p_Config;
  uint8 *p_End = (uint8 *)p_Config + ( sizeof( z_ConfigStructType ) - 1 );

  /* Compute checksum */
  while( p_Current <= p_End )
  {
    w_Checksum ^= *p_Current++;
  }

  /* Write structure to EEPROM */
  eeprom_write_block( p_Config, (uint8 *) 0, sizeof( z_ConfigStructType ) );

  /* Write Checksum */
  eeprom_write_block( &w_Checksum, (uint8 *)sizeof( z_ConfigStructType ), 
                      sizeof( w_Checksum ) );

  return( TRUE );
}

/* Attempt to read configuration from EEPROM
 *   Returns TRUE if success, FALSE if failure
 */
uint8 ConfigReadEEPROM( z_ConfigStructType *p_Config )
{
  z_ConfigStructType z_ReadStruct;

  uint16 w_ChecksumRead;
  uint16 w_ChecksumComputed = 0;

  uint8 *p_Current = (uint8 *)&z_ReadStruct;
  uint8 *p_End = (uint8 *)&z_ReadStruct + ( sizeof( z_ConfigStructType ) - 1 );

  /* Read structure from EEPROM */
  eeprom_read_block( &z_ReadStruct, (uint8 *) 0, sizeof( z_ConfigStructType ) );

  /* Read checksum */
  eeprom_read_block( &w_ChecksumRead, (uint8 *)sizeof( z_ConfigStructType ), sizeof( w_ChecksumRead ) );

  /* Compute Checksum */
  while( p_Current <= p_End )
  {
    w_ChecksumComputed ^= *p_Current++;
  }

  if( w_ChecksumComputed == w_ChecksumRead )
  {
    *p_Config = z_ReadStruct;
    return( TRUE );
  }
  else
  {
    return( FALSE );
  }
}

/* Utility function to handle configuration parameter selection via encoder 
 *   u_Flags - ISR flags
 *   p_Param - Pointer to configuration parameter
 *   u_ParamMin - Parameter minimum value
 *   u_ParamMax - Parameter maximum value
 *   p_String - Pointer to array of strings to be displayed for each value. If NULL display *p_Param instead
 */
void ConfigParameter( uint8 u_Flags, uint8 *p_Param, uint8 u_ParamMin, 
                      uint8 u_ParamMax, char const **p_String )
{
  if( !( ( u_Flags & C_ISR_FLAG_ENCODER_CW ) | ( u_Flags & C_ISR_FLAG_ENCODER_CCW ) ) )
    return;

  if( u_Flags & C_ISR_FLAG_ENCODER_CW )
  {
    if( ++(*p_Param) == u_ParamMax + 1 )
      *p_Param = u_ParamMin;
  }
  else
  if( u_Flags & C_ISR_FLAG_ENCODER_CCW )
  {
    if( *p_Param == u_ParamMin )
      *p_Param = u_ParamMax;
    else
      (*p_Param)--;
  }

  lcd_gotoxy(0,1);

  if( p_String )
    lcd_puts( p_String[*p_Param] );        
  else
    lcd_putc( *p_Param + 48 );
}

