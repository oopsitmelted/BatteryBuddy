/* 
ina219.h
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

#ifndef INA219_H
#define INA219_H

#include "types.h"

/* Initialize ina219 IC */
void   ina219_init();

/* Read a register from the ina219 */
uint16 ina219_read_voltage( void );

/* Read current in mA */
uint16 ina219_read_current( void );
uint16 ina219_read_power( void );

#endif

