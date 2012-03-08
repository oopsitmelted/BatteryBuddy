/* 
config.h
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

#ifndef CONFIG_H
#define CONFIG_H

#include "types.h"

typedef enum
{
  MODE_FULL_DISCHARGE,
  MODE_STORAGE,
  MODE_MAX
} ModeEnumType;

typedef enum
{
  CELL_TYPE_NIMH,
  CELL_TYPE_LIPO,
  CELL_TYPE_MAX
} CellTypeEnumType;

typedef enum
{
  DISCHARGE_CURRENT_50MA,
  DISCHARGE_CURRENT_100MA,
  DISCHARGE_CURRENT_200MA,
  DISCHARGE_CURRENT_400MA,
  DISCHARGE_CURRENT_CUSTOM,
  DISCHARGE_CURRENT_MAX
} DischargeCurrentEnumType;

typedef struct
{
  ModeEnumType e_Mode;
  CellTypeEnumType e_CellType;
  uint8 u_NumCells;
  DischargeCurrentEnumType e_DischargeCurrent;
  uint16 w_DischargeCurrentCustom;
} z_ConfigStructType;

extern z_ConfigStructType z_Config;

/* Write configuration parameters to EEPROM 
 *   Returns TRUE if success, FALSE if failure
 */
uint8 ConfigWriteEEPROM( z_ConfigStructType *p_Config );

/* Attempt to read configuration from EEPROM
 *   Returns TRUE if success, FALSE if failure
 */
uint8 ConfigReadEEPROM( z_ConfigStructType *p_Config );

/* Utility function to handle configuration parameter selection via encoder 
 *   u_Flags - ISR flags
 *   p_Param - Pointer to configuration parameter
 *   u_ParamMin - Parameter minimum value
 *   u_ParamMax - Parameter maximum value
 *   p_String - Pointer to array of strings to be displayed for each value. If NULL display *p_Param instead
 */
void ConfigParameter( uint8 u_Flags, uint8 *p_Param, uint8 u_ParamMin, 
                      uint8 u_ParamMax, char const **p_String );

#endif
