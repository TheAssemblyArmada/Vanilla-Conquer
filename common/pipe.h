//
// Copyright 2020 Electronic Arts Inc.
//
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

/* $Header: /CounterStrike/PIPE.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : PIPE.H                                                       *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 06/29/96                                                     *
 *                                                                                             *
 *                  Last Update : June 29, 1996 [JLB]                                          *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef PIPE_H
#define PIPE_H

#include "endianness.h"
#include "fixed.h"
#include <stddef.h>
#include <stdint.h>

/*
**	A "push through" pipe interface abstract class used for such purposes as compression
**	and translation of data. In STL terms, this is functionally similar to an output
**	iterator but with a few enhancements. A pipe class object that is not derived into
**	another useful class serves only as a pseudo null-pipe. It will accept data but
**	just throw it away but pretend that it sent it somewhere.
*/
class Pipe
{
public:
    Pipe(void)
        : ChainTo(0)
        , ChainFrom(0)
    {
    }
    virtual ~Pipe(void);

    virtual int Flush(void);
    virtual int End(void)
    {
        return (Flush());
    }
    virtual void Put_To(Pipe* pipe);
    void Put_To(Pipe& pipe)
    {
        Put_To(&pipe);
    }
    virtual int Put(void const* source, int slen);

    /*
    **	Write fixed width data to the stream.
    */
    void Put(int8_t val)
    {
        uint8_t data = val;
        Put(&data, sizeof(data));
    }

    void Put(uint8_t val)
    {
        uint8_t data = val;
        Put(&data, sizeof(data));
    }

    void Put(int16_t val)
    {
        uint16_t data = htole16(val);
        Put(&data, sizeof(data));
    }

    void Put(uint16_t val)
    {
        uint16_t data = htole16(val);
        Put(&data, sizeof(data));
    }

    void Put(int32_t val)
    {
        uint32_t data = htole32(val);
        Put(&data, sizeof(data));
    }

    void Put(uint32_t val)
    {
        uint32_t data = htole32(val);
        Put(&data, sizeof(data));
    }

    void Put(int64_t val)
    {
        uint64_t data = htole64(val);
        Put(&data, sizeof(data));
    }

    void Put(uint64_t val)
    {
        uint64_t data = htole64(val);
        Put(&data, sizeof(data));
    }

    void Put(const fixed& val)
    {
        uint32_t data = htole32(val.Data.Raw);
        static_assert(sizeof(data) == sizeof(val.Data.Raw), "Fixed point data does not match written data size.");
        Put(&data, sizeof(data));
    }

    /*
    **	Pointer to the next pipe segment in the chain.
    */
    Pipe* ChainTo;
    Pipe* ChainFrom;

private:
    /*
    **	Disable the copy constructor and assignment operator.
    */
    Pipe(Pipe& rvalue);
    Pipe& operator=(Pipe const& pipe);
};

#endif
