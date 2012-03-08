/* 
common.h
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

#ifndef COMMON_H
#define COMMON_H

#include "types.h"

typedef struct
{
  uint16 w_ADCBatteryVoltage;
  uint16 w_ADCCurrent;
  uint16 w_CutoffVoltage;
  uint16 w_DischargeCurrent;
  uint32 q_CapacityDischarged;
  uint8  u_Hours;
  uint8  u_Minutes;
  uint8  u_Seconds;
} StatusType;

extern StatusType z_Status;

#endif

