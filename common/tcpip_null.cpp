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
 *                 Project Name : Command & Conquer - Red Alert            *
 *                                                                         *
 *                    File Name : TCPIP.CPP                                *
 *                                                                         *
 *                   Programmer : Steve Tall                               *
 *                                                                         *
 *                   Start Date : March 11th, 1996                         *
 *                                                                         *
 *                  Last Update : March 20th, 1996 [ST]                    *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Overview:                                                               *
 *                                                                         *
 *  Member functions of the TcpipManagerClass which provides the Winsock   *
 * interface for C&C                                                       *
 *                                                                         *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *                                                                         *
 * TMC::TcpipManagerClass -- constructor for the TcpipManagerClass         *
 * TMC::~TcpipManagerClass -- destructor for the TcpipManagerClass         *
 * TMC::Close -- restores any currently in use Winsock resources           *
 * TMC::Init -- Initialised Winsock for use.                               *
 * TMC::Start_Server -- Initialise connection and start listening.         *
 * TMC::Read -- read any pending input from the stream socket              *
 * TMC::Write -- Send data via the Winsock streaming socket                *
 * TMC::Add_Client -- A client has requested to connect.                   *
 * TMC::Message_Handler -- Message handler for Winsock.                    *
 * TMC::Set_Host_Address -- Set the address of the host                    *
 * TMC::Start_Client -- Start trying to connect to a game host             *
 * TMC::Close_Socket -- Close an opened Winsock socket.                    *
 * TMC::Copy_To_In_Buffer -- copy data from our winsock buffer             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "tcpip.h"

#ifndef NETWORKING

/*
** Nasty globals
*/
bool Server; // Is this player acting as client or server
/* these are temporary to allow linking, this manager class is not used at the moment -hifi */
char PlanetWestwoodIPAddress[IP_ADDRESS_MAX];
unsigned short PlanetWestwoodPortNumber;
bool PlanetWestwoodIsHost;

TcpipManagerClass Winsock; // The object for interfacing with Winsock

/***********************************************************************************************
 * TMC::TcpipManagerClass -- constructor for the TcpipManagerClass                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    3/20/96 2:51PM ST : Created                                                              *
 *=============================================================================================*/
TcpipManagerClass::TcpipManagerClass(void)
{
}

/***********************************************************************************************
 * TMC::~TcpipManagerClass -- destructor for the TcpipManagerClass                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    3/20/96 2:52PM ST : Created                                                              *
 *=============================================================================================*/

TcpipManagerClass::~TcpipManagerClass(void)
{
}

/***********************************************************************************************
 * TMC::Close -- restores any currently in use Winsock resources                               *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    3/20/96 2:52PM ST : Created                                                              *
 *=============================================================================================*/

void TcpipManagerClass::Close(void)
{
}

/***********************************************************************************************
 * TMC::Init -- Initialised Winsock for use.                                                   *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   TRUE if Winsock is available and was initialised                                  *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    3/20/96 2:54PM ST : Created                                                              *
 *=============================================================================================*/

bool TcpipManagerClass::Init(void)
{
    return false;
}

/***********************************************************************************************
 * TMC::Start_Server -- initialise out connection as the server. Start listening for clients.  *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    3/20/96 2:56PM ST : Created                                                              *
 *=============================================================================================*/

void TcpipManagerClass::Start_Server(void)
{
}

/***********************************************************************************************
 * TMC::Read -- read any pending input from the stream socket                                  *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to buffer to receive input                                                    *
 *           length of buffer                                                                  *
 *                                                                                             *
 * OUTPUT:   number of bytes transfered to buffer                                              *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    3/20/96 2:58PM ST : Created                                                              *
 *=============================================================================================*/

int TcpipManagerClass::Read(void* buffer, int buffer_len)
{
    return 0;
}

/***********************************************************************************************
 * TMC::Write -- Send data via the Winsock UDP socket                                          *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to buffer containing data to send                                             *
 *           length of data to send                                                            *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    3/20/96 3:00PM ST : Created                                                              *
 *=============================================================================================*/

void TcpipManagerClass::Write(void* buffer, int buffer_len)
{
}

/***********************************************************************************************
 * TMC::Add_Client -- a client has requested to connect. Make the connection                   *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   TRUE if client was successfully connected                                         *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    3/20/96 3:02PM ST : Created                                                              *
 *=============================================================================================*/

bool TcpipManagerClass::Add_Client(void)
{
    return false;
}

/***********************************************************************************************
 * TMC::Message_Handler -- Message handler function for Winsock related messages               *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Windows message handler stuff                                                     *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    3/20/96 3:05PM ST : Created                                                              *
 *=============================================================================================*/
#ifdef _WIN32
void TcpipManagerClass::Message_Handler(HWND, UINT message, UINT, LONG lParam)
{
}
#endif

/***********************************************************************************************
 * TMC::Copy_To_In_Buffer -- copy data from our winsock buffer to our internal buffer          *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    bytes to copy                                                                     *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    3/20/96 3:17PM ST : Created                                                              *
 *=============================================================================================*/
void TcpipManagerClass::Copy_To_In_Buffer(int bytes)
{
}

/***********************************************************************************************
 * TMC::Set_Host_Address -- Set the address of the host game we want to connect to             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to address string                                                             *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    3/20/96 3:19PM ST : Created                                                              *
 *=============================================================================================*/
void TcpipManagerClass::Set_Host_Address(char* address)
{
}

/***********************************************************************************************
 * TMC::Start_Client -- Start trying to connect to a game host                                 *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    3/20/96 3:19PM ST : Created                                                              *
 *=============================================================================================*/

void TcpipManagerClass::Start_Client(void)
{
}

/***********************************************************************************************
 * TMC::Close_Socket -- Close an opened Winsock socket.                                        *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Socket to close                                                                   *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    3/20/96 3:24PM ST : Created                                                              *
 *=============================================================================================*/

void TcpipManagerClass::Close_Socket(SOCKET s)
{
}

void TcpipManagerClass::Set_Protocol_UDP(bool state)
{
}

void TcpipManagerClass::Clear_Socket_Error(SOCKET socket)
{
}

#endif
