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
#include "vqadrawer.h"
//#include "dialog.h"
#include "lcw.h"
#include "unvqbuff.h"
#include "vqacaption.h"
#include "vqafile.h"
#include "vqaloader.h"
#include "vqapalette.h"
#include <string.h>

int VQA_DrawFrame_Buffer(VQAHandle* handle)
{
    VQAConfig* config = &handle->Config;
    VQAData* vqabuf = handle->VQABuf;
    VQADrawer* drawer = &vqabuf->Drawer;
    VQAFrameNode* curframe = drawer->CurFrame;

    if (!(vqabuf->Flags & 2)) {
        int result = VQA_SelectFrame(handle);

        if (result) {
            return result;
        }

        VQA_PrepareFrame(vqabuf);
    }

    if (vqabuf->Flags & 1) {
        vqabuf->Flags |= 2;
        return VQAERR_SLEEPING;
    }

    if (vqabuf->Flags & 2) {
        ++vqabuf->Drawer.WaitsOnFlipper;
        vqabuf->Flags &= 0xFFFFFFFD;
    }

    if ((curframe->Flags & 4) || (vqabuf->Drawer.Flags & 1)) {
        VQA_Flag_To_Set_Palette(
            curframe->Palette, curframe->PaletteSize, (handle->Config.OptionFlags & VQAOPTF_SLOWPAL));
        curframe->Flags &= ~4;
        vqabuf->Drawer.Flags &= ~1;
    }

    vqabuf->UnVQ(curframe->Codebook->Buffer,
                 curframe->Pointers,
                 vqabuf->Drawer.ScreenOffset + vqabuf->Drawer.ImageBuf,
                 vqabuf->Drawer.BlocksPerRow,
                 vqabuf->Drawer.NumRows,
                 vqabuf->Drawer.ImageWidth);

    vqabuf->Drawer.LastFrameNum = curframe->FrameNum;
    vqabuf->Flipper.CurFrame = curframe;
    vqabuf->Flags |= 1;

    if (config->DrawerCallback != nullptr) {
        if (config->DrawerCallback(vqabuf->Drawer.ImageBuf, curframe->FrameNum)) {
            return VQAERR_ERROR;
        }
    }

    drawer->CurFrame = curframe->Next;

    return VQAERR_NONE;
}

int DrawFrame_Nop(VQAHandle* handle)
{
    return VQAERR_NONE;
}

int PageFlip_Nop(VQAHandle* handle)
{
    return VQAERR_NONE;
}

uint8_t* VQA_GetPalette(VQAHandle* handle)
{
    uint8_t* palette = 0;

    if (handle->VQABuf->Drawer.CurPalSize > 0) {
        palette = handle->VQABuf->Drawer.Palette;
    }

    return palette;
}

int VQA_GetPaletteSize(VQAHandle* handle)
{
    return handle->VQABuf->Drawer.CurPalSize;
}

// wip replacement for identical chunks in SetDrawBuffer and ConfigureDrawer
void VQA_SetDrawRect(VQAHandle* handle, unsigned width, unsigned height, int xpos, int ypos)
{
    VQAHeader* header = &handle->Header;
    VQAData* vqabuf = handle->VQABuf;
    VQAConfig* config = &handle->Config;
    unsigned flags_1 = config->DrawFlags & 0x30;

    if (xpos != -1 || ypos != -1) {
        if (flags_1 < 32) {
            vqabuf->Drawer.Y1 = ypos;
            vqabuf->Drawer.X1 = xpos;
            vqabuf->Drawer.X2 = vqabuf->Drawer.X1 + header->ImageWidth - 1;
            vqabuf->Drawer.Y2 = vqabuf->Drawer.Y1 + header->ImageHeight - 1;
        }

        if (flags_1 <= 32) {
            vqabuf->Drawer.Y1 = height - ypos;
            vqabuf->Drawer.X1 = width - xpos;
            vqabuf->Drawer.X2 = vqabuf->Drawer.X1 - header->ImageWidth;
            vqabuf->Drawer.Y2 = vqabuf->Drawer.Y1 - header->ImageHeight;
        }

        if (flags_1 != 48) {
            vqabuf->Drawer.Y1 = ypos;
            vqabuf->Drawer.X1 = xpos;
            vqabuf->Drawer.X2 = vqabuf->Drawer.X1 + header->ImageWidth - 1;
            vqabuf->Drawer.Y2 = vqabuf->Drawer.Y1 + header->ImageHeight - 1;
        }

        vqabuf->Drawer.X1 = xpos;
        vqabuf->Drawer.Y1 = height - ypos;
        vqabuf->Drawer.X2 = vqabuf->Drawer.X1 + header->ImageWidth - 1;
        vqabuf->Drawer.Y2 = vqabuf->Drawer.Y2 - header->ImageHeight - 1;
    } else {
        vqabuf->Drawer.X1 = (width - header->ImageWidth) / 2;
        vqabuf->Drawer.Y1 = (height - header->ImageHeight) / 2;
        vqabuf->Drawer.X2 = vqabuf->Drawer.X1 + header->ImageWidth - 1;
        vqabuf->Drawer.Y2 = vqabuf->Drawer.Y1 + header->ImageHeight - 1;
    }
}

void VQA_SetDrawBuffer(VQAHandle* handle, uint8_t* buffer, unsigned width, unsigned height, int xpos, int ypos)
{
    VQADrawer* drawer = &handle->VQABuf->Drawer;
    unsigned flags = handle->Config.DrawFlags & 0x30;

    drawer->ImageBuf = buffer;
    drawer->ImageWidth = width;
    drawer->ImageHeight = height;

    if (xpos == -1 || ypos == -1) {
        drawer->X1 = (width - handle->Header.ImageWidth) >> 1;
        drawer->Y1 = (height - handle->Header.ImageHeight) >> 1;
        drawer->X2 = drawer->X1 + handle->Header.ImageWidth - 1;
        drawer->Y2 = drawer->Y1 + handle->Header.ImageHeight - 1;
    } else {
        if (flags == 0) {
            drawer->X1 = xpos;
            drawer->Y1 = ypos;
            drawer->X2 = drawer->X1 + handle->Header.ImageWidth - 1;
            drawer->Y2 = drawer->Y1 + handle->Header.ImageHeight - 1;
        } else if (flags == 0x20) {
            drawer->X1 = width - xpos;
            drawer->Y1 = height - ypos;
            drawer->X2 = drawer->X1 - handle->Header.ImageWidth;
            drawer->Y2 = drawer->Y1 - handle->Header.ImageHeight;
        } else if (flags == 0x30) {
            drawer->X1 = xpos;
            drawer->Y1 = ypos;
            drawer->X2 = drawer->X1 + handle->Header.ImageWidth - 1;
            drawer->Y2 = drawer->Y1 + handle->Header.ImageHeight - 1;
        } else {
            drawer->X1 = xpos;
            drawer->Y1 = height - ypos;
            drawer->X2 = drawer->X1 + handle->Header.ImageWidth - 1;
            drawer->Y2 = drawer->Y2 - handle->Header.ImageHeight - 1;
        }
    }

    drawer->ScreenOffset = drawer->X1 + drawer->Y1 * width;
}

void VQA_ConfigureDrawer(VQAHandle* handle)
{
    VQAData* data = handle->VQABuf;
    unsigned flags = handle->Config.DrawFlags & 0x30;

    if (handle->Config.X1 == -1 && handle->Config.Y1 == -1) {
        data->Drawer.X1 = (data->Drawer.ImageWidth - handle->Header.ImageWidth) / 2;
        data->Drawer.Y1 = (data->Drawer.ImageHeight - handle->Header.ImageHeight) / 2;
        data->Drawer.X2 = data->Drawer.X1 + handle->Header.ImageWidth - 1;
        data->Drawer.Y2 = data->Drawer.Y1 + handle->Header.ImageHeight - 1;
    } else {
        if (flags == 0) {
            data->Drawer.X1 = handle->Config.X1;
            data->Drawer.Y1 = handle->Config.Y1;
            data->Drawer.X2 = data->Drawer.X1 + handle->Header.ImageWidth - 1;
            data->Drawer.Y2 = data->Drawer.Y1 + handle->Header.ImageHeight - 1;
        } else if (flags == 0x20) {
            data->Drawer.X1 = data->Drawer.ImageWidth - handle->Config.X1;
            data->Drawer.Y1 = data->Drawer.ImageHeight - handle->Config.Y1;
            data->Drawer.X2 = data->Drawer.X1 - handle->Header.ImageWidth;
            data->Drawer.Y2 = data->Drawer.Y1 - handle->Header.ImageHeight;
        } else if (flags != 0x30) {
            data->Drawer.X1 = handle->Config.X1;
            data->Drawer.Y1 = handle->Config.Y1;
            data->Drawer.X2 = data->Drawer.X1 + handle->Header.ImageWidth - 1;
            data->Drawer.Y2 = data->Drawer.Y1 + handle->Header.ImageHeight - 1;
        } else {
            data->Drawer.X1 = handle->Config.X1;
            data->Drawer.Y1 = data->Drawer.ImageHeight - handle->Config.Y1;
            data->Drawer.X2 = data->Drawer.X1 + handle->Header.ImageWidth - 1;
            data->Drawer.Y2 = data->Drawer.Y2 - handle->Header.ImageHeight - 1;
        }
    }

    data->Drawer.BlocksPerRow = handle->Header.ImageWidth / handle->Header.BlockWidth;
    data->Drawer.NumRows = handle->Header.ImageHeight / handle->Header.BlockHeight;
    data->Drawer.NumBlocks = data->Drawer.NumRows * data->Drawer.BlocksPerRow;
    int blocksize = handle->Header.BlockHeight | (handle->Header.BlockWidth << 8);
    data->UnVQ = UnVQ_Nop;
    data->Draw_Frame = DrawFrame_Nop;
    data->Page_Flip = PageFlip_Nop;

    if (handle->Config.DrawFlags & VQACFGF_BUFFER) {
        switch (blocksize) {
        case 0x402:
            data->UnVQ = UnVQ_4x2;
            break;
        case 0x404:
            data->UnVQ = UnVQ_4x4;
            break;
        default:
            break;
        };
    }

    data->Draw_Frame = VQA_DrawFrame_Buffer;
    data->Drawer.ScreenOffset = data->Drawer.X1 + data->Drawer.Y1 * data->Drawer.ImageWidth;
}

int VQA_SelectFrame(VQAHandle* handle)
{
    int result;
    VQAConfig* config = &handle->Config;
    VQAData* vqabuf = handle->VQABuf;
    VQAFrameNode* curframe = vqabuf->Drawer.CurFrame;

    if (!(curframe->Flags & 1)) {
        ++vqabuf->Drawer.WaitsOnLoader;
        return VQAERR_NOBUFFER;
    }

    if (handle->Config.OptionFlags & 2) {
        vqabuf->Drawer.LastFrame = curframe->FrameNum;
        return VQAERR_NONE;
    }

    unsigned curtime = VQA_GetTime(handle);
    int desiredframe = handle->Config.DrawRate * curtime / 60;
    vqabuf->Drawer.DesiredFrame = desiredframe;

    if (handle->Config.DrawRate == handle->Config.FrameRate) {
        if (curframe->FrameNum > desiredframe) {
            return VQAERR_NOT_TIME;
        }
    } else if (60u / handle->Config.DrawRate > curtime - vqabuf->Drawer.LastTime) {
        return VQAERR_NOT_TIME;
    }

    if (handle->Config.FrameRate / 5 > curframe->FrameNum - vqabuf->Drawer.LastFrame) {
        if (handle->Config.DrawFlags & 4) {
            vqabuf->Drawer.LastFrame = curframe->FrameNum;
            result = VQAERR_NONE;

        } else {
            while (true) {
                if (!(curframe->Flags & 1)) {
                    return VQAERR_NOBUFFER;
                }

                if (curframe->Flags & 2 || curframe->FrameNum >= desiredframe) {
                    break;
                }

                if (curframe->Flags & 4) {
                    if (curframe->Flags & 8) {
                        curframe->PaletteSize = LCW_Uncompress((curframe->PalOffset + curframe->Palette),
                                                               curframe->Palette,
                                                               vqabuf->MaxPalSize); // Beta line, was replaced with
                        curframe->Flags &= 0xFFFFFFF7;
                    }

                    memcpy(vqabuf->Drawer.Palette, curframe->Palette, curframe->PaletteSize);
                    vqabuf->Drawer.CurPalSize = curframe->PaletteSize;
                    vqabuf->Drawer.Flags |= 1u;
                }

                if (config->DrawerCallback != nullptr) {
                    config->DrawerCallback(nullptr, curframe->FrameNum);
                }

                curframe->Flags = 0;
                curframe = curframe->Next;
                vqabuf->Drawer.CurFrame = curframe;
                ++vqabuf->Drawer.NumSkipped;
            }

            vqabuf->Drawer.LastFrame = curframe->FrameNum;
            vqabuf->Drawer.LastTime = curtime;
            result = VQAERR_NONE;
        }
    } else {
        vqabuf->Drawer.LastFrame = curframe->FrameNum;
        result = VQAERR_NONE;
    }

    return result;
}

void VQA_PrepareFrame(VQAData* vqabuf)
{
    VQAFrameNode* curframe = vqabuf->Drawer.CurFrame;
    VQACBNode* codebook = curframe->Codebook;

    if (codebook->Flags & 0x02) {
        LCW_Uncompress(&codebook->Buffer[codebook->CBOffset], codebook->Buffer, vqabuf->MaxCBSize);
        codebook->Flags &= ~0x02;
    }

    if (curframe->Flags & 0x08) {
        curframe->PaletteSize =
            LCW_Uncompress(&curframe->Palette[curframe->PalOffset], curframe->Palette, vqabuf->MaxPalSize);
        curframe->Flags &= ~0x08;
    }

    if (curframe->Flags & 0x10) {
        LCW_Uncompress(&curframe->Pointers[curframe->PtrOffset], curframe->Pointers, vqabuf->MaxPtrSize);
        curframe->Flags &= ~0x10;
    }
}
