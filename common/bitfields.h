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

#ifndef COMMON_BITFIELDS_H
#define COMMON_BITFIELDS_H

/*
 * Use this macro to mark a structure to be packed in an MSVC compatible way.
 */
#if defined __clang__ || defined __GNUC__
#define BITFIELD_STRUCT __attribute__((ms_struct))
#else
#define BITFIELD_STRUCT
#endif

#endif /* COMMON_BITFIELDS_H */
