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

/* $Header: /CounterStrike/RGB.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : RGB.H                                                        *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 12/02/95                                                     *
 *                                                                                             *
 *                  Last Update : December 2, 1995 [JLB]                                       *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef RGB_H
#define RGB_H

class PaletteClass;
class HSVClass;

/*
**	Each color entry is represented by this class. It holds the values for the color
**	guns. The gun values are recorded in device dependent format, but the interface
**	uses gun values from 0 to 255.
*/
class RGBClass
{
    //	static RGBClass const BlackColor;

public:
    RGBClass(void)
        : Red(0)
        , Green(0)
        , Blue(0){};
    RGBClass(unsigned char red, unsigned char green, unsigned char blue)
        : Red((unsigned char)(red >> 2))
        , Green((unsigned char)(green >> 2))
        , Blue((unsigned char)(blue >> 2)){};
    operator HSVClass(void) const;
    RGBClass& operator=(RGBClass const& rgb)
    {
        if (this == &rgb)
            return (*this);

        Red = rgb.Red;
        Green = rgb.Green;
        Blue = rgb.Blue;
        return (*this);
    };

    enum
    {
        MAX_VALUE = 255
    };

    void Adjust(int ratio, RGBClass const& rgb);
    //		void Adjust(int ratio, RGBClass const & rgb = BlackColor);
    int Difference(RGBClass const& rgb) const;
    int Red_Component(void) const
    {
        return ((Red << 2) | (Red >> 6));
    };
    int Green_Component(void) const
    {
        return ((Green << 2) | (Green >> 6));
    };
    int Blue_Component(void) const
    {
        return ((Blue << 2) | (Blue >> 6));
    };
    void Set(int color) const;

private:
    friend class PaletteClass;

    void Raw_Set(void) const {
        // outrgb(Red, Green, Blue);

        //			outportb(0x03C9, Red);
        //			outportb(0x03C9, Green);
        //			outportb(0x03C9, Blue);
    };

    static void Raw_Color_Prep(unsigned char color){
        // outportb(0x03C8, color);
    };

    /*
    **	These hold the actual color gun values in machine dependent scale. This
    **	means the values range from 0 to 63.
    */
    unsigned char Red;
    unsigned char Green;
    unsigned char Blue;
};

extern RGBClass const BlackColor;

#endif
