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

/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : UTRACKER.H                               *
 *                                                                         *
 *                   Programmer : Steve Tall                               *
 *                                                                         *
 *                   Start Date : June 3rd, 1996                           *
 *                                                                         *
 *                  Last Update : June 7th, 1996 [ST]                      *
 *                                                                         *
 *-------------------------------------------------------------------------*
 *  The UnitTracker class exists to track the various statistics           *
 *   required for internet games.                                          *
 *                                                                         *
 *                                                                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef UTRACKER_H
#define UTRACKER_H
#include <cstdlib>

/*
** UnitTracker Class
*/

template <int N> class UnitTrackerClass
{

public:
    inline void Init()
    {
        InNetworkFormat = 0; // The unit entries are in host format
        Clear_Unit_Total();  // Clear each entry
    }

    inline UnitTrackerClass()
    {
        Init();
    }

    /* Increment the total for the specefied unit */
    inline void Increment_Unit_Total(int unit_type)
    {
        UnitTotals[unit_type]++;
    }

    /* Decrement the total for the specefied unit */
    inline void Decrement_Unit_Total(int unit_type)
    {
        UnitTotals[unit_type]--;
    }

    /* Clear out all the unit totals */
    inline void Clear_Unit_Total(void)
    {
        memset(UnitTotals, 0, N * sizeof(int));
    }

    /* Returns a pointer to the start of the unit totals list */
    int Get_Unit_Total(int unit_type)
    {
        if (UnitTotals == NULL) {
            return 0;
        }
        if (unit_type >= 0 && unit_type < N) {
            return UnitTotals[unit_type];
        }
        return 0;
    }

    /* Returns a pointer to the start of the unit totals list */
    inline int* Get_All_Totals(void)
    {
        return (UnitTotals);
    }

    inline int Get_Unit_Count(void)
    {
        return (N);
    };

    /* Changes all unit totals to network format for the internet */
    inline void To_Network_Format(void)
    {
        if (!InNetworkFormat) {
            for (int i = 0; i < N; i++) {
                UnitTotals[i] = hton32(UnitTotals[i]);
            }
        }
        InNetworkFormat = 1; // Flag that data is now in network format
    }

    /* Changes all unit totals to PC format from network format */
    void To_PC_Format(void)
    {
        if (InNetworkFormat) {
            for (int i = 0; i < N; i++) {
                UnitTotals[i] = ntoh32(UnitTotals[i]);
            }
        }
        InNetworkFormat = 0; // Flag that data is now in PC format
    }

private:
    int InNetworkFormat;
    int UnitTotals[N];
};

#endif
