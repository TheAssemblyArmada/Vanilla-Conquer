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
#include "vqaloader.h"
#include "soscomp.h"
#include "auduncmp.h"
#include "file.h"
#include "endianness.h"
#include "lcw.h"
#include "misc.h"
#include "vqacaption.h"
#include "vqaconfig.h"
#include <stdlib.h>
#include <string.h>

void Flip_VQAHeader(VQAHeader* header)
{
    header->Version = le16toh(header->Version);
    header->Flags = le16toh(header->Flags);
    header->Frames = le16toh(header->Frames);
    header->ImageWidth = le16toh(header->ImageWidth);
    header->ImageHeight = le16toh(header->ImageHeight);
    header->Num1Colors = le16toh(header->Num1Colors);
    header->CBentries = le16toh(header->CBentries);
    header->Xpos = le16toh(header->Xpos);
    header->Ypos = le16toh(header->Ypos);
    header->MaxFramesize = le16toh(header->MaxFramesize);
    header->SampleRate = le16toh(header->SampleRate);
    header->AltSampleRate = le16toh(header->AltSampleRate);
    header->MaxCompressedCBSize = le32toh(header->MaxCompressedCBSize);
    header->field_26 = le32toh(header->field_26);
}

int VQA_Load_FINF(VQAHandle* handle, unsigned iffsize)
{
    VQAData* data = handle->VQABuf;
    int i;

    if (data != nullptr && data->Foff != nullptr) {
        if (handle->StreamHandler(handle, VQACMD_READ, data->Foff, (iffsize + 1) & (~1))) {
            return VQAERR_READ;
        }

        for (i = 0; i < iffsize / 4; i++) {
            data->Foff[i] = le32toh(data->Foff[i]);
        }
    } else if (handle->StreamHandler(handle, VQACMD_SEEK, (void*)SEEK_CUR, (iffsize + 1) & (~1))) {
        return VQAERR_SEEK;
    }

    return VQAERR_NONE;
}

int VQA_Load_CAP0(VQAHandle* handle, unsigned iffsize)
{
    VQAData* data = handle->VQABuf;

    if (data != nullptr && data->Foff != nullptr) {
        if (handle->StreamHandler(handle, VQACMD_READ, data->Foff, (iffsize + 1) & (~1))) {
            return VQAERR_READ;
        }
    } else if (handle->StreamHandler(handle, VQACMD_SEEK, (void*)SEEK_CUR, (iffsize + 1) & (~1))) {
        return VQAERR_SEEK;
    }

    return VQAERR_NONE;
}

int VQA_Load_CBF0(VQAHandle* handle, unsigned iffsize)
{
    VQALoader* loader = &handle->VQABuf->Loader;
    VQACBNode* curcb = (VQACBNode*)loader->CurCB;

    if (handle->StreamHandler(handle, VQACMD_READ, loader->CurCB->Buffer, (iffsize + 1) & (~1))) {
        return VQAERR_READ;
    }

    loader->NumPartialCB = 0;
    curcb->Flags &= 0xFFFFFFFD;
    curcb->CBOffset = 0;
    loader->FullCB = curcb;
    loader->FullCB->Flags &= (~1);
    loader->CurCB = curcb->Next;

    return VQAERR_NONE;
}

int VQA_Load_CBFZ(VQAHandle* handle, unsigned iffsize)
{
    VQALoader* loader = &handle->VQABuf->Loader;
    VQACBNode* curcb = loader->CurCB;
    unsigned lcwoffset = handle->VQABuf->MaxCBSize - ((iffsize + 1) & 0xFFFE);

    if (handle->StreamHandler(handle, VQACMD_READ, &loader->CurCB->Buffer[lcwoffset], (iffsize + 1) & (~1))) {
        return VQAERR_READ;
    }

    loader->NumPartialCB = 0;
    curcb->Flags |= 2u;
    curcb->CBOffset = lcwoffset;
    loader->FullCB = curcb;
    loader->FullCB->Flags &= (~1);
    loader->CurCB = curcb->Next;

    return VQAERR_NONE;
}

int VQA_Load_CBP0(VQAHandle* handle, unsigned iffsize)
{
    VQALoader* loader = &handle->VQABuf->Loader;
    VQACBNode* curcb = (VQACBNode*)loader->CurCB;

    if (handle->StreamHandler(
            handle, VQACMD_READ, &loader->CurCB->Buffer[handle->VQABuf->Loader.PartialCBSize], (iffsize + 1) & (~1))) {
        return VQAERR_READ;
    }

    loader->PartialCBSize += (uint16_t)iffsize;

    if (handle->Header.Groupsize == ++loader->NumPartialCB) {
        loader->NumPartialCB = 0;
        loader->PartialCBSize = 0;
        curcb->Flags &= 0xFFFFFFFD;
        curcb->CBOffset = 0;
        loader->FullCB = curcb;
        loader->FullCB->Flags &= (~1);
        loader->CurCB = curcb->Next;
    }

    return VQAERR_NONE;
}

int VQA_Load_CBPZ(VQAHandle* handle, unsigned iffsize)
{
    VQAData* data = handle->VQABuf;
    VQALoader* loader = &data->Loader;
    VQACBNode* curcb = data->Loader.CurCB;
    unsigned padsize = (iffsize + 1) & (~1);

    if (!data->Loader.PartialCBSize) {
        curcb->CBOffset = data->MaxCBSize - (handle->Header.Groupsize * padsize + 0x64);
    }

    if (handle->StreamHandler(
            handle, VQACMD_READ, &curcb->Buffer[curcb->CBOffset] + data->Loader.PartialCBSize, padsize)) {
        return VQAERR_READ;
    }

    loader->PartialCBSize += (uint16_t)iffsize;

    if (handle->Header.Groupsize == ++loader->NumPartialCB) {
        loader->NumPartialCB = 0;
        loader->PartialCBSize = 0;
        curcb->Flags |= 2u;
        loader->FullCB = curcb;
        loader->FullCB->Flags &= (~1);
        loader->CurCB = curcb->Next;
    }

    return VQAERR_NONE;
}

int VQA_Load_CPL0(VQAHandle* handle, unsigned iffsize)
{
    VQAFrameNode* curframe = handle->VQABuf->Loader.CurFrame;

    if (handle->StreamHandler(handle, VQACMD_READ, curframe->Palette, (iffsize + 1) & (~1))) {
        return VQAERR_READ;
    }

    curframe->Flags &= 0xFFFFFFF7;
    curframe->PalOffset = 0;
    curframe->PaletteSize = (uint16_t)iffsize;

    return VQAERR_NONE;
}

int VQA_Load_CPLZ(VQAHandle* handle, unsigned iffsize)
{
    VQAFrameNode* curframe = handle->VQABuf->Loader.CurFrame;
    unsigned lcwoffset = handle->VQABuf->MaxPalSize - ((iffsize + 1) & 0xFFFE);

    if (handle->StreamHandler(handle, VQACMD_READ, &curframe->Palette[lcwoffset], (iffsize + 1) & (~1))) {
        return VQAERR_READ;
    }

    curframe->Flags |= 8u;
    curframe->PalOffset = lcwoffset;
    curframe->PaletteSize = (uint16_t)iffsize;

    return VQAERR_NONE;
}

int VQA_Load_VPT0(VQAHandle* handle, unsigned iffsize)
{
    VQAFrameNode* curframe = handle->VQABuf->Loader.CurFrame;

    if (handle->StreamHandler(handle, VQACMD_READ, curframe->Pointers, (iffsize + 1) & (~1))) {
        return VQAERR_READ;
    }

    curframe->Flags &= ~0x10;
    curframe->PtrOffset = 0;

    return VQAERR_NONE;
}

int VQA_Load_VPTZ(VQAHandle* handle, unsigned iffsize)
{
    VQAFrameNode* curframe = handle->VQABuf->Loader.CurFrame;
    unsigned lcwoffset = handle->VQABuf->MaxPtrSize - ((iffsize + 1) & (~1));

    if (handle->StreamHandler(handle, VQACMD_READ, &curframe->Pointers[lcwoffset], (iffsize + 1) & (~1))) {
        return VQAERR_READ;
    }

    curframe->Flags |= 0x10;
    curframe->PtrOffset = lcwoffset;

    return VQAERR_NONE;
}

int VQA_Load_SND0(VQAHandle* handle, unsigned iffsize)
{
    VQAConfig* config = &handle->Config;
    VQAData* data = handle->VQABuf;
    VQAAudio* audio = &data->Audio;
    unsigned size_aligned = ((iffsize + 1) & (~1));

    if (!(config->OptionFlags & 1) || !audio->Buffer) {
        if (handle->StreamHandler(handle, VQACMD_SEEK, (void*)SEEK_CUR, size_aligned)) {
            return VQAERR_SEEK;
        }

        return VQAERR_NONE;
    }

    if (size_aligned <= audio->TempBufLen || audio->AudBufPos) {
        if (handle->StreamHandler(handle, VQACMD_READ, audio->TempBuf, size_aligned)) {
            return VQAERR_READ;
        }

        audio->TempBufSize = iffsize;
        return VQAERR_NONE;
    }

    if (handle->StreamHandler(handle, VQACMD_READ, audio->Buffer, size_aligned)) {
        return VQAERR_READ;
    }

    audio->AudBufPos += iffsize;

    for (unsigned i = 0; i < (iffsize / config->HMIBufSize); ++i) {
        audio->IsLoaded[i] = 1;
    }

    return VQAERR_NONE;
}

int VQA_Load_SND1(VQAHandle* handle, unsigned iffsize)
{
    VQASND1Header snd1hdr;
    VQAConfig* config = &handle->Config;
    VQAAudio* audio = &handle->VQABuf->Audio;

    int size_aligned = ((iffsize + 1) & 0xFFFE);

    if (!(config->OptionFlags & 1) || !audio->Buffer) {
        if (handle->StreamHandler(handle, VQACMD_SEEK, (void*)SEEK_CUR, size_aligned)) {
            return VQAERR_SEEK;
        }

        return VQAERR_NONE;
    }

    if (handle->StreamHandler(handle, VQACMD_READ, &snd1hdr, sizeof(snd1hdr))) {
        return VQAERR_READ;
    }

    size_aligned -= sizeof(snd1hdr);

    if ((unsigned)snd1hdr.OutSize <= audio->TempBufLen || audio->AudBufPos > 0) {
        if (snd1hdr.OutSize == snd1hdr.Size) {
            if (handle->StreamHandler(handle, VQACMD_READ, audio->TempBuf, size_aligned)) {
                return VQAERR_READ;
            }
        } else {
            void* decomp_buff = ((audio->TempBuf + audio->TempBufLen) - size_aligned);

            if (handle->StreamHandler(handle, VQACMD_READ, decomp_buff, size_aligned)) {
                return VQAERR_READ;
            }

            Audio_Unzap(decomp_buff, audio->TempBuf, snd1hdr.OutSize);
        }

        audio->TempBufSize = snd1hdr.OutSize;

        return VQAERR_NONE;
    }

    if (snd1hdr.OutSize == snd1hdr.Size) {
        if (handle->StreamHandler(handle, VQACMD_READ, audio->Buffer, size_aligned)) {
            return VQAERR_READ;
        }

    } else {
        void* decomp_buff = ((audio->Buffer + config->AudioBufSize) - size_aligned);

        if (handle->StreamHandler(handle, VQACMD_READ, decomp_buff, size_aligned)) {
            return VQAERR_READ;
        }

        Audio_Unzap(decomp_buff, audio->Buffer, snd1hdr.OutSize);
    }

    audio->AudBufPos += snd1hdr.OutSize;

    for (int i = 0; i < (snd1hdr.OutSize / config->HMIBufSize); ++i) {
        audio->IsLoaded[i] = 1;
    }

    return VQAERR_NONE;
}

int VQA_Load_SND2(VQAHandle* handle, unsigned iffsize)
{
    VQAAudio* audio = &handle->VQABuf->Audio;
    VQAConfig* config = &handle->Config;
    unsigned size_aligned = ((iffsize + 1) & 0xFFFE);

    if (!(config->OptionFlags & 1) || !audio->Buffer) {
        if (handle->StreamHandler(handle, VQACMD_SEEK, (void*)SEEK_CUR, size_aligned)) {
            return VQAERR_SEEK;
        }

        return VQAERR_NONE;
    }

    unsigned decomp_size = iffsize * (handle->VQABuf->Audio.BitsPerSample / 4);

    if (decomp_size <= handle->VQABuf->Audio.TempBufLen || handle->VQABuf->Audio.AudBufPos != 0) {
        void* buffer = &audio->TempBuf[handle->VQABuf->Audio.TempBufLen - size_aligned];

        if (handle->StreamHandler(handle, VQACMD_READ, buffer, size_aligned)) {
            return VQAERR_READ;
        }

        audio->AdcpmInfo.lpSource = (char*)buffer;
        audio->AdcpmInfo.lpDest = (char*)audio->TempBuf;
        sosCODECDecompressData(&audio->AdcpmInfo, decomp_size);
        audio->TempBufSize = decomp_size;

        return VQAERR_NONE;
    }

    void* buffer = &audio->Buffer[config->AudioBufSize - size_aligned];

    if (handle->StreamHandler(handle, VQACMD_READ, buffer, size_aligned)) {
        return VQAERR_READ;
    }

    audio->AdcpmInfo.lpSource = (char*)buffer;
    audio->AdcpmInfo.lpDest = (char*)audio->Buffer;
    sosCODECDecompressData(&audio->AdcpmInfo, decomp_size);
    audio->AudBufPos += decomp_size;

    for (unsigned i = 0; i < (decomp_size / config->HMIBufSize); ++i) {
        audio->IsLoaded[i] = 1;
    }

    return VQAERR_NONE;
}

int VQA_Load_VQF(VQAHandle* handle, unsigned frame_iffsize)
{
    unsigned bytes_loaded = 0;
    VQAData* data = handle->VQABuf;
    VQAFrameNode* curframe = data->Loader.CurFrame;
    unsigned framesize = (frame_iffsize + 1) & (~1);
    VQADrawer* drawer = &handle->VQABuf->Drawer;
    VQAChunkHeader* chunk = &data->Chunk;

    while (bytes_loaded < framesize) {
        if (handle->StreamHandler(handle, VQACMD_READ, chunk, sizeof(*chunk))) {
            return VQAERR_ERROR;
        }

        unsigned iffsize = be32toh(chunk->Size);
        bytes_loaded += ((iffsize + 1) & (~1)) + sizeof(*chunk);

        switch (chunk->ID) {
        // Vector Pointer Table
        case CHUNK_VPT0:

            if (VQA_Load_VPT0(handle, iffsize)) {
                return VQAERR_READ;
            }

            break;

        case CHUNK_VPTZ:

            if (VQA_Load_VPTZ(handle, iffsize)) {
                return VQAERR_READ;
            }

            break;

        // Vector Pointer
        case CHUNK_VPTD:

            if (VQA_Load_VPTZ(handle, iffsize)) {
                return VQAERR_READ;
            }

            break;

        case CHUNK_VPTK:

            if (VQA_Load_VPTZ(handle, iffsize)) {
                return VQAERR_READ;
            }

            curframe->Flags |= 2;
            break;

        case CHUNK_VPTR:

            if (VQA_Load_VPT0(handle, iffsize)) {
                return VQAERR_READ;
            }

            break;

        case CHUNK_VPRZ:

            if (VQA_Load_VPTZ(handle, iffsize)) {
                return VQAERR_READ;
            }

            break;

        // BR
        // not proper code, both check and set flags
        case CHUNK_VPDZ:

            if (VQA_Load_VPTZ(handle, iffsize)) {
                return VQAERR_READ;
            }

            break;

        case CHUNK_VPKZ:

            if (VQA_Load_VPTZ(handle, iffsize)) {
                return VQAERR_READ;
            }

            break;

        // CodeBook Full
        case CHUNK_CBF0:

            if (VQA_Load_CBF0(handle, iffsize)) {
                return VQAERR_READ;
            }

            break;

        case CHUNK_CBFZ:

            if (VQA_Load_CBFZ(handle, iffsize)) {
                return VQAERR_READ;
            }

            break;

        // CodeBook Partial
        case CHUNK_CBP0:

            if (VQA_Load_CBP0(handle, iffsize)) {
                return VQAERR_READ;
            }

            break;

        case CHUNK_CBPZ:

            if (VQA_Load_CBPZ(handle, iffsize)) {
                return VQAERR_READ;
            }

            break;

        // Color PaLette or Codebook PaLette
        case CHUNK_CPL0:

            if (VQA_Load_CPL0(handle, iffsize)) {
                return VQAERR_READ;
            }

            if (!drawer->CurPalSize) {
                memcpy(drawer->Palette, curframe->Palette, curframe->PaletteSize);
                drawer->CurPalSize = curframe->PaletteSize;
            }

            curframe->Flags |= 4;
            break;

        case CHUNK_CPLZ:

            if (VQA_Load_CPLZ(handle, iffsize)) {
                return VQAERR_READ;
            }

            if (!drawer->CurPalSize) {
                drawer->CurPalSize =
                    LCW_Uncompress((curframe->PalOffset + curframe->Palette), drawer->Palette, data->MaxPalSize);
            }

            curframe->Flags |= 4;
            break;

        default:
            break;
        }
    }

    return VQAERR_NONE;
}

int VQA_Open(VQAHandle* handle, const char* filename, VQAConfig* config)
{
    VQAHeader* header = &handle->Header;
    VQAChunkHeader chunk;

    if (handle->StreamHandler(handle, VQACMD_OPEN, (char*)filename, 0)) {
        return VQAERR_OPEN;
    }

    if (handle->StreamHandler(handle, VQACMD_READ, &chunk, sizeof(chunk))) {
        VQA_Close(handle);
        return VQAERR_READ;
    }

    if (chunk.ID != CHUNK_FORM || chunk.Size <= 0) {
        VQA_Close(handle);
        return VQAERR_NOTVQA;
    }

    if (handle->StreamHandler(handle, VQACMD_READ, &chunk.ID, sizeof(chunk.ID))) {
        VQA_Close(handle);
        return VQAERR_READ;
    }

    if (chunk.ID != CHUNK_WVQA) {
        VQA_Close(handle);
        return VQAERR_NOTVQA;
    }

    if (config != nullptr) {
        memcpy(&handle->Config, config, sizeof(*config));
    } else {
        VQA_DefaultConfig(&handle->Config);
    }

    bool frame_info_found = false;

    while (!frame_info_found) {
        if (handle->StreamHandler(handle, VQACMD_READ, &chunk, sizeof(chunk))) {
            VQA_Close(handle);
            return VQAERR_READ;
        }

        int chunk_size = be32toh(chunk.Size);

        switch (chunk.ID) {
        case CHUNK_VQHD:

            if (chunk_size != sizeof(VQAHeader)) {
                VQA_Close(handle);
                return VQAERR_NOTVQA;
            }

            if (handle->StreamHandler(handle, VQACMD_READ, header, sizeof(*header))) {
                VQA_Close(handle);
                return VQAERR_READ;
            }

            Flip_VQAHeader(header);

            // in LOLG VQAs Groupsize is 0 because it only has one codebook chunk so forcing it to common default here,
            // allows LOLG VQAs to be played
            if (header->Groupsize == 0) {
                header->Groupsize = 8;
            }

            if (config->ImageWidth == -1) {
                config->ImageWidth = header->ImageWidth;
            }

            if (config->ImageHeight == -1) {
                config->ImageHeight = header->ImageHeight;
            }

            if (handle->Config.FrameRate == -1) {
                handle->Config.FrameRate = header->FPS;
            }

            if (handle->Config.DrawRate == -1) {
                handle->Config.DrawRate = header->FPS;
            }

            if (handle->Config.DrawRate == -1 || !handle->Config.DrawRate) {
                handle->Config.DrawRate = header->FPS;
            }

            if (header->Version >= 2 && !(header->Flags & 2)) {
                config->OptionFlags &= 0xBF;
            }

            handle->VQABuf = VQA_AllocBuffers(header, &handle->Config);

            if (handle->VQABuf == nullptr) {
                VQA_Close(handle);
                return VQAERR_NOMEM;
            }

            break;

        // TODO: Needs confirming with a 'Poly VQA file.
        case CHUNK_NAME: {

            if (handle->StreamHandler(handle, VQACMD_READ, &chunk, sizeof(VQAChunkHeader))) {
                VQA_Close(handle);
                return VQAERR_READ;
            }

            char* buffer = new char[chunk_size + 1];
            memset(buffer, 0, chunk_size + 1);

            if (handle->StreamHandler(handle, VQACMD_READ, buffer, chunk_size)) {
                VQA_Close(handle);
                return VQAERR_READ;
            }

            break;
        }
        case CHUNK_EVA0:

            if (config->EVAFont && config->OptionFlags & VQAOPTF_CAPTIONS) {
                // TODO
            }

            break;

        case CHUNK_CAP0:

            if (config->CapFont && config->OptionFlags & VQAOPTF_AUDIO) {
                short v78 = 0;
                if (handle->StreamHandler(handle, VQACMD_READ, &v78, sizeof(v78))) {
                    VQA_Close(handle);
                    return VQAERR_READ;
                }

                if (handle->StreamHandler(handle, VQACMD_READ, &v78, sizeof(v78))) {
                    VQA_Close(handle);
                    return VQAERR_NOMEM;
                }

                // TODO

                if (handle->field_C2) {
                    VQA_Close(handle);
                    return VQAERR_NOMEM;
                }
            }

            break;

        case CHUNK_FINF:

            if (VQA_Load_FINF(handle, chunk_size)) {
                VQA_Close(handle);
                return VQAERR_READ;
            }
            frame_info_found = true;
            break;

        default:

            if (handle->StreamHandler(handle, VQACMD_SEEK, (void*)SEEK_CUR, (chunk_size + 1) & ~1)) {
                VQA_Close(handle);
                return VQAERR_SEEK;
            }

            break;
        };
    }

    if (frame_info_found) {

        if (!(header->Flags & 1)) {
            handle->Config.OptionFlags &= VQAOPTF_AUDIO;
        }

        if (handle->Config.OptionFlags & VQAOPTF_AUDIO) {
#ifdef _WIN32
            if (VQA_OpenAudio(handle, (void*)MainWindow)) {
#else
            if (VQA_OpenAudio(handle, nullptr)) {
#endif
                VQA_Close(handle);
                return VQAERR_AUDIO;
            }

            VQAAudio* audio = &handle->VQABuf->Audio;
            sosCODECInitStream(&audio->AdcpmInfo);

            if (header->Version >= 2) {
                audio->AdcpmInfo.wBitSize = audio->BitsPerSample;
                audio->AdcpmInfo.dwUnCompSize =
                    audio->Channels * (audio->BitsPerSample / 8) * audio->SampleRate / header->FPS * header->Frames;
                audio->AdcpmInfo.wChannels = audio->Channels;
            } else {
                audio->AdcpmInfo.wBitSize = 8;
                audio->AdcpmInfo.dwUnCompSize = 22050 / header->FPS * header->Frames;
                audio->AdcpmInfo.wChannels = 1;
            }

            audio->AdcpmInfo.dwCompSize = audio->AdcpmInfo.dwUnCompSize / audio->AdcpmInfo.wBitSize / 4;
        }

        if ((!(config->OptionFlags & 1) || config->TimerMethod == 2)) {
            if (VQA_StartTimerInt(handle, config->OptionFlags & 0x20)) {
                VQA_Close(handle);
                return VQAERR_AUDIO;
            }
        }

        if (VQA_PrimeBuffers(handle)) {
            VQA_Close(handle);
            return VQAERR_READ;
        }
    }

    return VQAERR_NONE;
}

void VQA_Close(VQAHandle* handle)
{
    if (handle->Config.OptionFlags & 1) {
        VQA_CloseAudio(handle);
    } else {
        VQA_StopTimerInt(handle);
    }

    if (handle->VQABuf) {
        VQA_FreeBuffers(handle->VQABuf, &handle->Config, &handle->Header);
    }

    handle->StreamHandler(handle, VQACMD_CLOSE, nullptr, 0);
    memset(handle, 0, sizeof(VQAHandle));
}

int VQA_LoadFrame(VQAHandle* handle)
{
    unsigned iffsize;
    bool frame_loaded = false;
    VQAData* data = handle->VQABuf;
    VQALoader* loader = &data->Loader;
    VQADrawer* drawer = &handle->VQABuf->Drawer;
    VQAFrameNode* curframe = data->Loader.CurFrame;
    VQAChunkHeader* chunk = &data->Chunk;

    if (handle->Header.Frames <= data->Loader.CurFrameNum) {
        return VQAERR_ERROR;
    }

    if (curframe->Flags & 1) {
        ++data->Loader.WaitsOnDrawer;
        return VQAERR_NOBUFFER;
    }

    if (!(data->Flags & 4)) {
        frame_loaded = false;
        data->Loader.FrameSize = 0;
        curframe->Codebook = loader->FullCB;
    }

    while (!frame_loaded) {
        if (!(data->Flags & 4)) {
            if (handle->StreamHandler(handle, VQACMD_READ, chunk, sizeof(*chunk))) {
                return VQAERR_ERROR;
            }

            iffsize = be32toh(chunk->Size);
            loader->FrameSize += iffsize;
        }

        // Sorted by how its commonly in the VQA's
        switch (chunk->ID) {
        case CHUNK_FINF:

            if (VQA_Load_FINF(handle, iffsize)) {
                return VQAERR_READ;
            }

            break;

        case CHUNK_VQFK:

            if (VQA_Load_VQF(handle, iffsize)) {
                return VQAERR_READ;
            }

            curframe->Flags |= 2;
            frame_loaded = true;
            break;

        case CHUNK_VQFR:

            if (VQA_Load_VQF(handle, iffsize)) {
                return VQAERR_READ;
            }

            frame_loaded = true;
            break;

        // Version 3 chunk
        // Vector Quantized Frame Loop? appears to call Load_VQF
        // Not the proper code as there's a few checks and flags it sets
        case CHUNK_VQFL:

            if (VQA_Load_VQF(handle, iffsize)) {
                return VQAERR_READ;
            }

            frame_loaded = true;
            break;

        // Vector Pointer Table
        case CHUNK_VPT0:

            if (VQA_Load_VPT0(handle, iffsize)) {
                return VQAERR_READ;
            }

            frame_loaded = true;
            break;

        case CHUNK_VPTZ:

            if (VQA_Load_VPTZ(handle, iffsize)) {
                return VQAERR_READ;
            }

            frame_loaded = true;
            break;

        // Vector Pointer
        case CHUNK_VPTD:

            if (VQA_Load_VPTZ(handle, iffsize)) {
                return VQAERR_READ;
            }

            frame_loaded = true;
            break;

        case CHUNK_VPTK:

            if (VQA_Load_VPTZ(handle, iffsize)) {
                return VQAERR_READ;
            }

            curframe->Flags |= 2;
            frame_loaded = true;
            break;

        case CHUNK_VPTR:

            if (VQA_Load_VPT0(handle, iffsize)) {
                return VQAERR_READ;
            }

            frame_loaded = true;
            break;

        case CHUNK_VPRZ:

            if (VQA_Load_VPTZ(handle, iffsize)) {
                return VQAERR_READ;
            }

            frame_loaded = true;
            break;

        // CodeBook Full
        case CHUNK_CBF0:

            if (VQA_Load_CBF0(handle, iffsize)) {
                return VQAERR_READ;
            }

            break;

        case CHUNK_CBFZ:

            if (VQA_Load_CBFZ(handle, iffsize)) {
                return VQAERR_READ;
            }

            break;

        // CodeBook Partial
        case CHUNK_CBP0:

            if (VQA_Load_CBP0(handle, iffsize)) {
                return VQAERR_READ;
            }

            break;

        case CHUNK_CBPZ:

            if (VQA_Load_CBPZ(handle, iffsize)) {
                return VQAERR_READ;
            }

            break;

        // Color PaLette or Codebook PaLette
        case CHUNK_CPL0:

            if (VQA_Load_CPL0(handle, iffsize)) {
                return VQAERR_READ;
            }

            if (!drawer->CurPalSize) {
                memcpy(drawer->Palette, curframe->Palette, curframe->PaletteSize);
                drawer->CurPalSize = curframe->PaletteSize;
            }

            curframe->Flags |= 4;

            break;

        case CHUNK_CPLZ:

            if (VQA_Load_CPLZ(handle, iffsize)) {
                return VQAERR_READ;
            }

            if (!drawer->CurPalSize) {
                drawer->CurPalSize =
                    LCW_Uncompress((curframe->PalOffset + curframe->Palette), drawer->Palette, data->MaxPalSize);
            }

            curframe->Flags |= 4;

            break;

        case CHUNK_SND0:

            if (handle->Config.OptionFlags & 0x40) {
                if (handle->StreamHandler(handle, VQACMD_SEEK, (void*)SEEK_CUR, (iffsize + 1) & (~1))) {
                    return VQAERR_SEEK;
                }

            } else {
                if (VQA_CopyAudio(handle) == VQAERR_SLEEPING) {
                    handle->VQABuf->Flags |= 4;
                    return VQAERR_SLEEPING;
                }

                handle->VQABuf->Flags &= 0xFB;
                if (VQA_Load_SND0(handle, iffsize)) {
                    return VQAERR_READ;
                }
            }

            break;

        case CHUNK_SNA0:

            if (handle->Config.OptionFlags & 0x40) {
                if (VQA_CopyAudio(handle) == VQAERR_SLEEPING) {
                    handle->VQABuf->Flags |= 4;
                    return VQAERR_SLEEPING;
                }

                handle->VQABuf->Flags &= 0xFB;

                if (VQA_Load_SND0(handle, iffsize)) {
                    return VQAERR_READ;
                }

            } else {
                if (handle->StreamHandler(handle, VQACMD_SEEK, (void*)SEEK_CUR, (iffsize + 1) & (~1))) {
                    return VQAERR_SEEK;
                }
            }

            break;

        case CHUNK_SND1:

            if (handle->Config.OptionFlags & 0x40) {
                if (handle->StreamHandler(handle, VQACMD_SEEK, (void*)SEEK_CUR, (iffsize + 1) & (~1))) {
                    return VQAERR_SEEK;
                }
            } else {
                if (VQA_CopyAudio(handle) == VQAERR_SLEEPING) {
                    handle->VQABuf->Flags |= 4;
                    return VQAERR_SLEEPING;
                }

                handle->VQABuf->Flags &= 0xFB;

                if (VQA_Load_SND1(handle, iffsize)) {
                    return VQAERR_READ;
                }
            }

            break;

        case CHUNK_SNA1:

            if (handle->Config.OptionFlags & 0x40) {
                if (VQA_CopyAudio(handle) == VQAERR_SLEEPING) {
                    handle->VQABuf->Flags |= 4;
                    return VQAERR_SLEEPING;
                }

                handle->VQABuf->Flags &= 0xFB;

                if (VQA_Load_SND1(handle, iffsize)) {
                    return VQAERR_READ;
                }
            } else {
                if (handle->StreamHandler(handle, VQACMD_SEEK, (void*)SEEK_CUR, (iffsize + 1) & (~1))) {
                    return VQAERR_SEEK;
                }
            }

            break;

        case CHUNK_SND2:

            if (handle->Config.OptionFlags & 0x40) {
                if (handle->StreamHandler(handle, VQACMD_SEEK, (void*)SEEK_CUR, (iffsize + 1) & (~1))) {
                    return VQAERR_SEEK;
                }
            } else {
                if (VQA_CopyAudio(handle) == VQAERR_SLEEPING) {
                    handle->VQABuf->Flags |= 4;
                    return VQAERR_SLEEPING;
                }

                handle->VQABuf->Flags &= 0xFB;

                if (VQA_Load_SND2(handle, iffsize)) {
                    return VQAERR_READ;
                }
            }

            break;

        case CHUNK_SNA2:

            if (handle->Config.OptionFlags & 0x40) {
                if (VQA_CopyAudio(handle) == VQAERR_SLEEPING) {
                    handle->VQABuf->Flags |= 4;
                    return VQAERR_SLEEPING;
                }

                handle->VQABuf->Flags &= 0xFB;

                if (VQA_Load_SND2(handle, iffsize)) {
                    return VQAERR_READ;
                }
            } else if (handle->StreamHandler(handle, VQACMD_SEEK, (void*)SEEK_CUR, (iffsize + 1) & (~1))) {
                return VQAERR_SEEK;
            }

            break;

        case CHUNK_SN2J:

            if (handle->StreamHandler(handle, VQACMD_SEEK, (void*)SEEK_CUR, (iffsize + 1) & (~1))) {
                return VQAERR_SEEK;
            }

            break;

        default:
            break;
        }
    }

    if (loader->CurFrameNum > 0 && loader->FrameSize > loader->MaxFrameSize) {
        loader->MaxFrameSize = loader->FrameSize;
    }

    curframe->FrameNum = loader->CurFrameNum++;
    loader->LastFrameNum = loader->CurFrameNum;
    curframe->Flags |= 1;
    loader->CurFrame = curframe->Next;

    return VQAERR_NONE;
}

int VQA_SeekFrame(VQAHandle* handle, int framenum, int fromwhere)
{
    VQAErrorType rc = VQAERR_ERROR;
    VQAData* data = handle->VQABuf;
    VQAAudio* audio = &data->Audio;

    if (audio->Flags & 0x40) {
        VQA_StopAudio(handle);
    }

    if (handle->Header.Frames <= framenum) {
        return rc;
    }

    if (!data->Foff) {
        return rc;
    }

    int group = framenum / handle->Header.Groupsize * handle->Header.Groupsize;

    if (group >= handle->Header.Groupsize) {
        group -= handle->Header.Groupsize;
    }

    if (handle->StreamHandler(handle, VQACMD_SEEK, nullptr, 2 * ((data->Foff[group]) & 0xFFFFFFF))) {
        return VQAERR_SEEK;
    }

    data->Loader.NumPartialCB = 0;
    data->Loader.PartialCBSize = 0;
    data->Loader.FullCB = data->CBData;
    data->Loader.CurCB = data->CBData;
    data->Loader.CurFrameNum = group;

    for (int i = 0; i < framenum - group; ++i) {
        data->Loader.CurFrame->Flags = 0;
        rc = (VQAErrorType)VQA_LoadFrame(handle);

        if (rc) {
            if (rc != VQAERR_NOBUFFER && rc != VQAERR_SLEEPING) {
                rc = VQAERR_ERROR;
                break;
            }

            rc = VQAERR_NONE;
        }
    }

    if (rc) {
        return rc;
    }

    data->Loader.CurFrame->Flags = 0;

    for (VQAFrameNode* frame = data->Loader.CurFrame->Next; frame != data->Loader.CurFrame; frame = frame->Next) {
        frame->Flags = 0;
    }

    data->Drawer.CurFrame = data->Loader.CurFrame;

    if (VQA_PrimeBuffers(handle)) {
        rc = VQAERR_ERROR;
    } else {
        rc = (VQAErrorType)framenum;
    }

    if (audio->Flags & 0x40) {
        VQA_StartAudio(handle);
    }

    return rc;
}

VQAData* VQA_AllocBuffers(VQAHeader* header, VQAConfig* config)
{
    VQACBNode* cbnode;
    VQAFrameNode* framenode;
    VQAFrameNode* last_framenode;
    VQACBNode* last_cbnode;

    if (config->NumCBBufs <= 0 || config->NumFrameBufs <= 0 /*|| config->AudioBufSize < config->HMIBufSize*/) {
        return 0;
    }

    VQAData* data = (VQAData*)malloc(sizeof(VQAData));

    if (data == nullptr) {
        return nullptr;
    }

    memset(data, 0, sizeof(VQAData));
    data->MemUsed = sizeof(VQAData);
    data->Drawer.LastTime = -60;
    data->MaxCBSize = (header->BlockHeight * header->BlockWidth * header->CBentries + 250) & 0xFFFC;
    data->MaxPalSize = 1792;
    data->MaxPtrSize =
        (2 * (header->ImageHeight / header->BlockHeight) * (header->ImageWidth / header->BlockWidth) + 1024) & 0xFFFC;
    data->Loader.LastCBFrame = header->Groupsize * ((header->Frames - 1) / header->Groupsize);

    // BR code
    char color_mode = header->ColorMode;

    if (color_mode == 1 || color_mode == 4) {
        data->MaxCBSize *= 2;
    }

    // Make a linked list of nodes.
    for (int index = 0; index < config->NumCBBufs; ++index) {
        cbnode = (VQACBNode*)malloc(data->MaxCBSize + sizeof(VQACBNode));

        if (cbnode == nullptr) {
            VQA_FreeBuffers(data, config, header);
            return nullptr;
        }

        data->MemUsed += data->MaxCBSize + sizeof(VQACBNode);
        memset(cbnode, 0, sizeof(*cbnode));

        // Set buffer to be the allocated memory in excess of the node struct size.
        cbnode->Buffer = reinterpret_cast<uint8_t*>(&cbnode[1]);

        if (index > 0) {
            last_cbnode->Next = cbnode;
            last_cbnode = cbnode;
        } else {
            data->CBData = cbnode;
            last_cbnode = cbnode;
        }
    }

    // Make a looping list by linking last to first.
    cbnode->Next = data->CBData;
    data->Loader.CurCB = data->CBData;
    data->Loader.FullCB = data->CBData;

    // Make linked list of framenodes.
    for (int index = 0; index < config->NumFrameBufs; ++index) {
        framenode = (VQAFrameNode*)calloc(data->MaxPalSize + data->MaxPtrSize + sizeof(VQAFrameNode), 1);

        if (!framenode) {
            VQA_FreeBuffers(data, config, header);
            return 0;
        }

        data->MemUsed += data->MaxPalSize + data->MaxPtrSize + sizeof(VQAFrameNode);
        framenode->Pointers = reinterpret_cast<uint8_t*>(&framenode[1]);
        framenode->Palette = reinterpret_cast<uint8_t*>(&framenode[1]) + data->MaxPtrSize;
        framenode->Codebook = data->CBData;

        if (index > 0) {
            last_framenode->Next = framenode;
            last_framenode = framenode;
        } else {
            data->FrameData = framenode;
            last_framenode = framenode;
        }
    }

    framenode->Next = data->FrameData;
    data->Loader.CurFrame = data->FrameData;
    data->Drawer.CurFrame = data->FrameData;
    data->Flipper.CurFrame = data->FrameData;

    if (config->ImageBuf != nullptr) {
        data->Drawer.ImageBuf = config->ImageBuf;
        data->Drawer.ImageWidth = config->ImageWidth;
        data->Drawer.ImageHeight = config->ImageHeight;

    } else if (config->DrawFlags & VQACFGF_BUFFER) {
        data->Drawer.ImageBuf = (uint8_t*)malloc(header->ImageHeight * header->ImageWidth);

        if (data->Drawer.ImageBuf == nullptr) {
            VQA_FreeBuffers(data, config, header);
            return nullptr;
        }

        data->Drawer.ImageWidth = header->ImageWidth;
        data->Drawer.ImageHeight = header->ImageHeight;
        data->MemUsed += header->ImageHeight * header->ImageWidth;
    } else {
        data->Drawer.ImageWidth = config->ImageWidth;
        data->Drawer.ImageHeight = config->ImageHeight;
    }

    if (header->Flags & 1) {
        if (config->OptionFlags & 1) {

            VQAAudio* audio = &data->Audio;

            if (header->Version >= 2) {
                int bitspersample;

                if (config->OptionFlags & 0x40 && header->Flags & 2) {
                    audio->SampleRate = header->AltSampleRate;
                    audio->Channels = header->AltChannels;
                    bitspersample = header->AltBitsPerSample;
                } else {
                    audio->SampleRate = header->SampleRate;
                    audio->Channels = header->Channels;
                    bitspersample = header->BitsPerSample;
                }

                audio->BitsPerSample = bitspersample;
                audio->BytesPerSecond = (bitspersample / 8) * header->SampleRate * header->Channels;

            } else {
                audio->SampleRate = 22050;
                audio->Channels = 1;
                audio->BitsPerSample = 8;
                audio->BytesPerSecond = 22050;
            }

            if (config->AudioBufSize == -1) {
                config->AudioBufSize =
                    (audio->BytesPerSecond + (audio->BytesPerSecond / 2)) / config->HMIBufSize * config->HMIBufSize;
            }

            if (config->AudioBufSize <= 0) {
                VQA_FreeBuffers(data, config, header);
                return nullptr;
            }

            if (config->AudioBufSize > 0) {
                if (config->AudioBuf != nullptr) {
                    audio->Buffer = config->AudioBuf;

                } else {
                    audio->Buffer = (uint8_t*)malloc(config->AudioBufSize);
                    if (audio->Buffer == nullptr) {
                        VQA_FreeBuffers(data, config, header);
                        return nullptr;
                    }

                    data->MemUsed += config->AudioBufSize;
                }

                audio->NumAudBlocks = config->AudioBufSize / config->HMIBufSize;
                audio->IsLoaded = (short*)malloc(2 * config->AudioBufSize / config->HMIBufSize);

                if (audio->IsLoaded == nullptr) {
                    VQA_FreeBuffers(data, config, header);
                    return nullptr;
                }

                data->MemUsed += audio->NumAudBlocks * 2;
                memset(audio->IsLoaded, 0, audio->NumAudBlocks * 2);
                audio->TempBufLen = 2 * (audio->BytesPerSecond / header->FPS) + 100;
                audio->TempBuf = (uint8_t*)malloc(2 * (audio->BytesPerSecond / header->FPS) + 100);

                if (audio->TempBuf == nullptr) {
                    VQA_FreeBuffers(data, config, header);
                    return nullptr;
                }

                data->MemUsed += audio->TempBufLen;
            }
        }
    }

    // This looks like Foff is an array of ints
    data->Foff = (int*)malloc(header->Frames * 4);

    if (data->Foff == nullptr) {
        VQA_FreeBuffers(data, config, header);
        return nullptr;
    }

    data->MemUsed += header->Frames * 4;

    return data;
}

void VQA_FreeBuffers(VQAData* data, VQAConfig* config, VQAHeader* header)
{
    if (data->Foff != nullptr) {
        free(data->Foff);
    }

    if (config->AudioBuf == nullptr) {
        if (data->Audio.Buffer != nullptr) {
            free(data->Audio.Buffer);
        }
    }

    if (data->Audio.IsLoaded != nullptr) {
        free(data->Audio.IsLoaded);
    }

    if (data->Audio.TempBuf != nullptr) {
        free(data->Audio.TempBuf);
    }

    if (config->ImageBuf == nullptr) {
        if (data->Drawer.ImageBuf != nullptr) {
            free(data->Drawer.ImageBuf);
        }
    }

    // Free frame nodes.
    VQAFrameNode* frame_this = data->FrameData;

    for (int index = 0; index < config->NumFrameBufs && frame_this; ++index) {
        VQAFrameNode* frame_next = frame_this->Next;
        free(frame_this);
        frame_this = frame_next;
    }

    // Free codebook nodes.
    VQACBNode* cb_this = data->CBData;

    for (int index = 0; index < config->NumCBBufs && cb_this; ++index) {
        VQACBNode* cb_next = cb_this->Next;
        free(cb_this);
        cb_this = cb_next;
    }

    // Free vqa data.
    free(data);
}

int VQA_PrimeBuffers(VQAHandle* handle)
{
    VQAData* data = handle->VQABuf;

    for (int index = 0; index < handle->Config.NumFrameBufs; ++index) {
        VQAErrorType result = (VQAErrorType)VQA_LoadFrame(handle);

        if (result) {
            if (result != VQAERR_NOBUFFER && result != VQAERR_SLEEPING) {
                return result;
            }
        } else {
            ++data->LoadedFrames;
        }
    }

    return VQAERR_NONE;
}
