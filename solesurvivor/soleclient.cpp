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
#include "soleclient.h"
#include "soleglobals.h"
#include "solepackets.h"
#include "common/wwstd.h"
#include "common/ccfile.h"
#include "common/framelimit.h"
#include "common/ini.h"
#include "common/internet.h"
#include "function.h"
#include "reliable.h"
#include "soleparams.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern TimerClass GameTimer;

int Get_Map()
{
    static unsigned _rand_map_index = rand() + time(nullptr);
    static char _string_to_check[50];
    char name_buf[80];

    if (strlen(_string_to_check) > 0) {
        for (int i = 0; i < MPlayerScenarios.Count(); ++i) {
            strcpy(name_buf, MPlayerScenarios[i]);
            strupr(name_buf);

            if (strstr(name_buf, _string_to_check)) {
                return MPlayerFilenum[i];
            }
        }
    }

    CCFileClass fc("SERVER.INI");
    INIClass ini;
    ini.Load(fc);

    bool random_maps = ini.Get_Bool("GameParms", "RandomMaps", false);
    int old_index = _rand_map_index;

    if (random_maps) {
        _rand_map_index = rand() + Random_Picky(0, MPlayerFilenum.Count() - 1, __FILE__, __LINE__) + time(0);
    } else {
        ++_rand_map_index;
    }

    _rand_map_index %= MPlayerFilenum.Count();

    if (_rand_map_index == old_index) {
        ++_rand_map_index;
        _rand_map_index %= MPlayerFilenum.Count();
    }

    return _rand_map_index;
}

bool Connect_To_Host(const char* address)
{
    static constexpr int MAX_PACKET_SIZE = 422;
    ReliableProtocolClass* protocol = new ReliableProtocolClass();
    ReliableCommClass* comm = new ReliableCommClass(protocol, MAX_PACKET_SIZE);

    ReliableProtocols.Add(protocol);
    ReliableComms.Add(comm);

    char addr[30];
    strcpy(addr, address);

    if (!ReliableComms[0]->Connect(addr, PlanetWestwoodPortNumber, 0)) {
        ReliableComms[0]->Disconnect();
        delete ReliableComms[0];
        delete ReliableProtocols[0];
        ReliableComms.Clear();
        ReliableProtocols.Clear();

        return false;
    }

    return true;
}

// Defines function declared in internet.h in common.
int Read_Game_Options(char* name)
{
    return false;
}

void Client_Remote_Disconnect()
{
    if (ReliableProtocols.Count() != 0 && ReliableComms[0] != nullptr) {
        ReliableComms[0]->Disconnect();
        delete ReliableComms[0];
        delete ReliableProtocols[0];
    }

    ReliableComms.Clear();
    ReliableProtocols.Clear();
}

bool Wait_For_WDT_Connection()
{
    enum
    {
        BUTTON_CANCEL = 100,
        NUM_OF_BUTTONS = 1,
    };

    enum RedrawType
    {
        REDRAW_NONE = 0,
        REDRAW_BUTTONS,
        REDRAW_BACKGROUND,
        REDRAW_ALL = REDRAW_BACKGROUND
    };

    int d_cancel_x = 275;
    int d_cancel_y = 209;
    int d_cancel_w = 90;
    int d_cancel_h = 18;

    char str[80] = {0};
    ControlClass* commands = nullptr; // the button list

    TextButtonClass cancelbtn(BUTTON_CANCEL, 
                              TXT_CANCEL,
                              TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                              d_cancel_x,
                              d_cancel_y,
                              d_cancel_w,
                              d_cancel_h);
    commands = &cancelbtn;

    Set_Logic_Page(SeenBuff);
    Hide_Mouse();
    Load_Title_Screen("HTITLE.PCX", &HidPage, Palette);
    Set_Palette(Palette);
    Blit_Hid_Page_To_Seen_Buff();
    Show_Mouse();

    bool conn_sent = false;
    bool connected = true;
    bool process = true;
    int display = REDRAW_ALL;

    while (process) {
        if (AllSurfaces.SurfacesRestored) {
            AllSurfaces.SurfacesRestored = false;
            display = REDRAW_ALL;
        }

        Call_Back();

        if (display > REDRAW_NONE) {
            Hide_Mouse();
            if (display >= REDRAW_BACKGROUND) {
                Dialog_Box(206, 159, 228, 82);
                Draw_Caption(TXT_NONE, 206, 159, 228);
                Fancy_Text_Print(TXT_CONNECTING,
                                 320,
                                 173,
                                 CC_GREEN,
                                 TBLACK,
                                 TPF_BRIGHT_COLOR | TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
            }

            if (display >= REDRAW_BUTTONS) {
                commands->Flag_List_To_Redraw();
            }

            Show_Mouse();
            display = REDRAW_NONE;
        }

        /*
        ........................... Get user input ............................
        */
        int input = commands->Input();

        switch (input) {
        case (BUTTON_CANCEL | KN_BUTTON):
        case KN_ESC:
            connected = false;
            process = false;
            break;
        }

        int conn_state = ReliableProtocols[0]->Connected;

        switch (conn_state) {
        case -1:
            WWMessageBox().Process(TXT_UNABLE_TO_CONNECT);
            break;
        case 0:
            if (conn_sent) {
                if (GameParams.IsSquadChannel) {
                    WWMessageBox().Process("Connection refused.\r\rThis probably means that a squad game is in "
                                           "progress,\rand entry is closed until it finishes.");
                } else {
                    WWMessageBox().Process("Connection refused!");
                }
            }
            break;
        case 1:
            if (!conn_sent) {
                ReliableCommClass* comm = ReliableComms[0];
                ConnectionPacket packet;
                char dest[30];
                strcpy(dest, comm->Host.DotAddr);
                conn_sent = true;
                packet.PacketType = 101;
                strcpy(packet.PlayerName, MPlayerName);
                packet.Side = Side;
                packet.ChosenRTTI = ChosenRTTI;
                packet.ChosenType = ChosenType;
                packet.field_15 = 1;
                packet.field_19 = 0;
                packet.VersionNumber = Version_Number();
                ReliableProtocols[0]->Queue->Queue_Send(&packet, PacketLength[PACKET_CONNECTION]);
                ReliableComms[0]->Send();
            }
            break;
        }

        // If the initial connection packet has been sent, keep looking for the reply packet.
        if (conn_sent) {
            if (ReliableProtocols[0]->Queue->Num_Receive() > 0) {
                GameOptionsPacket* packet = (GameOptionsPacket*)ReliableProtocols[0]->Queue->Get_Receive(0)->Buffer;

                // Make sure it actually is a Game Options Packet.
                if (packet->PacketType == PACKET_GAME_OPTIONS) {
                    ScenPlayer = SCEN_PLAYER_MPLAYER;
                    ScenDir = SCEN_DIR_EAST;
                    Side = (HousesType)packet->Side;
                    SolePlayerHouse = (HousesType)packet->PlayerHouse;
                    GameOption_577AEC = packet->dword5;
                    GameOption_577AF0 = packet->dword5;
                    Scenario = packet->Scenario;
                    GameOption_577B00 = packet->dwordA;
                    MPlayerBases = packet->Bitfield1 & 1;
                    SoleEnhancedDefense = packet->Bitfield1 << 30 >> 31;
                    GameOption_577B08 = packet->Bitfield1 << 29 >> 31;
                    GameOption_577B0C = false;
                    BuildLevel = packet->BuildLevel;
                    MPlayerUnitCount = packet->MPlayerUnitCount;
                    Special.Unpack_Bools(packet->Special);
                    memcpy(&GameParams, &packet->GameParams, sizeof(GameParams));
                    GameTimer.Set(packet->TimerVal, true);
                    GameOptionsBitfield = packet->Bitfield2 & 1;

                    for (int i = 0; i < 4; ++i) {
                        TeamScores[i] = packet->TeamScores[i];
                    }

                    if (DebugLogTeams) {
                        DBG_LOG("*Receiving TeamScore in PACKET_GAME_OPTIONS during Wait_For_WDT_Connection\n");
                    }

                    for (int i = 0; i < WEAPON_COUNT; ++i) {
                        // Cast away const here for now.
                        // TODO Check how RA handles writable weapons from rules.ini, heap maybe?
                        WeaponTypeClass* weapons = (WeaponTypeClass*)Weapons;
                        weapons[i].Attack = packet->Damages[i];
                        weapons[i].ROF = packet->RateOfFires[i];
                        weapons[i].Range = packet->Ranges[i];
                        DBG_LOG(
                            "W %02d:\t\t%d\t\t%d\t\t%d\n", i, Weapons[i].Attack, Weapons[i].ROF, Weapons[i].Range);
                    }

                    if (SoleEnhancedDefense) {
                        Special.IsTreeTarget = true;
                        Special.IsSmartDefense = true;
                    } else {
                        Special.IsTreeTarget = false;
                        Special.IsSmartDefense = false;
                    }

                    ScenarioIdx = -1;

                    for (int i = 0; i < MPlayerFilenum.Count(); ++i) {
                        if (packet->Scenario == MPlayerFilenum[i]) {
                            ScenarioIdx = i;
                        }
                    }

                    // If we didn't find the requested scenario we have a problem.
                    if (ScenarioIdx == -1) {
                        WWMessageBox().Process(TXT_SCENARIO_NOT_FOUND);
                        connected = false;
                    }

                    process = false;
                    ReliableProtocols[0]->Queue->UnQueue_Receive(nullptr, nullptr, 0);
                }
            }
        }

        Frame_Limiter();
    }

    Hide_Mouse();
    Load_Title_Screen("HTITLE.PCX", &HidPage, Palette);
    Set_Palette(Palette);
    Blit_Hid_Page_To_Seen_Buff();
    Show_Mouse();

    return connected;
}
