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

/* $Header: /CounterStrike/COMBUF.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : COMBUF.CPP                             	*
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : December 19, 1994                        *
 *                                                                         *
 *                  Last Update : October 23, 1995 [BRR]                   *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   CommBufferClass::CommBufferClass -- class constructor           		*
 *   CommBufferClass::~CommBufferClass -- class destructor           		*
 *   CommBufferClass::Init -- initializes this queue                       *
 *   CommBufferClass::Init_Send_Queue -- Clears the send queue             *
 *   CommBufferClass::Queue_Send -- queues a message for sending           *
 *   CommBufferClass::UnQueue_Send -- removes next entry from send queue	*
 *   CommBufferClass::Get_Send -- gets ptr to queue entry                  *
 *   CommBufferClass::Queue_Receive -- queues a received message				*
 *   CommBufferClass::UnQueue_Receive -- removes next entry from send queue*
 *   CommBufferClass::Get_Receive -- gets ptr to queue entry               *
 *   CommBufferClass::Add_Delay -- adds a new delay value for response time*
 *   CommBufferClass::Avg_Response_Time -- returns average response time  	*
 *   CommBufferClass::Max_Response_Time -- returns max response time  		*
 *   CommBufferClass::Reset_Response_Time -- resets computations				*
 *   Mono_Debug_Print -- Debug output routine                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include <stdio.h>
#include <string.h>
#include "combuf.h"

// todo for ConnectionClass::Command_Name in Mono_Debug_Print2, fix when moved
//#include "connect.h"        // for command names for debug output

/***************************************************************************
 * CommBufferClass::CommBufferClass -- class constructor             		*
 *                                                                         *
 * INPUT:                                                                  *
 *		numsend		# queue entries for sending										*
 *		numreceive	# queue entries for receiving										*
 *		maxlen		maximum desired packet length, in bytes						*
 *		extralen		max size of app-specific extra bytes (optional)				*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.																						*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/19/1994 BR : Created.                                              *
 *=========================================================================*/
CommBufferClass::CommBufferClass(int numsend, int numreceive, int maxlen, int extralen)
{
    int i;

    //------------------------------------------------------------------------
    //	Init variables
    //------------------------------------------------------------------------
    MaxSend = numsend;
    MaxReceive = numreceive;
    MaxPacketSize = maxlen;
    MaxExtraSize = extralen;

    //------------------------------------------------------------------------
    //	Allocate the queue entries
    //------------------------------------------------------------------------
    SendQueue = new SendQueueType[numsend];
    ReceiveQueue = new ReceiveQueueType[numreceive];

    SendIndex = new int[numsend];
    ReceiveIndex = new int[numreceive];

    //------------------------------------------------------------------------
    //	Allocate queue entry buffers
    //------------------------------------------------------------------------
    for (i = 0; i < MaxSend; i++) {
        SendQueue[i].Buffer = new char[maxlen];
        if (MaxExtraSize > 0) {
            SendQueue[i].ExtraBuffer = new char[MaxExtraSize];
        } else {
            SendQueue[i].ExtraBuffer = NULL;
        }
    }

    for (i = 0; i < MaxReceive; i++) {
        ReceiveQueue[i].Buffer = new char[maxlen];
        if (MaxExtraSize > 0) {
            ReceiveQueue[i].ExtraBuffer = new char[MaxExtraSize];
        } else {
            ReceiveQueue[i].ExtraBuffer = NULL;
        }
    }

    Init();

} /* end of CommBufferClass */

/***************************************************************************
 * CommBufferClass::~CommBufferClass -- class destructor             		*
 *                                                                         *
 * INPUT:                                                                  *
 *		none.																						*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.																						*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/19/1994 BR : Created.                                              *
 *=========================================================================*/
CommBufferClass::~CommBufferClass()
{
    int i;

    //------------------------------------------------------------------------
    //	Free queue entry buffers
    //------------------------------------------------------------------------
    for (i = 0; i < MaxSend; i++) {
        delete[] SendQueue[i].Buffer;
        if (SendQueue[i].ExtraBuffer) {
            delete[] SendQueue[i].ExtraBuffer;
        }
    }

    for (i = 0; i < MaxReceive; i++) {
        delete[] ReceiveQueue[i].Buffer;
        if (ReceiveQueue[i].ExtraBuffer) {
            delete[] ReceiveQueue[i].ExtraBuffer;
        }
    }

    delete[] SendQueue;
    delete[] ReceiveQueue;

    delete[] SendIndex;
    delete[] ReceiveIndex;

} /* end of ~CommBufferClass */

/***************************************************************************
 * CommBufferClass::Init -- initializes this queue                         *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.																						*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.																						*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   01/20/1995 BR : Created.                                              *
 *=========================================================================*/
void CommBufferClass::Init(void)
{
    int i;

    //------------------------------------------------------------------------
    //	Init data members
    //------------------------------------------------------------------------
    SendTotal = 0L;
    ReceiveTotal = 0L;

    DelaySum = 0L;
    NumDelay = 0L;
    MeanDelay = 0L;
    MaxDelay = 0L;

    SendCount = 0;

    ReceiveCount = 0;

    //------------------------------------------------------------------------
    //	Init the queue entries
    //------------------------------------------------------------------------
    for (i = 0; i < MaxSend; i++) {
        SendQueue[i].IsActive = 0;
        SendQueue[i].IsACK = 0;
        SendQueue[i].FirstTime = 0L;
        SendQueue[i].LastTime = 0L;
        SendQueue[i].SendCount = 0L;
        SendQueue[i].BufLen = 0;
        SendQueue[i].ExtraLen = 0;

        SendIndex[i] = 0;
    }

    for (i = 0; i < MaxReceive; i++) {
        ReceiveQueue[i].IsActive = 0;
        ReceiveQueue[i].IsRead = 0;
        ReceiveQueue[i].IsACK = 0;
        ReceiveQueue[i].BufLen = 0;
        ReceiveQueue[i].ExtraLen = 0;

        ReceiveIndex[i] = 0;
    }

    //------------------------------------------------------------------------
    //	Init debug values
    //------------------------------------------------------------------------
    DebugOffset = 0;
    DebugSize = 0;
    DebugNames = NULL;
    DebugNameCount = 0;

} /* end of Init */

/***************************************************************************
 * CommBufferClass::Init_Send_Queue -- Clears the send queue               *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.																						*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.																						*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   10/23/1995 BRR : Created.                                             *
 *=========================================================================*/
void CommBufferClass::Init_Send_Queue(void)
{
    int i;

    //------------------------------------------------------------------------
    //	Init data members
    //------------------------------------------------------------------------
    SendCount = 0;

    //------------------------------------------------------------------------
    //	Init the queue entries
    //------------------------------------------------------------------------
    for (i = 0; i < MaxSend; i++) {
        SendQueue[i].IsActive = 0;
        SendQueue[i].IsACK = 0;
        SendQueue[i].FirstTime = 0L;
        SendQueue[i].LastTime = 0L;
        SendQueue[i].SendCount = 0L;
        SendQueue[i].BufLen = 0;
        SendQueue[i].ExtraLen = 0;

        SendIndex[i] = 0;
    }

} /* end of Init_Send_Queue */

/***************************************************************************
 * CommBufferClass::Queue_Send -- queues a message for sending             *
 *                                                                         *
 * INPUT:                                                                  *
 *		buf			buffer containing the message										*
 *		buflen		length of 'buf'														*
 *		extrabuf		buffer containing extra data (optional)						*
 *		extralen		length of extra data (optional)									*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = no room in the queue													*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
int CommBufferClass::Queue_Send(void* buf, int buflen, void* extrabuf, int extralen)
{
    int i;
    int index;

    //------------------------------------------------------------------------
    //	Error if no room in the queue
    //------------------------------------------------------------------------
    if (SendCount == MaxSend || buflen > MaxPacketSize)
        return (0);

    //------------------------------------------------------------------------
    //	Find an empty slot
    //------------------------------------------------------------------------
    index = -1;
    for (i = 0; i < MaxSend; i++) {
        if (SendQueue[i].IsActive == 0) {
            index = i;
            break;
        }
    }
    if (index == -1)
        return (0);

    //------------------------------------------------------------------------
    //	Set entry flags
    //------------------------------------------------------------------------
    SendQueue[index].IsActive = 1;    // entry is now active
    SendQueue[index].IsACK = 0;       // entry hasn't been ACK'd
    SendQueue[index].FirstTime = 0L;  // filled in by Manager when sent
    SendQueue[index].LastTime = 0L;   // filled in by Manager when sent
    SendQueue[index].SendCount = 0L;  // filled in by Manager when sent
    SendQueue[index].BufLen = buflen; // save buffer size

    //------------------------------------------------------------------------
    //	Copy the packet data
    //------------------------------------------------------------------------
    memcpy(SendQueue[index].Buffer, buf, buflen);

    //------------------------------------------------------------------------
    //	Fill in the extra data, if there is any
    //------------------------------------------------------------------------
    if (extrabuf != NULL && extralen > 0 && extralen <= MaxExtraSize) {
        memcpy(SendQueue[index].ExtraBuffer, extrabuf, extralen);
        SendQueue[index].ExtraLen = extralen;
    } else {
        SendQueue[index].ExtraLen = 0;
    }

    //------------------------------------------------------------------------
    //	Save this entry's index
    //------------------------------------------------------------------------
    SendIndex[SendCount] = index;

    //------------------------------------------------------------------------
    //	Increment counters & entry ptr
    //------------------------------------------------------------------------
    SendCount++;
    SendTotal++;

    return (1);

} /* end of Queue_Send */

/***************************************************************************
 * CommBufferClass::UnQueue_Send -- removes next entry from send queue		*
 *                                                                         *
 * Frees the given entry; the index given by the caller is the "active"		*
 * index value (ie the "nth" active entry), not the actual index in the		*
 * array.																						*
 *                                                                         *
 * INPUT:                                                                  *
 *		buf			buffer to store entry's data in; if NULL, it's discarded	*
 *		buflen		filled in with length of entry retrieved						*
 *		index			"index" of entry to un-queue										*
 *		extrabuf		buffer for extra data (optional)									*
 *		extralen		ptr to length of extra data (optional)							*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = no entry to retrieve													*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
int CommBufferClass::UnQueue_Send(void* buf, int* buflen, int index, void* extrabuf, int* extralen)
{
    int i;

    //------------------------------------------------------------------------
    //	Error if no entry to retrieve
    //------------------------------------------------------------------------
    if (SendCount == 0 || SendQueue[SendIndex[index]].IsActive == 0) {
        return (0);
    }

    //------------------------------------------------------------------------
    //	Copy the data from the entry
    //------------------------------------------------------------------------
    if (buf != NULL) {
        memcpy(buf, SendQueue[SendIndex[index]].Buffer, SendQueue[SendIndex[index]].BufLen);
        (*buflen) = SendQueue[SendIndex[index]].BufLen;
    }

    //------------------------------------------------------------------------
    //	Copy the extra data
    //------------------------------------------------------------------------
    if (extrabuf != NULL && extralen != NULL) {
        memcpy(extrabuf, SendQueue[SendIndex[index]].ExtraBuffer, SendQueue[SendIndex[index]].ExtraLen);
        (*extralen) = SendQueue[SendIndex[index]].ExtraLen;
    }

    //------------------------------------------------------------------------
    //	Set entry flags
    //------------------------------------------------------------------------
    SendQueue[SendIndex[index]].IsActive = 0;
    SendQueue[SendIndex[index]].IsACK = 0;
    SendQueue[SendIndex[index]].FirstTime = 0L;
    SendQueue[SendIndex[index]].LastTime = 0L;
    SendQueue[SendIndex[index]].SendCount = 0L;
    SendQueue[SendIndex[index]].BufLen = 0;
    SendQueue[SendIndex[index]].ExtraLen = 0;

    //------------------------------------------------------------------------
    //	Move Indices back one
    //------------------------------------------------------------------------
    for (i = index; i < SendCount - 1; i++) {
        SendIndex[i] = SendIndex[i + 1];
    }
    SendIndex[SendCount - 1] = 0;
    SendCount--;

    return (1);

} /* end of UnQueue_Send */

/***************************************************************************
 * CommBufferClass::Get_Send -- gets ptr to queue entry                    *
 *                                                                         *
 * This routine gets a pointer to the indicated queue entry.  The index		*
 * value is relative to the next-accessible queue entry; 0 = get the			*
 * next available queue entry, 1 = get the one behind that, etc.				*
 *                                                                         *
 * INPUT:                                                                  *
 *		index		index of entry to get (0 = 1st available)							*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		ptr to entry																			*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/21/1994 BR : Created.                                              *
 *=========================================================================*/
SendQueueType* CommBufferClass::Get_Send(int index)
{
    if (SendQueue[SendIndex[index]].IsActive == 0) {
        return (NULL);
    } else {
        return (&SendQueue[SendIndex[index]]);
    }

} /* end of Get_Send */

/***************************************************************************
 * CommBufferClass::Queue_Receive -- queues a received message					*
 *                                                                         *
 * INPUT:                                                                  *
 *		buf			buffer containing the message										*
 *		buflen		length of 'buf'														*
 *		extrabuf		buffer containing extra data (optional)						*
 *		extralen		length of extra data (optional)									*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = no room in the queue													*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
int CommBufferClass::Queue_Receive(void* buf, int buflen, void* extrabuf, int extralen)
{
    int i;
    int index;

    //------------------------------------------------------------------------
    //	Error if no room in the queue
    //------------------------------------------------------------------------
    if (ReceiveCount == MaxReceive || buflen > MaxPacketSize) {
        return (0);
    }

    //------------------------------------------------------------------------
    //	Find an empty slot
    //------------------------------------------------------------------------
    index = -1;
    for (i = 0; i < MaxReceive; i++) {
        if (ReceiveQueue[i].IsActive == 0) {
            index = i;
            break;
        }
    }
    if (index == -1)
        return (0);

    //------------------------------------------------------------------------
    //	Set entry flags
    //------------------------------------------------------------------------
    ReceiveQueue[index].IsActive = 1;
    ReceiveQueue[index].IsRead = 0;
    ReceiveQueue[index].IsACK = 0;
    ReceiveQueue[index].BufLen = buflen;

    //------------------------------------------------------------------------
    //	Copy the packet data
    //------------------------------------------------------------------------
    memcpy(ReceiveQueue[index].Buffer, buf, buflen);

    //------------------------------------------------------------------------
    //	Fill in the extra data, if there is any
    //------------------------------------------------------------------------
    if (extrabuf != NULL && extralen > 0 && extralen <= MaxExtraSize) {
        memcpy(ReceiveQueue[index].ExtraBuffer, extrabuf, extralen);
        ReceiveQueue[index].ExtraLen = extralen;
    } else {
        ReceiveQueue[index].ExtraLen = 0;
    }

    //------------------------------------------------------------------------
    //	Save this entry's index
    //------------------------------------------------------------------------
    ReceiveIndex[ReceiveCount] = index;

    //------------------------------------------------------------------------
    //	Increment counters & entry ptr
    //------------------------------------------------------------------------
    ReceiveCount++;
    ReceiveTotal++;

    return (1);

} /* end of Queue_Receive */

/***************************************************************************
 * CommBufferClass::UnQueue_Receive -- removes next entry from send queue	*
 *                                                                         *
 * Frees the given entry; the index given by the caller is the "active"		*
 * index value (ie the "nth" active entry), not the actual index in the		*
 * array.																						*
 *                                                                         *
 * INPUT:                                                                  *
 *		buf			buffer to store entry's data in; if NULL, it's discarded	*
 *		buflen		filled in with length of entry retrieved						*
 *		index			index of entry to un-queue											*
 *		extrabuf		buffer for extra data (optional)									*
 *		extralen		ptr to length of extra data (optional)							*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = no entry to retrieve													*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
int CommBufferClass::UnQueue_Receive(void* buf, int* buflen, int index, void* extrabuf, int* extralen)
{
    int i;

    //------------------------------------------------------------------------
    //	Error if no entry to retrieve
    //------------------------------------------------------------------------
    if (ReceiveCount == 0 || ReceiveQueue[ReceiveIndex[index]].IsActive == 0) {
        return (0);
    }

    //------------------------------------------------------------------------
    //	Copy the data from the entry
    //------------------------------------------------------------------------
    if (buf != NULL) {
        memcpy(buf, ReceiveQueue[ReceiveIndex[index]].Buffer, ReceiveQueue[ReceiveIndex[index]].BufLen);
        (*buflen) = ReceiveQueue[ReceiveIndex[index]].BufLen;
    }

    //------------------------------------------------------------------------
    //	Copy the extra data
    //------------------------------------------------------------------------
    if (extrabuf != NULL && extralen != NULL) {
        memcpy(extrabuf, ReceiveQueue[ReceiveIndex[index]].ExtraBuffer, ReceiveQueue[ReceiveIndex[index]].ExtraLen);
        (*extralen) = ReceiveQueue[ReceiveIndex[index]].ExtraLen;
    }

    //------------------------------------------------------------------------
    //	Set entry flags
    //------------------------------------------------------------------------
    ReceiveQueue[ReceiveIndex[index]].IsActive = 0;
    ReceiveQueue[ReceiveIndex[index]].IsRead = 0;
    ReceiveQueue[ReceiveIndex[index]].IsACK = 0;
    ReceiveQueue[ReceiveIndex[index]].BufLen = 0;
    ReceiveQueue[ReceiveIndex[index]].ExtraLen = 0;

    //------------------------------------------------------------------------
    //	Move Indices back one
    //------------------------------------------------------------------------
    for (i = index; i < ReceiveCount - 1; i++) {
        ReceiveIndex[i] = ReceiveIndex[i + 1];
    }
    ReceiveIndex[ReceiveCount - 1] = 0;
    ReceiveCount--;

    return (1);

} /* end of UnQueue_Receive */

/***************************************************************************
 * CommBufferClass::Get_Receive -- gets ptr to queue entry                 *
 *                                                                         *
 * This routine gets a pointer to the indicated queue entry.  The index		*
 * value is relative to the next-accessible queue entry; 0 = get the			*
 * next available queue entry, 1 = get the one behind that, etc.				*
 *                                                                         *
 * INPUT:                                                                  *
 *		index		index of entry to get (0 = 1st available)							*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		ptr to entry																			*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/21/1994 BR : Created.                                              *
 *=========================================================================*/
ReceiveQueueType* CommBufferClass::Get_Receive(int index)
{
    if (ReceiveQueue[ReceiveIndex[index]].IsActive == 0) {
        return (NULL);
    } else {
        return (&ReceiveQueue[ReceiveIndex[index]]);
    }

} /* end of Get_Receive */

/***************************************************************************
 * CommBufferClass::Add_Delay -- adds a new delay value for response time  *
 *                                                                         *
 * This routine updates the average response time for this queue.  The		*
 * computation is based on the average of the last 'n' delay values given,	*
 * It computes a running total of the last n delay values, then divides 	*
 * that by n to compute the average.													*
 *																									*
 * When the number of values given exceeds the max, the mean is subtracted	*
 * off the total, then the new value is added in.  Thus, any single delay	*
 * value will have an effect on the total that approaches 0 over time, and	*
 * the new delay value contributes to 1/n of the mean.							*
 *                                                                         *
 * INPUT:                                                                  *
 *		delay			value to add into the response time computation				*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.																						*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   01/19/1995 BR : Created.                                              *
 *=========================================================================*/
void CommBufferClass::Add_Delay(unsigned long delay)
{
    int roundoff = 0;

    if (NumDelay == 256) {
        DelaySum -= MeanDelay;
        DelaySum += delay;
        if ((DelaySum & 0x00ff) > 127)
            roundoff = 1;
        MeanDelay = (DelaySum >> 8) + roundoff;
    } else {
        NumDelay++;
        DelaySum += delay;
        MeanDelay = DelaySum / NumDelay;
    }

    if (delay > MaxDelay) {
        MaxDelay = delay;
    }

} /* end of Add_Delay */

/***************************************************************************
 * CommBufferClass::Avg_Response_Time -- returns average response time    	*
 *                                                                         *
 * INPUT:                                                                  *
 *		none.																						*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		latest computed average response time											*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   01/19/1995 BR : Created.                                              *
 *=========================================================================*/
unsigned long CommBufferClass::Avg_Response_Time(void)
{
    return (MeanDelay);

} /* end of Avg_Response_Time */

/***************************************************************************
 * CommBufferClass::Max_Response_Time -- returns max response time    		*
 *                                                                         *
 * INPUT:                                                                  *
 *		none.																						*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		latest computed average response time											*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   01/19/1995 BR : Created.                                              *
 *=========================================================================*/
unsigned long CommBufferClass::Max_Response_Time(void)
{
    return (MaxDelay);

} /* end of Max_Response_Time */

/***************************************************************************
 * CommBufferClass::Reset_Response_Time -- resets computations					*
 *                                                                         *
 * INPUT:                                                                  *
 *		none.																						*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.																						*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   01/19/1995 BR : Created.                                              *
 *=========================================================================*/
void CommBufferClass::Reset_Response_Time(void)
{
    DelaySum = 0L;
    NumDelay = 0L;
    MeanDelay = 0L;
    MaxDelay = 0L;

} /* end of Reset_Response_Time */

/***************************************************************************
 * CommBufferClass::Configure_Debug -- sets up special debug values        *
 *                                                                         *
 * Mono_Debug_Print2() can look into a packet to pull out a particular		*
 * ID, and can print both that ID and a string corresponding to				*
 * that ID.  This routine configures these values so it can find				*
 * and decode the ID.  This ID is used in addition to the normal				*
 * CommHeaderType values.																	*
 *                                                                         *
 * INPUT:                                                                  *
 *		type_offset		ID's byte offset into packet									*
 *		type_size		size of ID, in bytes; 0 if none								*
 *		names				ptr to array of names; use ID as an index into this	*
 *		maxnames			max # in the names array; 0 if none.						*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.																						*
 *                                                                         *
 * WARNINGS:                                                               *
 *		Names shouldn't be longer than 12 characters.								*
 *                                                                         *
 * HISTORY:                                                                *
 *   05/31/1995 BRR : Created.                                             *
 *=========================================================================*/
void CommBufferClass::Configure_Debug(int type_offset, int type_size, const char** names, int namestart, int namecount)
{
    DebugOffset = type_offset;
    DebugSize = type_size;
    DebugNames = names;
    DebugNameStart = namestart;
    DebugNameCount = namecount;

} /* end of Configure_Debug */

/***************************************************************************
 * Mono_Debug_Print -- Debug output routine                                *
 *                                                                         *
 * This routine leaves 5 lines at the top for the caller's use.				*
 *                                                                         *
 * INPUT:                                                                  *
 *		refresh		1 = clear screen & completely refresh							*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.																						*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   05/02/1995 BRR : Created.                                             *
 *=========================================================================*/
void CommBufferClass::Mono_Debug_Print(int refresh)
{
#ifdef WWLIB32_H
    int i;                                   // loop counter
    static int send_col[] = {1, 14, 28};     // coords of send queue columns
    static int receive_col[] = {40, 54, 68}; // coords of recv queue columns
    int row, col;                            // current row,col for printing
    int num;                                 // max # items to print

    struct CommHdr
    { // this mirrors the CommHeaderType
        unsigned short MagicNumber;
        unsigned char Code;
        unsigned long PacketID;
    } * hdr;

    //------------------------------------------------------------------------
    //	If few enough entries, call the verbose debug version
    //------------------------------------------------------------------------
    if (MaxSend <= 16) {
        Mono_Debug_Print2(refresh);
        return;
    }

    //------------------------------------------------------------------------
    //	Refresh the screen
    //------------------------------------------------------------------------
    if (refresh) {
        Mono_Clear_Screen();
        Mono_Printf("�����������������������������������������������������������������������������Ŀ\n");
        Mono_Printf("�                                                                             �\n");
        Mono_Printf("�                                                                             �\n");
        Mono_Printf("�                                                                             �\n");
        Mono_Printf("�����������������������������������������������������������������������������Ĵ\n");
        Mono_Printf("�              Send Queue              �             Receive Queue            �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("� ID  Ct ACK   ID  Ct ACK    ID  Ct ACK� ID  Rd ACK    ID  Rd ACK   ID  Rd ACK�\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�������������������������������������������������������������������������������");
    }

    //------------------------------------------------------------------------
    //	Print Send Queue items
    //------------------------------------------------------------------------
    if (MaxSend <= 48) {
        num = MaxSend;
    } else {
        num = 48;
    }
    col = 0;
    row = 0;
    for (i = 0; i < MaxSend; i++) {
        Mono_Set_Cursor(send_col[col], row + 8);
        if (SendQueue[i].IsActive) {
            hdr = (CommHdr*)SendQueue[i].Buffer;
            hdr->MagicNumber = hdr->MagicNumber;
            hdr->Code = hdr->Code;
            Mono_Printf("%4d %2d  %d", hdr->PacketID, SendQueue[i].SendCount, SendQueue[i].IsACK);
        } else {
            Mono_Printf("____ __  _ ");
        }

        row++;
        if (row > 15) {
            row = 0;
            col++;
        }
    }

    //------------------------------------------------------------------------
    //	Print Receive Queue items
    //------------------------------------------------------------------------
    if (MaxReceive <= 48) {
        num = MaxSend;
    } else {
        num = 48;
    }
    col = 0;
    row = 0;
    for (i = 0; i < MaxReceive; i++) {
        Mono_Set_Cursor(receive_col[col], row + 8);
        if (ReceiveQueue[i].IsActive) {
            hdr = (CommHdr*)ReceiveQueue[i].Buffer;
            Mono_Printf("%4d  %d  %d", hdr->PacketID, ReceiveQueue[i].IsRead, ReceiveQueue[i].IsACK);
        } else {
            Mono_Printf("____  _  _ ");
        }

        row++;
        if (row > 15) {
            row = 0;
            col++;
        }
    }

#else
    refresh = refresh;
#endif
} /* end of Mono_Debug_Print */

/***************************************************************************
 * CommBufferClass::Mono_Debug_Print2 -- Debug output; alternate format    *
 *                                                                         *
 * This routine prints more information than the other version; it's			*
 * called only if the number of queue entries is small enough to support	*
 * this format.																				*
 *                                                                         *
 * INPUT:                                                                  *
 *		refresh		1 = clear screen & completely refresh							*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.																						*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   05/31/1995 BRR : Created.                                             *
 *=========================================================================*/
void CommBufferClass::Mono_Debug_Print2(int refresh)
{
#ifdef WWLIB32_H
    int i; // loop counter
    char txt[80];
    int val;

    struct CommHdr
    { // this mirrors the CommHeaderType
        unsigned short MagicNumber;
        unsigned char Code;
        unsigned long PacketID;
    } * hdr;

    //------------------------------------------------------------------------
    //	Refresh the screen
    //------------------------------------------------------------------------
    if (refresh) {
        Mono_Clear_Screen();
        Mono_Printf("�����������������������������������������������������������������������������Ŀ\n");
        Mono_Printf("�                                                                             �\n");
        Mono_Printf("�                                                                             �\n");
        Mono_Printf("�                                                                             �\n");
        Mono_Printf("�����������������������������������������������������������������������������Ĵ\n");
        Mono_Printf("�              Send Queue              �             Receive Queue            �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("� ID  Ct Type   Data  Name         ACK � ID  Rd Type   Data  Name         ACK �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�                                      �                                      �\n");
        Mono_Printf("�������������������������������������������������������������������������������");
    }

    //------------------------------------------------------------------------
    //	Print Send Queue items
    //------------------------------------------------------------------------
    for (i = 0; i < MaxSend; i++) {
        Mono_Set_Cursor(1, 8 + i);

        //.....................................................................
        //	Print an active entry
        //.....................................................................
        if (SendQueue[i].IsActive) {

            //..................................................................
            //	Get header info
            //..................................................................
            hdr = (CommHdr*)SendQueue[i].Buffer;
            hdr->MagicNumber = hdr->MagicNumber;
            hdr->Code = hdr->Code;
            sprintf(
                txt, "%4d %2d %-5s  ", hdr->PacketID, SendQueue[i].SendCount, ConnectionClass::Command_Name(hdr->Code));

            //..................................................................
            //	Decode app's ID & its name
            //..................................................................
            if (DebugSize && (DebugOffset + DebugSize) <= SendQueue[i].BufLen) {
                if (DebugSize == 1) {
                    val = *(SendQueue[i].Buffer + DebugOffset);

                } else if (DebugSize == 2) {
                    val = *((short*)(SendQueue[i].Buffer + DebugOffset));

                } else if (DebugSize == 4) {
                    val = *((int*)(SendQueue[i].Buffer + DebugOffset));
                }
                sprintf(txt + strlen(txt), "%4d  ", val);

                if (DebugNameCount > 0 && val >= 0 && val < DebugNameCount) {
                    sprintf(txt + strlen(txt), "%-12s  %x", DebugNames[val - DebugNameStart], SendQueue[i].IsACK);
                } else {
                    sprintf(txt + strlen(txt), "              %x", SendQueue[i].IsACK);
                }
            } else {
                sprintf(txt + strlen(txt), "                    %x", SendQueue[i].IsACK);
            }

            Mono_Printf("%s", txt);
        } else {

            //..................................................................
            //	Entry isn't active; print blanks
            //..................................................................
            Mono_Printf("____ __                            _");
        }
    }

    //------------------------------------------------------------------------
    //	Print Receive Queue items
    //------------------------------------------------------------------------
    for (i = 0; i < MaxReceive; i++) {
        Mono_Set_Cursor(40, 8 + i);

        //.....................................................................
        //	Print an active entry
        //.....................................................................
        if (ReceiveQueue[i].IsActive) {

            //..................................................................
            //	Get header info
            //..................................................................
            hdr = (CommHdr*)ReceiveQueue[i].Buffer;
            hdr->MagicNumber = hdr->MagicNumber;
            hdr->Code = hdr->Code;
            sprintf(
                txt, "%4d %2d %-5s  ", hdr->PacketID, ReceiveQueue[i].IsRead, ConnectionClass::Command_Name(hdr->Code));

            //..................................................................
            //	Decode app's ID & its name
            //..................................................................
            if (DebugSize && (DebugOffset + DebugSize) <= ReceiveQueue[i].BufLen) {
                if (DebugSize == 1) {
                    val = *(ReceiveQueue[i].Buffer + DebugOffset);

                } else if (DebugSize == 2) {
                    val = *((short*)(ReceiveQueue[i].Buffer + DebugOffset));

                } else if (DebugSize == 4) {
                    val = *((int*)(ReceiveQueue[i].Buffer + DebugOffset));
                }
                sprintf(txt + strlen(txt), "%4d  ", val);

                if (DebugNameCount > 0 && val >= 0 && val < DebugNameCount) {
                    sprintf(txt + strlen(txt), "%-12s  %x", DebugNames[val - DebugNameStart], ReceiveQueue[i].IsACK);
                } else {
                    sprintf(txt + strlen(txt), "              %x", ReceiveQueue[i].IsACK);
                }
            } else {
                sprintf(txt + strlen(txt), "                    %x", ReceiveQueue[i].IsACK);
            }

            Mono_Printf("%s", txt);
        } else {

            //..................................................................
            //	Entry isn't active; print blanks
            //..................................................................
            Mono_Printf("____ __                            _");
        }
    }

#else
    refresh = refresh;
#endif
} /* end of Mono_Debug_Print2 */

/************************** end of combuf.cpp ******************************/
