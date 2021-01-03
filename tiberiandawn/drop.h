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

/* $Header: /CounterStrike/DROP.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : DROP.H                                                       *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 07/05/96                                                     *
 *                                                                                             *
 *                  Last Update : July 5, 1996 [JLB]                                           *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef DROP_H
#define DROP_H

#include "list.h"
#include "edit.h"
#include "common/keyframe.h"

class DropListClass : public EditClass
{
public:
    DropListClass(int id,
                  char* text,
                  int max_len,
                  TextPrintType flags,
                  int x,
                  int y,
                  int w,
                  int h,
                  void const* up,
                  void const* down);
    virtual ~DropListClass(void){};

    virtual DropListClass& Add(LinkClass& object);
    virtual DropListClass& Add_Tail(LinkClass& object);
    virtual DropListClass& Add_Head(LinkClass& object);
    virtual DropListClass* Remove(void);
    virtual void Zap(void);

    virtual int Add_Item(char const* text);
    virtual char const* Current_Item(void);
    virtual int Current_Index(void);
    virtual void Set_Selected_Index(int index);
    virtual void Set_Selected_Index(char const* text);
    virtual void Peer_To_Peer(unsigned flags, KeyNumType&, ControlClass& whom);
    virtual void Clear_Focus(void);
    virtual int Count(void) const
    {
        return (List.Count());
    };
    virtual char const* Get_Item(int index) const
    {
        return (List.Get_Item(index));
    };

    virtual void Flag_To_Redraw(void);

    void Expand(void);
    void Collapse(void);

    virtual void Set_Position(int x, int y);

    DropListClass& operator=(DropListClass const& list);
    DropListClass(DropListClass const& list);

    /*
    **	Indicates whether the list box has dropped down or not.
    */
    unsigned IsDropped : 1;

    /*
    **	Height of list box when it is expanded.
    */
    int ListHeight;

    /*
    **	Drop down button.
    */
    ShapeButtonClass DropButton;

    /*
    **	List object when it is expanded.
    */
    ListClass List;
};

#endif
