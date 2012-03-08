/* 
ina219.c
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

#include "types.h"
#include "ina219.h"
#include "i2cmaster.h"

#define DEVICE_ADDRESS 0x80
#define CONFIG_REG_VAL ( (uint16) 0x219F )
#define CAL_REG_VAL    ( (uint16) 0x29B1 )

/* Initialize ina219 IC */
void ina219_init( void )
{
  /* Initialize I2C Controller */
  i2c_init();

  /* Set calibration word */
  i2c_start_wait( DEVICE_ADDRESS + I2C_WRITE );
  i2c_write( 0x5 ); // Calibration Register Address

  i2c_write( CAL_REG_VAL >> 8 );
  i2c_write( CAL_REG_VAL & 0xF );

  i2c_stop();

  /* Set config register */
  i2c_start_wait( DEVICE_ADDRESS + I2C_WRITE );
  i2c_write( 0x0 ); // Calibration Register Address

  i2c_write( CONFIG_REG_VAL >> 8 );
  i2c_write( CONFIG_REG_VAL & 0xF );

  i2c_stop();
}

/* Read a register from the ina219 */
static uint16 ina219_read( uint8 u_Register )
{
  uint16 w_Temp;

  i2c_start_wait( DEVICE_ADDRESS + I2C_WRITE );
  i2c_write( u_Register ); 
  
  i2c_rep_start( DEVICE_ADDRESS + I2C_READ );
  
  w_Temp = i2c_readAck() << 8;
  w_Temp |= i2c_readAck();

  i2c_stop();

  return( w_Temp );
}

/* Read load voltage in mV */
uint16 ina219_read_voltage( void )
{
  uint16 w_Voltage;

  /* Battery Voltage = Shunt Voltage + BusVoltage */
  w_Voltage =  ( ina219_read( 0x2 ) >> 3 ) * 4; // Bus
  w_Voltage += ina219_read( 0x1 ) / 100; // Shunt

  return( w_Voltage );
}

/* Read current in mA */
uint16 ina219_read_current( void )
{
  return( ina219_read( 0x4 ) / 10 );
}

uint16 ina219_read_power( void )
{
  return( ina219_read( 0x3 ) );
}

