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

/* $Header: /CounterStrike/CCPTR.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : CCPTR.CPP                                                    *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 06/07/96                                                     *
 *                                                                                             *
 *                  Last Update : July 6, 1996 [JLB]                                           *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   CCPtr<T>::operator > -- Greater than comparison operator.                                 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "function.h"

template class CCPtr<AircraftClass>;
template CCPtr<AircraftClass>::CCPtr(AircraftClass *);
template class CCPtr<AircraftTypeClass>;
template CCPtr<AircraftTypeClass>::CCPtr(AircraftTypeClass *);
template class CCPtr<AnimClass>;
template CCPtr<AnimClass>::CCPtr(AnimClass *);
template class CCPtr<AnimTypeClass>;
template CCPtr<AnimTypeClass>::CCPtr(AnimTypeClass *);
template class CCPtr<BuildingClass>;
template CCPtr<BuildingClass>::CCPtr(BuildingClass *);
template class CCPtr<BuildingTypeClass>;
template CCPtr<BuildingTypeClass>::CCPtr(BuildingTypeClass *);
template class CCPtr<BulletClass>;
template CCPtr<BulletClass>::CCPtr(BulletClass *);
template class CCPtr<BulletTypeClass>;
template CCPtr<BulletTypeClass>::CCPtr(BulletTypeClass *);
template class CCPtr<FactoryClass>;
template CCPtr<FactoryClass>::CCPtr(FactoryClass *);
template class CCPtr<HouseClass>;
template CCPtr<HouseClass>::CCPtr(HouseClass *);
template class CCPtr<HouseTypeClass>;
template CCPtr<HouseTypeClass>::CCPtr(HouseTypeClass *);
template class CCPtr<InfantryClass>;
template CCPtr<InfantryClass>::CCPtr(InfantryClass *);
template class CCPtr<InfantryTypeClass>;
template CCPtr<InfantryTypeClass>::CCPtr(InfantryTypeClass *);
template class CCPtr<OverlayClass>;
template CCPtr<OverlayClass>::CCPtr(OverlayClass *);
template class CCPtr<OverlayTypeClass>;
template CCPtr<OverlayTypeClass>::CCPtr(OverlayTypeClass *);
template class CCPtr<SmudgeClass>;
template CCPtr<SmudgeClass>::CCPtr(SmudgeClass *);
template class CCPtr<SmudgeTypeClass>;
template CCPtr<SmudgeTypeClass>::CCPtr(SmudgeTypeClass *);
template class CCPtr<TeamClass>;
template CCPtr<TeamClass>::CCPtr(TeamClass *);
template class CCPtr<TeamTypeClass>;
template CCPtr<TeamTypeClass>::CCPtr(TeamTypeClass *);
template class CCPtr<TemplateClass>;
template CCPtr<TemplateClass>::CCPtr(TemplateClass *);
template class CCPtr<TemplateTypeClass>;
template CCPtr<TemplateTypeClass>::CCPtr(TemplateTypeClass *);
template class CCPtr<TerrainClass>;
template CCPtr<TerrainClass>::CCPtr(TerrainClass *);
template class CCPtr<TerrainTypeClass>;
template CCPtr<TerrainTypeClass>::CCPtr(TerrainTypeClass *);
template class CCPtr<TriggerClass>;
template CCPtr<TriggerClass>::CCPtr(TriggerClass *);
template class CCPtr<TriggerTypeClass>;
template CCPtr<TriggerTypeClass>::CCPtr(TriggerTypeClass *);
template class CCPtr<UnitClass>;
template CCPtr<UnitClass>::CCPtr(UnitClass *);
template class CCPtr<UnitTypeClass>;
template CCPtr<UnitTypeClass>::CCPtr(UnitTypeClass *);
template class CCPtr<VesselClass>;
template CCPtr<VesselClass>::CCPtr(VesselClass *);
template class CCPtr<VesselTypeClass>;
template CCPtr<VesselTypeClass>::CCPtr(VesselTypeClass *);
template class CCPtr<WarheadTypeClass>;
template CCPtr<WarheadTypeClass>::CCPtr(WarheadTypeClass *);
template class CCPtr<WeaponTypeClass>;
template CCPtr<WeaponTypeClass>::CCPtr(WeaponTypeClass *);

template <class T> FixedIHeapClass* CCPtr<T>::Heap = NULL;

/*
**	These member functions for the CCPtr class cannot be declared inside the
**	class definition since they could refer to other objects that themselves
**	contain CCPtr objects. The recursive nature of this type of declaration
**	is not handled by Watcom, hence the body declaration is dislocated here.
*/
template <class T>
CCPtr<T>::CCPtr(T* ptr)
    : ID(-1)
{
    if (ptr != NULL) {
        ID = ptr->ID;
    }
}

/***********************************************************************************************
 * CCPtr<T>::operator > -- Greater than comparison operator.                                   *
 *                                                                                             *
 *    This will compare two pointer value to see if the left hand value is greater than the    *
 *    right hand. The values are compared by comparing based on their Name() functions.        *
 *                                                                                             *
 * INPUT:   rvalue   -- The right handle CCPtr value.                                          *
 *                                                                                             *
 * OUTPUT:  Is the left hand value greater than the right hand value?                          *
 *                                                                                             *
 * WARNINGS:   The values pointed to by CCPtr must have a Name() function defined.             *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/06/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
template <class T> bool CCPtr<T>::operator>(CCPtr<T> const& rvalue) const
{
    return (stricmp((*this)->Name(), rvalue->Name()) > 0);
}
