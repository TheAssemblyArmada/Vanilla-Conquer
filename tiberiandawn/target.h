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

/* $Header:   F:\projects\c&c\vcs\code\target.h_v   2.16   16 Oct 1995 16:45:04   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : TARGET.H                                                     *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : April 25, 1994                                               *
 *                                                                                             *
 *                  Last Update : April 25, 1994   [JLB]                                       *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef TARGET_H
#define TARGET_H

#include "defines.h"

/**************************************************************************
** When a unit proceeds with carrying out its mission, it can have several
**	intermediate goals. Each goal (or target if you will) can be one of the
**	following kinds.
*/
typedef enum KindType : unsigned char
{
    KIND_NONE,
    KIND_CELL,
    KIND_UNIT,
    KIND_INFANTRY,
    KIND_BUILDING,
    KIND_TERRAIN,
    KIND_AIRCRAFT,
    KIND_TEMPLATE,
    KIND_BULLET,
    KIND_ANIMATION,
    KIND_TRIGGER,
    KIND_TEAM,
    KIND_TEAMTYPE
} KindType;

inline TARGET Build_Target(KindType kind, int value)
{
    TARGET_COMPOSITE target;

    target.Target = 0;
    target.Sub.Exponent = kind;
    target.Sub.Mantissa = value;
    return (target.Target);
}

inline KindType Target_Kind(TARGET a)
{
    return (KindType(((TARGET_COMPOSITE&)a).Sub.Exponent));
}

inline unsigned Target_Value(TARGET a)
{
    return (((TARGET_COMPOSITE&)a).Sub.Mantissa);
}

inline bool Is_Target_Team(TARGET a)
{
    return (Target_Kind(a) == KIND_TEAM);
}
inline bool Is_Target_TeamType(TARGET a)
{
    return (Target_Kind(a) == KIND_TEAMTYPE);
}
inline bool Is_Target_Trigger(TARGET a)
{
    return (Target_Kind(a) == KIND_TRIGGER);
}
inline bool Is_Target_Infantry(TARGET a)
{
    return (Target_Kind(a) == KIND_INFANTRY);
}
inline bool Is_Target_Bullet(TARGET a)
{
    return (Target_Kind(a) == KIND_BULLET);
}
inline bool Is_Target_Terrain(TARGET a)
{
    return (Target_Kind(a) == KIND_TERRAIN);
}
inline bool Is_Target_Cell(TARGET a)
{
    return (Target_Kind(a) == KIND_CELL);
}
inline bool Is_Target_Unit(TARGET a)
{
    return (Target_Kind(a) == KIND_UNIT);
}
inline bool Is_Target_Building(TARGET a)
{
    return (Target_Kind(a) == KIND_BUILDING);
}
inline bool Is_Target_Template(TARGET a)
{
    return (Target_Kind(a) == KIND_TEMPLATE);
}
inline bool Is_Target_Aircraft(TARGET a)
{
    return (Target_Kind(a) == KIND_AIRCRAFT);
}
inline bool Is_Target_Animation(TARGET a)
{
    return (Target_Kind(a) == KIND_ANIMATION);
}

inline TARGET As_Target(CELL cell)
{
    return (TARGET)(((unsigned)KIND_CELL << TARGET_MANTISSA) | cell);
}

class UnitClass;
class BuildingClass;
class TechnoClass;
class TerrainClass;
class ObjectClass;
class InfantryClass;
class BulletClass;
class TriggerClass;
class TeamClass;
class TeamTypeClass;
class AnimClass;
class AircraftClass;

/*
** Must not have a constructor since Watcom cannot handle a class that has a constructor if
** that class object is in a union. Don't use this class for normal purposes. Use the TargetClass
**	instead. The xTargetClass is only used in one module for a special reason -- keep it that way.
*/
class xTargetClass
{
protected:
    TARGET_COMPOSITE Target;

public:
    // conversion operator to RTTIType
    operator RTTIType(void) const
    {
        return (RTTIType(Target.Sub.Exponent));
    }

    // comparison operator
    int operator==(xTargetClass& tgt)
    {
        return (tgt.Target.Target == Target.Target ? 1 : 0);
    }

    // conversion operator to regular TARGET type
    TARGET As_TARGET(void) const
    {
        return (Target.Target);
    }

    unsigned Value(void) const
    {
        return (Target.Sub.Mantissa);
    };

    void Invalidate(void)
    {
        Target.Sub.Exponent = RTTI_NONE;
        Target.Sub.Mantissa = -1;
    }
    bool Is_Valid(void) const
    {
        return (Target.Sub.Exponent != RTTI_NONE);
    }

    TARGET As_Target(void) const
    {
        return (Target.Target);
    }
    AbstractTypeClass* As_TypeClass(void) const;
    AbstractClass* As_Abstract(bool check_active = true) const;
    TechnoClass* As_Techno(bool check_active = true) const;
    ObjectClass* As_Object(bool check_active = true) const;

    /*
    **	Helper routines to combine testing for, and fetching a pointer to, the
    **	type of object indicated.
    */
    TerrainClass* As_Terrain(bool check_active = true) const
    {
        if (*this == RTTI_TERRAIN)
            return ((TerrainClass*)As_Abstract(check_active));
        return (0);
    }
    BulletClass* As_Bullet(bool check_active = true) const
    {
        if (*this == RTTI_BULLET)
            return ((BulletClass*)As_Abstract(check_active));
        return (0);
    }
    AnimClass* As_Anim(bool check_active = true) const
    {
        if (*this == RTTI_ANIM)
            return ((AnimClass*)As_Abstract(check_active));
        return (0);
    }
    TeamClass* As_Team(bool check_active = true) const
    {
        if (*this == RTTI_TEAM)
            return ((TeamClass*)As_Abstract(check_active));
        return (0);
    }
    InfantryClass* As_Infantry(bool check_active = true) const
    {
        if (*this == RTTI_INFANTRY)
            return ((InfantryClass*)As_Techno(check_active));
        return (0);
    }
    UnitClass* As_Unit(bool check_active = true) const
    {
        if (*this == RTTI_UNIT)
            return ((UnitClass*)As_Techno(check_active));
        return (0);
    }
    BuildingClass* As_Building(bool check_active = true) const
    {
        if (*this == RTTI_BUILDING)
            return ((BuildingClass*)As_Techno(check_active));
        return (0);
    }
    AircraftClass* As_Aircraft(bool check_active = true) const
    {
        if (*this == RTTI_AIRCRAFT)
            return ((AircraftClass*)As_Techno(check_active));
        return (0);
    }
};

/*
**	This class only serves as a wrapper to the xTargetClass. This class must not define any members except
**	for the constructors. This is because the xTargetClass is used in a union and this target object is
**	used as its initializer. If this class had any extra members they would not be properly copied and
**	communicated to the other machines in a network/modem game. Combining this class with xTargetClass would
**	be more efficient, but Watcom doesn't allow class objects that have a constructor to be part of a union [even
**	if the class object has a default constructor!].
*/
class TargetClass : public xTargetClass
{
public:
    TargetClass(void)
    {
        Invalidate();
    }
    TargetClass(NoInitClass const&)
    {
    }
    TargetClass(RTTIType rtti, int id)
    {
        Target.Sub.Exponent = rtti;
        Target.Sub.Mantissa = id;
    }
    TargetClass(TARGET target);
    TargetClass(AbstractClass const* ptr);
    TargetClass(AbstractTypeClass const* ptr);
};

#endif
