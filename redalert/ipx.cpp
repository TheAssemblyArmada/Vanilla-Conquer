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

/* $Header: /CounterStrike/IPX.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : IPX.CPP                                  *
 *                                                                         *
 *                   Programmer : Barry Nance										*
 * 										 from Client/Server LAN Programming			*
 *											 Westwood-ized by Bill Randolph				*
 *                                                                         *
 *                   Start Date : December 14, 1994                        *
 *                                                                         *
 *                  Last Update : December 15, 1994   [BR]                 *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Pitfalls:																					*
 * - Never try to use a closed socket; always check the return code from	*
 *   IPX_Open_Socket().																		*
 * - Always give IPX an outstanding ECB for listening, before you send.		*
 * - It turns out that IPX is pretty bad about saving registers, so if		*
 *   you have any register variables in your program, they may get			*
 *   trashed.  To circumvent this, all functions in this module save &		*
 *   restore the registers before invoking any IPX or NETX function.			*
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   IPX_SPX_Installed -- checks for installation of IPX/SPX               *
 *   IPX_Open_Socket -- opens an IPX socket for sending or receiving       *
 *   IPX_Close_Socket -- closes an open socket                             *
 *   IPX_Get_Connection_Number -- gets local Connection Number					*
 *   IPX_Get_1st_Connection_Num -- gets 1st Connect Number for given user  *
 *   IPX_Get_Internet_Address -- gets Network Number & Node Address			*
 *   IPX_Get_User_ID -- gets user ID from Connection Number                *
 *   IPX_Listen_For_Packet -- commands IPX to listen for a packet          *
 *   IPX_Send_Packet -- commands IPX to send a packet                      *
 *   IPX_Get_Local_Target -- fills in ImmediateAddress field of ECB        *
 *   IPX_Cancel_Event -- cancels an operation in progress                  *
 *   Let_IPX_Breath -- gives IPX some CPU time                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "function.h"
#include <stdio.h>
#include "ipx.h"
#include "ipx95.h"

/***************************************************************************
 * IPX_SPX_Installed -- checks for installation of IPX/SPX                 *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.																						*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		0 = not installed; 1 = IPX only, 2 = IPX and SPX are installed			*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/14/1994 BR : Created.                                              *
 *=========================================================================*/
int IPX_SPX_Installed(void)
{
    return false;
} /* end of IPX_SPX_Installed */

/**************************** end of ipx.cpp *******************************/
