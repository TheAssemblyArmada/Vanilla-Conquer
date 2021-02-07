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
#ifndef RELIABLE_H
#define RELIABLE_H

#include "common/combuf.h"
#include "protocol.h"
#include "common/sockets.h"

class ListenerClass;

class ReliableProtocolClass : public ProtocolClass
{
public:
    ReliableProtocolClass();

    virtual ~ReliableProtocolClass() {}

    virtual void Data_Received() override {}
    virtual void Connected_To_Server(int status) override
    {
        if (status == 1) {
            Connected = 1;
        } else {
            Connected = -1;
        }
    }
    virtual void Connection_Requested() override {}
    virtual void Closed() override { Connected = 0; }
    virtual void Name_Resolved() override { NameResolved = 1; }

    int Connected;
    bool NameResolved;
};

struct TCPHost
{
    int Addr;
    char DotAddr[16];
    char Name[255];
    short Port;
};

class ReliableCommClass
{
public:
    ReliableCommClass(int maxpacketsize);
    ReliableCommClass(ProtocolClass *protocol, int maxpacketsize);

    virtual ~ReliableCommClass();
    virtual void Register(ProtocolClass *protocol);
    virtual bool Connect(void *, int, int);
    virtual int Connect(ListenerClass *listener);
    virtual void Disconnect();
    virtual int Send();

private:
    static unsigned short Network_Short(unsigned short);
    static unsigned short Local_Short(unsigned short);
    static unsigned int Network_Long(unsigned int);
    static unsigned int Local_Long(unsigned int);
    void Create_Window();
    void Destroy_Window();
    int Open_Socket();
    void Close_Socket();
#ifdef _WIN32
    static LRESULT __stdcall Window_Proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
#endif

public:
    TCPHost Host;
    int MaxPacketSize;
    SOCKET Socket;
    ProtocolClass *Protocol;
#ifdef _WIN32
    HWND Window;
    HANDLE Async;
#endif
    char Hbuf[1024];
    SendQueueType* SendEntry;
    int SendLen;
    char *ReceiveBuf;
    int ReceiveLen;
    bool IsConnected;
};

#endif
