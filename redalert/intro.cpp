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

/* $Header: /CounterStrike/INTRO.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : INTRO.H                                                      *
 *                                                                                             *
 *                   Programmer : Barry W. Green                                               *
 *                                                                                             *
 *                   Start Date : May 8, 1995                                                  *
 *                                                                                             *
 *                  Last Update : May 8, 1995  [BWG]                                           *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "function.h"

#if (0) // PG
VQAHandle* Open_Movie(char* name);
VQAHandle* Open_Movie(char* name)
{
    if (!Debug_Quiet && Get_Digi_Handle() != -1) {
        AnimControl.OptionFlags |= VQAOPTF_AUDIO;
    } else {
        AnimControl.OptionFlags &= ~VQAOPTF_AUDIO;
    }

    VQAHandle* vqa = VQA_Alloc();
    if (vqa) {
        VQA_Init(vqa, MixFileHandler);

        if (VQA_Open(vqa, name, &AnimControl) != 0) {
            VQA_Free(vqa);
            vqa = 0;
        }
    }
    return (vqa);
}
#endif

/***********************************************************************************************
 * Choose_Side -- play the introduction movies, select house                                   *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   5/08/1995 BWG : Created.                                                                  *
 *=============================================================================================*/
void Choose_Side(void) //	ajw - In RA, all this did was play a movie. Denzil is using it in its original sense.
{

    Whom = HOUSE_GOOD;

#if (0) // PG
    if (Special.IsFromInstall) {
#ifdef DVD // Denzil
        if (Using_DVD()) {
            Hide_Mouse();
            Load_Title_Page();
            GamePalette = CCPalette;
            HidPage.Blit(SeenPage);
            CCPalette.Set();
            Set_Logic_Page(SeenBuff);
            Show_Mouse();

            switch (WWMessageBox().Process(TXT_CHOOSE, TXT_ALLIES, TXT_SOVIET)) {
            case 0:
                CurrentCD = 0;
                break;

            case 1:
                CurrentCD = 1;
                break;
            }

            Hide_Mouse();
            BlackPalette.Set(FADE_PALETTE_SLOW);
            SeenPage.Clear();
        }
#endif

        Play_Movie(VQ_INTRO_MOVIE, THEME_NONE, false);
    }
#endif
    //	Scen.ScenPlayer = SCEN_PLAYER_GREECE;
}
