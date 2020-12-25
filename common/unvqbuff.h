// TiberianDawn.DLL and RedAlert.dll and corresponding source code is free
// software: you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.

// TiberianDawn.DLL and RedAlert.dll and corresponding source code is distributed
// in the hope that it will be useful, but with permitted additional restrictions
// under Section 7 of the GPL. See the GNU General Public License in LICENSE.TXT
// distributed with this program. You should have received a copy of the
// GNU General Public License along with permitted additional restrictions
// with this program. If not, see https://github.com/electronicarts/CnC_Remastered_Collection
#ifndef UNVQBUFF_H
#define UNVQBUFF_H

#include <stdint.h>

void UnVQ_4x2(uint8_t* codebook,
              uint8_t* pointers,
              uint8_t* buffer,
              unsigned blocks_per_row,
              unsigned num_rows,
              unsigned buff_width);
void UnVQ_4x4(uint8_t* codebook,
              uint8_t* pointers,
              uint8_t* buffer,
              unsigned blocks_per_row,
              unsigned num_rows,
              unsigned buff_width);
void UnVQ_Nop(uint8_t* codebook,
              uint8_t* pointers,
              uint8_t* buffer,
              unsigned blocks_per_row,
              unsigned num_rows,
              unsigned buff_width);

#endif
