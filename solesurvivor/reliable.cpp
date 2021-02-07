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
#include "function.h"
#include "reliable.h"
#include "listener.h"
#include "common/debugstring.h"
#include "common/endianness.h"
#include "soleglobals.h"
#include <algorithm>

#define SSWM_RELIABLE_COMM_MSG (WM_USER + 0x67)
#define SSWM_HOSTBYADDR_MSG (WM_USER + 0x65)
#define SSWM_HOSTBYNAME_MSG (WM_USER + 0x66)

ReliableProtocolClass::ReliableProtocolClass() : Connected(0), NameResolved(0)
{
    if (GameToPlay == GAME_SERVER) {
        Queue = new CommBufferClass(200, 200, 422, 32);
    } else {
        Queue = new CommBufferClass(200, 600, 422, 32);
    }
}

ReliableCommClass::ReliableCommClass(int maxpacketsize)
{
    Socket = -1;
    Protocol = 0;
#ifdef _WIN32
    Window = 0;
    Async = 0;
#endif
    Host.Addr = -1;
    Host.DotAddr[0] = 0;
    Host.Name[0] = 0;
    Host.Port = 0;
    SendEntry = 0;
    SendLen = 0;
    ReceiveBuf = 0;
    ReceiveLen = 0;
    IsConnected = false;
    MaxPacketSize = maxpacketsize;
    ReceiveBuf = new char[maxpacketsize];
    Create_Window();
}

ReliableCommClass::ReliableCommClass(ProtocolClass *protocol, int maxpacketsize)
{
    Socket = -1;
    Protocol = 0;
#ifdef _WIN32
    Window = 0;
    Async = 0;
#endif
    Host.Addr = -1;
    Host.DotAddr[0] = 0;
    Host.Name[0] = 0;
    Host.Port = 0;
    SendEntry = 0;
    SendLen = 0;
    ReceiveBuf = 0;
    ReceiveLen = 0;
    IsConnected = false;
    MaxPacketSize = maxpacketsize;
    ReceiveBuf = new char[maxpacketsize];

    Create_Window();

    Register(protocol);
}

ReliableCommClass::~ReliableCommClass()
{
    if (IsConnected) {
        Disconnect();
    }

    Destroy_Window();
    delete[] ReceiveBuf;
}

void ReliableCommClass::Register(ProtocolClass *protocol)
{
    Protocol = protocol;
}

bool ReliableCommClass::Connect(void *addr, int len, int type)
{
    DestAddress *_addr = (DestAddress *)addr;

    if (Protocol != nullptr) {
        return false;
    }

    if (len != sizeof(DestAddress)) {
        return false;
    }

    if (IsConnected & 1) {
        Disconnect();
    }

    if (!Open_Socket()) {
        return false;
    }

#ifdef _WIN32
    // Handle most network events.
    if (WSAAsyncSelect(Socket, Window, SSWM_RELIABLE_COMM_MSG, FD_READ | FD_WRITE | FD_CONNECT | FD_CLOSE) == SOCKET_ERROR) {
        Close_Socket();
        return false;
    }

    if (type == 0) // Sole 1.0 has this as UnreliableCommClass::Resolve_Address
    {
        Host.Port = _addr->Port;
        Host.Addr = inet_addr(_addr->Host);
        if (Host.Addr != -1) {
            strcpy(Host.DotAddr, _addr->Host);
            Host.Name[0] = 0;
            Async = WSAAsyncGetHostByAddr(
                Window, SSWM_HOSTBYADDR_MSG, (char *)&Host.Addr, sizeof(Host.Addr), PF_INET, Hbuf, sizeof(Hbuf));
            if (!Async) {
                WSAAsyncSelect(Socket, Window, SSWM_RELIABLE_COMM_MSG, 0);
                Close_Socket();
                return false;
            }

            sockaddr_in name;
            name.sin_family = 2;
            name.sin_port = htons(Host.Port);
            name.sin_addr.s_addr = Host.Addr;
            if (connect(Socket, (sockaddr*)&name, sizeof(name)) == SOCKET_ERROR && LastSocketError != WSAEWOULDBLOCK) {
                WSACancelAsyncRequest(Async);
                Async = 0;
                WSAAsyncSelect(Socket, Window, SSWM_RELIABLE_COMM_MSG, 0);
                Close_Socket();
                return false;
            }
        } else {
            strcpy(Host.Name, _addr->Host);
            Async = WSAAsyncGetHostByName(Window, SSWM_HOSTBYNAME_MSG, Host.Name, Hbuf, sizeof(Hbuf));

            if (!Async) {
                WSAAsyncSelect(Socket, Window, SSWM_RELIABLE_COMM_MSG, 0);
                Close_Socket();
                return false;
            }
        }

        IsConnected = true;
        return true;
    }

    WSAAsyncSelect(Socket, Window, SSWM_RELIABLE_COMM_MSG, 0);
#endif
    Close_Socket();
    return false;
}

int ReliableCommClass::Connect(ListenerClass *listener)
{
    if (Protocol == nullptr) {
        return false;
    }

    if (IsConnected) {
        Disconnect();
    }

    int addrlen = 16;
    sockaddr_in addr;
    Socket = accept(listener->Socket, (sockaddr *)&addr, &addrlen);

    if (Socket == -1) {
        return false;
    }

#ifdef _WIN32
    if (WSAAsyncSelect(Socket, Window, SSWM_RELIABLE_COMM_MSG, FD_READ | FD_WRITE | FD_CONNECT | FD_CLOSE) == SOCKET_ERROR) {
        Close_Socket();
        return false;
    }

    Host.Addr = addr.sin_addr.s_addr;
    strcpy(Host.DotAddr, inet_ntoa(addr.sin_addr));
    Host.Port = ntohs(addr.sin_port);
    Host.Name[0] = 0;

    Async = WSAAsyncGetHostByAddr(Window, SSWM_HOSTBYADDR_MSG, (char *)&Host.Addr, sizeof(Host.Addr), PF_INET, Hbuf, 1024);

    if (!Async) {
        WSAAsyncSelect(Socket, Window, SSWM_RELIABLE_COMM_MSG, 0);
        Close_Socket();
        return false;
    }
#endif
    IsConnected = true;
    return true;
}

void ReliableCommClass::Disconnect()
{
#ifdef _WIN32
    if (IsConnected) {
        if (Async) {
            WSACancelAsyncRequest(Async);
            Async = 0;
        }

        WSAAsyncSelect(Socket, Window, SSWM_RELIABLE_COMM_MSG, 0);
        Close_Socket();
        Host.Name[0] = 0;
        Host.DotAddr[0] = 0;
        Host.Addr = -1;
        Host.Port = 0;
        SendEntry = 0;
        SendLen = 0;
        IsConnected = false;
    }
#endif
}

int ReliableCommClass::Send()
{
    uint16_t *buff;

    if (Protocol == nullptr || !IsConnected) {
        return false;
    }

    if (SendEntry != nullptr) {
        return true;
    }

    SendEntry = Protocol->Queue->Get_Send(0);

    if (SendEntry) {
        SendLen = SendEntry->BufLen;
        buff = (uint16_t *)SendEntry->Buffer;
        *buff = htons(SendEntry->BufLen);
#ifdef _WIN32
        PostMessageA(Window, SSWM_RELIABLE_COMM_MSG, 0, 2);
#endif
        return true;
    }

    return false;
}

unsigned short ReliableCommClass::Network_Short(unsigned short local_val)
{
    return hton16(local_val);
}

unsigned short ReliableCommClass::Local_Short(unsigned short net_val)
{
    return ntoh16(net_val);
}

unsigned int ReliableCommClass::Network_Long(unsigned int local_val)
{
    return hton32(local_val);
}

unsigned int ReliableCommClass::Local_Long(unsigned int net_val)
{
    return ntoh32(net_val);
}

void ReliableCommClass::Create_Window()
{
#ifdef _WIN32
    WNDCLASSA WndClass;

    WndClass.style = 3;
    WndClass.lpfnWndProc = ReliableCommClass::Window_Proc;
    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = 0;
    WndClass.hInstance = hWSockInstance;
    WndClass.hIcon = LoadIconA(NULL, (LPCSTR)0x7F00);
    WndClass.hCursor = LoadCursorA(NULL, (LPCSTR)0x7F00);
    WndClass.hbrBackground = (HBRUSH)GetStockObject(0);
    WndClass.lpszMenuName = 0;
    WndClass.lpszClassName = "RCommWin";
    RegisterClassA(&WndClass);

    Window = CreateWindowExA(0,
        "RCommWin",
        "Data Transmission",
        0x80000,
        0x80000000,
        0x80000000,
        0x80000000,
        0x80000000,
        NULL,
        NULL,
        hWSockInstance,
        NULL);

    ShowWindow(Window, 0);
    UpdateWindow(Window);
    SetWindowLongPtrA(Window, GWLP_USERDATA, (LONG_PTR)this);
#endif
}

void ReliableCommClass::Destroy_Window()
{
#ifdef _WIN32
    if (Window) {
        DestroyWindow(Window);
        Window = 0;
    }
#endif
}

int ReliableCommClass::Open_Socket()
{
    Socket = socket(AF_INET, SOCK_STREAM, 0);

    if (Socket == INVALID_SOCKET) {
        return false;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = htonl(0);

    if (bind(Socket, (sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        Close_Socket();
        return false;
    }

    return true;
}

void ReliableCommClass::Close_Socket()
{
    struct linger optval;
    if (Socket != -1) {
        optval.l_linger = 0;
        optval.l_onoff = 0;
        setsockopt(Socket, SOL_SOCKET, SO_LINGER, (const char *)&optval, sizeof(optval));
        closesocket(Socket);
        Socket = -1;
    }
}

#ifdef _WIN32
LRESULT __stdcall ReliableCommClass::Window_Proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    static const char _http_response[] =
        "HTTP/1.0 200 OK\nServer: NealScape/1.0\nContent-Type: text/html\n\n<title>Sole Survivor</title><h1>Sole Survivor "
        "is "
        "alive.</h1>HTTP Responder.";

    ReliableCommClass *thisptr = reinterpret_cast<ReliableCommClass *>(GetWindowLongPtrA(hwnd, -21));

    switch (message) {
        // Fallthrough on all these events we are going to ignore.
        case WM_CREATE:
        case WM_PAINT:
        case WM_COMMAND:
            return 0;
        case WM_DESTROY:
            if (thisptr->IsConnected) {
                thisptr->Disconnect();
            }
            return 0;
        case SSWM_HOSTBYADDR_MSG:
            DBG_INFO("ReliableComms resolved a host by address.");
            if (WSAGETSELECTERROR(lparam) != 0) {
                thisptr->Host.Name[0] = '\0';
            } else {
                strcpy(thisptr->Host.Name, thisptr->Hbuf);

                if (thisptr->Protocol != nullptr) {
                    thisptr->Protocol->Name_Resolved();
                }
            }

            return 0;
        case SSWM_HOSTBYNAME_MSG:
            DBG_INFO("ReliableComms resolved a host by name.");
            if (WSAGETSELECTERROR(lparam) != 0) {
                if (thisptr->Protocol != nullptr) {
                    thisptr->Protocol->Connected_To_Server(0);
                }

                thisptr->Disconnect();
            } else {
                struct hostent *host = reinterpret_cast<struct hostent *>(thisptr->Hbuf);
                // Copy first address out of the list.
                memcpy(&thisptr->Host.Addr, *host->h_addr_list, sizeof(thisptr->Host.Addr));
                struct sockaddr_in addr;
                addr.sin_family = AF_INET;
                addr.sin_port = htons(thisptr->Host.Port);
                addr.sin_addr.s_addr = thisptr->Host.Addr;
                strcpy(thisptr->Host.DotAddr, inet_ntoa(addr.sin_addr));

                if (connect(thisptr->Socket, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR
                    && LastSocketError != WSAEWOULDBLOCK) {
                    if (thisptr->Protocol != nullptr) {
                        thisptr->Protocol->Connected_To_Server(0);
                    }

                    thisptr->Disconnect();
                } else {
                    thisptr->Async = 0;
                }
            }

            return 0;

        case SSWM_RELIABLE_COMM_MSG:
            DBG_INFO("ReliableComms handling a packet.");
            switch (WSAGETSELECTEVENT(lparam)) {
                case FD_READ:
                    if (WSAGETSELECTERROR(lparam) != 0 && WSAGETSELECTERROR(lparam) != WSAEWOULDBLOCK) {
                        if (thisptr->Protocol != nullptr) {
                            thisptr->Protocol->Closed();
                        }

                        thisptr->Disconnect();

                        return 0;
                    }

                    while (true) {
                        bool get_request = false;
                        int received = 0;
                        unsigned short packet_size = 0;

                        // If we are yet to receive at least 2 bytes, then we need to try and get the first 2 bytes.
                        if (thisptr->ReceiveLen < 2) {
                            received = recv(
                                thisptr->Socket, &thisptr->ReceiveBuf[thisptr->ReceiveLen], 2 - thisptr->ReceiveLen, 0);

                            if (received == SOCKET_ERROR) {
                                if (LastSocketError != WSAEWOULDBLOCK) {
                                    if (thisptr->Protocol != nullptr) {
                                        thisptr->Protocol->Connected_To_Server(0);
                                    }

                                    thisptr->Disconnect();
                                    break;
                                }

                                received = 0;
                            }

                            thisptr->ReceiveLen += received;

                            // If we still haven't received at least 2 bytes just return as we are done for now.
                            if (thisptr->ReceiveLen < 2) {
                                break;
                            }

                            // If we get GE in the buffer assume this is a get http request.
                            if (strncmp(thisptr->ReceiveBuf, "GE", sizeof(unsigned short)) == 0) {
                                get_request = true;
                            } else {
                                packet_size = ntohs(*((unsigned short *)thisptr->ReceiveBuf));

                                if (packet_size > thisptr->MaxPacketSize) {
                                    packet_size = thisptr->MaxPacketSize;
                                }

                                *((unsigned short *)thisptr->ReceiveBuf) = min((int)packet_size, thisptr->MaxPacketSize);
                                get_request = false;
                            }
                        }

                        if (thisptr->ReceiveLen >= 2) {
                            // Already converted from network byte order in last section.
                            packet_size = *((unsigned short *)thisptr->ReceiveBuf);

                            // Check if we have all the packet yet.
                            if (packet_size > thisptr->ReceiveLen) {
                                // If its a get request, make a response. Otherwise handle as game packet.
                                if (get_request) {
                                    // Discard the rest of the packet.
                                    while (recv(thisptr->Socket, thisptr->ReceiveBuf, 10, 0) > 0) {
                                    }

                                    send(thisptr->Socket, _http_response, strlen(_http_response) + 1, 0);
                                    thisptr->Disconnect();
                                    break;
                                } else {
                                    received = recv(thisptr->Socket,
                                    &thisptr->ReceiveBuf[thisptr->ReceiveLen],
                                    packet_size - thisptr->ReceiveLen,
                                    0);

                                    if (received == SOCKET_ERROR) {
                                        if (LastSocketError != WSAEWOULDBLOCK) {
                                            if (thisptr->Protocol != nullptr) {
                                                thisptr->Protocol->Connected_To_Server(0);
                                            }

                                            thisptr->Disconnect();
                                            break;
                                        }

                                        received = 0;
                                    }
                                }

                                thisptr->ReceiveLen += received;
                            }
                        }

                        // Have we got all of the packet yet? If not return 0.
                        if (packet_size != thisptr->ReceiveLen) {
                            break;
                        }

                        if (thisptr->Protocol != nullptr) {
                            if (thisptr->Protocol->Queue->Num_Receive() == thisptr->Protocol->Queue->Max_Receive()) {
                                thisptr->Protocol->Queue->Grow_Receive(50);
                            }

                            received = thisptr->Protocol->Queue->Queue_Receive(
                                thisptr->ReceiveBuf, thisptr->ReceiveLen, nullptr, 0);

                            if (received != 0) {
                                thisptr->Protocol->Data_Received();
                            }
                        }

                        thisptr->ReceiveLen = 0;

                        if (thisptr->Protocol == nullptr) {
                            break;
                        }
                    }

                    return 0;
                case FD_WRITE: {
                    // If we don't have a send entry, then assume nothing to send?
                    if (thisptr->SendEntry == nullptr) {
                        return 0;
                    }

                    if (WSAGETSELECTERROR(lparam) != 0) {
                        if (thisptr->Protocol != nullptr) {
                            thisptr->Protocol->Closed();
                        }

                        thisptr->Disconnect();

                        return 0;
                    }

                    unsigned short packet_size = ntohs(*((unsigned short *)thisptr->SendEntry->Buffer));

                    while (thisptr->SendLen > 0) {
                        int sent = send(thisptr->Socket, (const char *)&thisptr->SendEntry->Buffer[packet_size - thisptr->SendLen], thisptr->SendLen, 0);
                        
                        if (sent == SOCKET_ERROR) {
                            if (LastSocketError != WSAEWOULDBLOCK) {
                                if (thisptr->Protocol != nullptr) {
                                    thisptr->Protocol->Closed();
                                }

                                thisptr->Disconnect();
                                return 0;
                            }

                            break;
                        }

                        thisptr->SendLen -= sent;
                    }

                    if (thisptr->SendLen == 0) {
                        if (thisptr->Protocol != nullptr) {
                            thisptr->Protocol->Queue->UnQueue_Send(nullptr, nullptr, 0, nullptr, nullptr);
                        }

                        thisptr->SendEntry = nullptr;
                    }
                }

                    return 0;
                case FD_CONNECT:
                    if (WSAGETSELECTERROR(lparam) != 0) {
                        if (thisptr->Protocol != nullptr) {
                            thisptr->Protocol->Connected_To_Server(0);
                        }

                        thisptr->Disconnect();
                    } else {
                        if (thisptr->Protocol != nullptr) {
                            thisptr->Protocol->Connected_To_Server(1);

                            if (strlen(thisptr->Host.Name) != 0) {
                                thisptr->Protocol->Name_Resolved();
                            }
                        }
                    }

                    return 0;
                case FD_CLOSE:
                    if (thisptr->Protocol != nullptr) {
                        thisptr->Protocol->Closed();
                    }

                    thisptr->Disconnect();
                    return 0;

                default:
                    break;
            }

            return 0;
        default:
            break;
    }

    // All other events get passed to the next handler.
    return DefWindowProcA(hwnd, message, wparam, lparam);
}
#endif
