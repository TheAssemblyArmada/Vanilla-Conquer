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
#ifndef LISTENER_H
#define LISTENER_H

#include "protocol.h"
#include "common/sockets.h"

struct DestAddress
{
    char Host[30];
    unsigned short Port;
};

class ListenerProtocolClass : public ProtocolClass
{
public:
    ListenerProtocolClass();
    virtual ~ListenerProtocolClass();

    virtual void Data_Received() override {}
    virtual void Connected_To_Server(int status) override {}
    virtual void Connection_Requested() override;
    virtual void Closed() override {}
    virtual void Name_Resolved() override {}

private:
    unsigned ConnectionRequests;
};

class ListenerClass
{
public:
    ListenerClass();
    ListenerClass(ProtocolClass *protocol);
    virtual ~ListenerClass();

    virtual void Register(ProtocolClass *protocol);
    virtual int Start_Listening(void *address, int size, int unk);
    virtual void Stop_Listening();

private:
    void Create_Window();
    void Destroy_Window();
    int Open_Socket(unsigned short port);
    void Close_Socket();
#ifdef _WIN32
    static LRESULT __stdcall Window_Proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);
#endif

public:
    SOCKET Socket;
    ProtocolClass *Protocol;
#ifdef _WIN32
    HWND Window;
#endif
    bool IsListening;
};

int Init_Listener();
void Deinit_Listener();

#endif
