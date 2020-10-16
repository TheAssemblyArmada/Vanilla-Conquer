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

/* $Header: /CounterStrike/LINK.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : LINK.H                                                       *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 01/15/95                                                     *
 *                                                                                             *
 *                  Last Update : January 15, 1995 [JLB]                                       *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef LINK_H
#define LINK_H

struct NoInitClass;

/*
**	This implements a simple linked list. It is possible to add, remove, and traverse the
**	list. Since this is a doubly linked list, it is possible to remove an entry from the
**	middle of an existing list.
*/
class LinkClass
{
public:
    LinkClass(NoInitClass const&){};
    LinkClass(void)
        : Next(0)
        , Prev(0){};
    virtual ~LinkClass(void);

    virtual LinkClass* Get_Next(void) const;
    virtual LinkClass* Get_Prev(void) const;
    virtual LinkClass& Add(LinkClass& object);
    virtual LinkClass& Add_Tail(LinkClass& object);
    virtual LinkClass& Add_Head(LinkClass& object);
    virtual LinkClass& Head_Of_List(void);
    virtual LinkClass& Tail_Of_List(void);
    virtual void Zap(void);
    virtual LinkClass* Remove(void);

    LinkClass& operator=(LinkClass const& link); // Assignment operator.
    LinkClass(LinkClass const& link);            // Copy constructor.

private:
    /*
    **	Pointers to previous and next link objects in chain.
    */
    LinkClass* Next;
    LinkClass* Prev;
};

#endif
