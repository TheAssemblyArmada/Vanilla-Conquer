/**
 * @file
 *
 * @author OmniBlade
 *
 * @brief Interface for protocol class implementations.
 *
 * @copyright Remnant is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            2 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#include "listener.h"
#include "common/combuf.h"
#include "protocol.h"
#include "common/debugstring.h"
#include "common/internet.h"
#include "soleglobals.h"

#ifdef _WIN32
#define SSWM_LISTENER_MSG (WM_USER + 0x65)
#endif

ListenerProtocolClass::ListenerProtocolClass() : ConnectionRequests(0)
{
    Queue = new CommBufferClass(100, 100, 422, 32);
}

ListenerProtocolClass::~ListenerProtocolClass()
{
    delete Queue;
}

void ListenerProtocolClass::Connection_Requested()
{
    // TODO This is called on FD_ACCEPT events, I believe it should also signal ReliableCommClass to accept the connection.
    ++ConnectionRequests;
}

ListenerClass::ListenerClass() :
    Socket(INVALID_SOCKET),
    Protocol(nullptr),
#ifdef _WIN32
    Window(),
#endif
    IsListening(false)
{
    Create_Window();
}

ListenerClass::ListenerClass(ProtocolClass *protocol) :
    Socket(INVALID_SOCKET),
    Protocol(nullptr),
#ifdef _WIN32
    Window(),
#endif
    IsListening(false)
{
    Create_Window();
    Register(protocol);
}

ListenerClass::~ListenerClass()
{
    if (IsListening) {
        Stop_Listening();
    }

    Destroy_Window();
}

void ListenerClass::Register(ProtocolClass *protocol)
{
    Protocol = protocol;
}

int ListenerClass::Start_Listening(void *address, int size, int unk)
{
    // Check we are in a state capable of listening and have what we need.
    if (Protocol == nullptr || size != sizeof(DestAddress) || IsListening || unk != 0) {
        return 0;
    }

#ifdef _WIN32
    // Open our listening socket.
    if (!Open_Socket(static_cast<DestAddress *>(address)->Port)) {
        return 0;
    }

    // Start listening for FD_ACCEPT events.
    if (listen(Socket, 3) == SOCKET_ERROR || WSAAsyncSelect(Socket, Window, SSWM_LISTENER_MSG, FD_ACCEPT) == SOCKET_ERROR) {
        Close_Socket();
        return 0;
    }
#endif
    IsListening = true;

    return 1;
}

void ListenerClass::Stop_Listening()
{
    // If we aren't listening, then there is nothing to do.
    if (IsListening) {
        // Set to not be notified of any socket events, then close socket.
#ifdef _WIN32
        WSAAsyncSelect(Socket, Window, SSWM_LISTENER_MSG, 0);
#endif
        Close_Socket();
        IsListening = false;
    }
}

void ListenerClass::Create_Window()
{
#ifdef _WIN32
    WNDCLASSA wndclass;

    wndclass.style = 3;
    wndclass.lpfnWndProc = Window_Proc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hWSockInstance;
    wndclass.hIcon = LoadIconA(0, MAKEINTRESOURCEA(0x7F00));
    wndclass.hCursor = LoadCursorA(0, MAKEINTRESOURCEA(0x7F00));
    wndclass.hbrBackground = (HBRUSH)GetStockObject(0);
    wndclass.lpszMenuName = 0;
    wndclass.lpszClassName = "ListenWin";
    RegisterClassA(&wndclass);

    Window = CreateWindowExA(0,
        "ListenWin",
        "Listening for Connections",
        0x80000u,
        0x80000000,
        0x80000000,
        0x80000000,
        0x80000000,
        0,
        0,
        hWSockInstance,
        0);

    ShowWindow(Window, 0);
    UpdateWindow(Window);
    SetWindowLongPtrA(Window, GWLP_USERDATA, (LONG_PTR)this);
#endif
}

void ListenerClass::Destroy_Window()
{
#ifdef _WIN32
    if (Window != nullptr) {
        DestroyWindow(Window);
        Window = nullptr;
    }
#endif
}

int ListenerClass::Open_Socket(unsigned short port)
{
    // Create a TCP/IP socket.
    Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

    if (Socket == INVALID_SOCKET) {
        return 0;
    }

    // Set port for this socket to listen to a bind it.
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(0);

    if (bind(Socket, (sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        Close_Socket();
        return 0;
    }

    return 1;
}

void ListenerClass::Close_Socket()
{
    if (Socket == INVALID_SOCKET) {
        struct linger optval;
        optval.l_onoff = 0;
        optval.l_linger = 0;
        setsockopt(Socket, SOL_SOCKET, SO_LINGER, (const char *)&optval, sizeof(optval));
        closesocket(Socket);
        Socket = INVALID_SOCKET;
    }
}

#ifdef _WIN32
LRESULT __stdcall ListenerClass::Window_Proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    ListenerClass *thisptr = reinterpret_cast<ListenerClass *>(GetWindowLongPtrA(hwnd, -21));

    switch (umsg) {
        // Fallthrough on all these events we are going to ignore.
        case WM_CREATE:
        case WM_DESTROY:
        case WM_PAINT:
        case WM_COMMAND:
            return 0;
        case SSWM_LISTENER_MSG:
            DBG_INFO("Listener accepted a connection.");
            if ((lparam >> 16) == 0) {
                thisptr->Protocol->Connection_Requested();
            }

            return 0;
        default:
            break;
    }

    // All other events get passed to the next handler.
    return DefWindowProcA(hwnd, umsg, wparam, lparam);
}
#endif

int Init_Listener()
{
    ListenerProtocol = new ListenerProtocolClass;
    Listener = new ListenerClass(ListenerProtocol);

    DestAddress addr;
    addr.Host[0] = '\0';
    addr.Port = PlanetWestwoodPortNumber;

    if (!Listener->Start_Listening(&addr, sizeof(addr), 0)) {
        delete Listener;
        Listener = nullptr;
        delete ListenerProtocol;
        ListenerProtocol = nullptr;

        return 0;
    }

    return 1;
}

void Deinit_Listener() {}
