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
#include "vqaconfig.h"
#include "wwstd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static VQAConfig _defaultconfig = {
    NULL,          // DrawerCallback
    NULL,          // EventHandler
    0,             // NotifyFlags
    19,            // Vmode
    -1,            // VBIBit
    NULL,          // ImageBuf
    320,           // ImageWidth
    200,           // ImageHeight
    -1,            // X1
    -1,            // Y1
    -1,            // FrameRate
    -1,            // DrawRate
    -1,            // TimerMethod
    0,             // DrawFlags
    VQAOPTF_AUDIO, // OptionFlags
    6,             // NumFrameBufs
    3,             // NumCBBufs
#ifdef _WIN32
    NULL, // SoundObject
    NULL, // PrimarySoundBuffer
#endif
    NULL,             // VocFile
    NULL,             // AudioBuf
    (unsigned)-1,     // AudioBufSize
    -1,               // AudioRate
    255,              // Volume
    8192,             // HMIBufSize
    -1,               // DigiHandle
    -1,               // DigiCard
    -1,               // DigiPort
    -1,               // DigiIRQ
    -1,               // DigiDMA
    VQA_LANG_ENGLISH, // Language
    NULL,             // CapFont
    NULL              // EVAFont
};

void VQA_INIConfig(VQAConfig* config)
{
    // INI config isn't needed by the original games, so isn't implemented here.
    memcpy(config, &_defaultconfig, sizeof(*config));
}

void VQA_DefaultConfig(VQAConfig* dest)
{
    memcpy(dest, &_defaultconfig, sizeof(*dest));
}
