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

/* $Header: /CounterStrike/IPX.H 1     3/03/97 10:24a Joe_bostic $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : IPX.H                                    *
 *                                                                         *
 *                   Programmer : Barry Nance										*
 * 										 from Client/Server LAN Programming			*
 *											 Westwood-ized by Bill Randolph				*
 *                                                                         *
 *                   Start Date : December 14, 1994                        *
 *                                                                         *
 *                  Last Update : December 14, 1994   [BR]                 *
 *                                                                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef IPX_H
#define IPX_H

/*
******************************** Structures *********************************
*/
typedef unsigned char NetNumType[4];
typedef unsigned char NetNodeType[6];
typedef char UserID[48];

/*---------------------------------------------------------------------------
This is the IPX Packet structure.  It's followed by the data itself, which
can be up to 546 bytes long.  Annotation of 'IPX' means IPX will set this
field; annotation of 'APP' means the application must set the field.
NOTE: All header fields are ordered high-byte,low-byte.
---------------------------------------------------------------------------*/
typedef struct IPXHEADER
{
    unsigned short CheckSum;              // IPX: Not used; always 0xffff
    unsigned short Length;                // IPX: Total size, incl header & data
    unsigned char TransportControl;       // IPX: # bridges message crossed
    unsigned char PacketType;             // APP: Set to 4 for IPX (5 for SPX)
    unsigned char DestNetworkNumber[4];   // APP: destination Network Number
    unsigned char DestNetworkNode[6];     // APP: destination Node Address
    unsigned short DestNetworkSocket;     // APP: destination Socket Number
    unsigned char SourceNetworkNumber[4]; // IPX: source Network Number
    unsigned char SourceNetworkNode[6];   // IPX: source Node Address
    unsigned short SourceNetworkSocket;   // IPX: source Socket Number
} IPXHeaderType;

/*---------------------------------------------------------------------------
This is the IPX Event Control Block.  It serves as a communications area
between IPX and the application for a single IPX operation.  You should set
up a separate ECB for each IPX operation you perform.
---------------------------------------------------------------------------*/
typedef struct ECB
{
    void* Link_Address;
    void (*Event_Service_Routine)(void); // APP: event handler (NULL=none)
    unsigned char InUse;                 // IPX: 0 = event complete
    unsigned char CompletionCode;        // IPX: event's return code
    unsigned short SocketNumber;         // APP: socket to send data through
    unsigned short ConnectionID;         // returned by Listen (???)
    unsigned short RestOfWorkspace;
    unsigned char DriverWorkspace[12];
    unsigned char ImmediateAddress[6]; // returned by Get_Local_Target
    unsigned short PacketCount;
    struct
    {
        void* Address;
        unsigned short Length;
    } Packet[2];
} ECBType;

/*---------------------------------------------------------------------------
This structure is used for calling DPMI function 0x300, Call-Real-Mode-
Interrupt.  It passes register values to & from the interrupt handler.
---------------------------------------------------------------------------*/
typedef struct
{
    int edi;
    int esi;
    int ebp;
    int Reserved;
    int ebx;
    int edx;
    int ecx;
    int eax;
    short Flags;
    short es;
    short ds;
    short fs;
    short gs;
    short ip;
    short cs;
    short sp;
    short ss;
} RMIType;

#endif

/***************************** end of ipx.h ********************************/
