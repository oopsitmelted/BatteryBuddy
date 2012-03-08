/* 
isr.h
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

#ifndef ISR_H
#define ISR_H

#include "types.h"

#define C_ISR_FLAG_1HZ_TICK           ( 1 << 0 )
#define C_ISR_FLAG_ENCODER_CW         ( 1 << 1 )
#define C_ISR_FLAG_ENCODER_CCW        ( 1 << 2 )
#define C_ISR_FLAG_SHORT_BUTTON_PRESS ( 1 << 3 )
#define C_ISR_FLAG_LONG_BUTTON_PRESS  ( 1 << 4 )

/* Returns current ISR flag values */
uint8 ISRGetFlags( void );

#endif
