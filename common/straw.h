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

/* $Header: /CounterStrike/STRAW.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : STRAW.H                                                      *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 07/02/96                                                     *
 *                                                                                             *
 *                  Last Update : July 2, 1996 [JLB]                                           *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef STRAW_H
#define STRAW_H

#include "endianness.h"
#include "fixed.h"
#include <stdlib.h>
#include <stdint.h>

/*
**	This is a demand driven data carrier. It will retrieve the byte request by passing
**	the request down the chain (possibly processing on the way) in order to fulfill the
**	data request. Without being derived, this class merely passes the data through. Derived
**	versions are presumed to modify the data in some useful way or monitor the data
**	flow.
*/
class Straw
{
public:
    Straw(void)
        : ChainTo(0)
        , ChainFrom(0)
    {
    }
    virtual ~Straw(void);

    virtual void Get_From(Straw* pipe);
    void Get_From(Straw& pipe)
    {
        Get_From(&pipe);
    }
    virtual int Get(void* buffer, int slen);

    bool Get(int8_t& val)
    {
        uint8_t data;
        bool success = Get(&data, sizeof(data)) == sizeof(data);
        val = data;
        return success;
    }

    bool Get(uint8_t& val)
    {
        uint8_t data;
        bool success = Get(&data, sizeof(data)) == sizeof(data);
        val = data;
        return success;
    }

    bool Get(int16_t& val)
    {
        uint16_t data;
        bool success = Get(&data, sizeof(data)) == sizeof(data);
        val = le16toh(data);
        return success;
    }

    bool Get(uint16_t& val)
    {
        uint16_t data;
        bool success = Get(&data, sizeof(data)) == sizeof(data);
        val = le16toh(data);
        return success;
    }

    bool Get(int32_t& val)
    {
        uint32_t data;
        bool success = Get(&data, sizeof(data)) == sizeof(data);
        val = le32toh(data);
        return success;
    }

    bool Get(uint32_t& val)
    {
        uint32_t data;
        bool success = Get(&data, sizeof(data)) == sizeof(data);
        val = le32toh(data);
        return success;
    }

    bool Get(int64_t& val)
    {
        uint64_t data;
        bool success = Get(&data, sizeof(data)) == sizeof(data);
        val = le64toh(data);
        return success;
    }

    bool Get(uint64_t& val)
    {
        uint64_t data;
        bool success = Get(&data, sizeof(data)) == sizeof(data);
        val = le64toh(data);
        return success;
    }

    bool Get(fixed& val)
    {
        uint32_t data;
        bool success = Get(&data, sizeof(data)) == sizeof(data);
        static_assert(sizeof(data) == sizeof(val.Data.Raw), "Fixed point data does not match written data size.");
        val.Data.Raw = le32toh(data);
        return success;
    }

    /*
    **	Pointer to the next pipe segment in the chain.
    */
    Straw* ChainTo;
    Straw* ChainFrom;

private:
    /*
    **	Disable the copy constructor and assignment operator.
    */
    Straw(Straw& rvalue);
    Straw& operator=(Straw const& pipe);
};

#endif
