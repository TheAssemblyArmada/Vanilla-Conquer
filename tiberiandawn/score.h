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

/* $Header:   F:\projects\c&c\vcs\code\score.h_v   2.18   16 Oct 1995 16:46:18   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : SCORE.H                                                      *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : April 19, 1994                                               *
 *                                                                                             *
 *                  Last Update : April 19, 1994   [JLB]                                       *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef SCORE_H
#define SCORE_H

#include "unit.h"
#include "building.h"

class ScoreClass
{
public:
    int Score;
    int NKilled;
    int GKilled;
    int CKilled;
    int NBKilled;
    int GBKilled;
    int CBKilled;
    int NHarvested;
    int GHarvested;
    int CHarvested;
    unsigned long ElapsedTime;

    ScoreClass(void)
    {
    }
    ScoreClass(NoInitClass const&)
    {
    }

    void Init(void)
    {
        memset(this, 0, sizeof(ScoreClass));
    };
    void Presentation(void);

    /*
    **	File I/O.
    */
    bool Load(FileClass& file);
    bool Save(FileClass& file);
    void Code_Pointers(void);
    void Decode_Pointers(void);

protected:
private:
    unsigned char* ChangingGun;

    void ScoreDelay(int ticks);
    void Pulse_Bar_Graph(void);
    void Print_Graph_Title(int, int);
    void Print_Minutes(int minutes);
    void Count_Up_Print(const char* str, int percent, int max, int xpos, int ypos);
    void Show_Credits(int house, char const pal[]);
    void Do_GDI_Graph(void const* yellowptr, void const* redptr, int gdikilled, int nodkilled, int ypos);
    void Do_Nod_Casualties_Graph(void);
    void Do_Nod_Buildings_Graph(void);
    void Input_Name(char str[], int xpos, int ypos, char const pal[]);
};

class ScoreAnimClass
{
public:
    ScoreAnimClass(int x, int y, void const* data);
    int XPos;
    int YPos;
    CountDownTimerClass Timer;
    void const* DataPtr;
    virtual void Update(void){};
    virtual ~ScoreAnimClass(void){};
};

class ScoreCredsClass : public ScoreAnimClass
{
public:
    int Stage;
    int MaxStage;
    int TimerReset;
    void const* CashTurn;
    void const* Clock1;

    virtual void Update(void);
    ScoreCredsClass(int xpos, int ypos, void const* data, int max, int timer);
    virtual ~ScoreCredsClass(void){};
};

class ScoreTimeClass : public ScoreAnimClass
{
public:
    int Stage;
    int MaxStage;
    int TimerReset;
    virtual void Update(void);
    ScoreTimeClass(int xpos, int ypos, void const* data, int max, int timer);
    virtual ~ScoreTimeClass(void){};
};

class ScorePrintClass : public ScoreAnimClass
{
public:
    int Background;
    int Stage;
    void const* PrimaryPalette;
    virtual void Update(void);
    ScorePrintClass(void const* string, int xpos, int ypos, void const* palette, int background = TBLACK);
    ScorePrintClass(int string, int xpos, int ypos, void const* palette, int background = TBLACK);
    virtual ~ScorePrintClass(void){};
};

class MultiStagePrintClass : public ScoreAnimClass
{
public:
    int Background;
    int Stage;
    void const* PrimaryPalette;
    virtual void Update(void);
    MultiStagePrintClass(void const* string, int xpos, int ypos, void const* palette, int background = TBLACK);
    MultiStagePrintClass(int string, int xpos, int ypos, void const* palette, int background = TBLACK);
    virtual ~MultiStagePrintClass(void){};
};

class ScoreScaleClass : public ScoreAnimClass
{
public:
    int Stage;
    char const* Palette;
    virtual void Update(void);
    ScoreScaleClass(void const* data, int xpos, int ypos, char const pal[]);
    virtual ~ScoreScaleClass(void){};
};

#define MAXSCOREOBJS 8
extern ScoreAnimClass* ScoreObjs[MAXSCOREOBJS];

void Multi_Score_Presentation(void);

#endif
