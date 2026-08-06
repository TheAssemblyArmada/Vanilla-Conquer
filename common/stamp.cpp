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
#include "tile.h"
#include "endianness.h"
#include "graphicsviewport.h"
#include <string.h>
#include <stdint.h>

#define TD_TILESET_CHECK 0x20

int IconEntry;
void* IconData;
const IControl_Type* LastIconset;
const uint8_t* StampPtr;
const uint8_t* TransFlagPtr;
const uint8_t* MapPtr;
int IconWidth;
int IconHeight;
int IconSize;
int IconCount;
int TDIcons;

void Init_Stamps(const IControl_Type* iconset)
{
    if (iconset && LastIconset != iconset) {
        IconCount = le16toh(iconset->Count);
        IconWidth = le16toh(iconset->Width);
        IconHeight = le16toh(iconset->Height);
        LastIconset = iconset;
        IconSize = IconWidth * IconHeight;

        // TD and RA tileset headers are slightly different, so check a constant that only exists in one type.
        if (iconset->Is_CC_IconSet()) {
            MapPtr = reinterpret_cast<const uint8_t*>(iconset) + le32toh(iconset->CC.Map);
            StampPtr = reinterpret_cast<const uint8_t*>(iconset) + le32toh(iconset->CC.Icons);
            TransFlagPtr = reinterpret_cast<const uint8_t*>(iconset) + le32toh(iconset->CC.TransFlag);
        } else {
            MapPtr = reinterpret_cast<const uint8_t*>(iconset) + le32toh(iconset->RA.Map);
            StampPtr = reinterpret_cast<const uint8_t*>(iconset) + le32toh(iconset->RA.Icons);
            TransFlagPtr = reinterpret_cast<const uint8_t*>(iconset) + le32toh(iconset->RA.TransFlag);
        }
    }
}

void Buffer_Draw_Stamp(void* thisptr, void* icondata, int icon, int x, int y, const void* remapper)
{
    GraphicViewPortClass& viewport = *static_cast<GraphicViewPortClass*>(thisptr);
    IControl_Type* tileset = static_cast<IControl_Type*>(icondata);

    if (!tileset) {
        return;
    }

    if (LastIconset != tileset) {
        Init_Stamps(tileset);
    }

    int32_t icon_index = MapPtr != nullptr ? MapPtr[icon] : icon;

    if (icon_index < IconCount) {

        int32_t fullpitch = viewport.Get_Pitch() + viewport.Get_XAdd() + viewport.Get_Width();
        uint8_t* dst = x + y * fullpitch + reinterpret_cast<uint8_t*>(viewport.Get_Offset());
        int32_t blitpitch = fullpitch - IconWidth;
        const uint8_t* src = &StampPtr[IconSize * icon_index];

        if (remapper) {
            const uint8_t* remap = static_cast<const uint8_t*>(remapper);
            for (int i = 0; i < IconHeight; ++i) {
                for (int j = 0; j < IconWidth; ++j) {
                    uint8_t cur_byte = remap[*src++];

                    if (cur_byte) {
                        *dst = cur_byte;
                    }

                    ++dst;
                }

                dst += blitpitch;
            }

        } else if (TransFlagPtr[icon_index]) {
            for (int i = 0; i < IconHeight; ++i) {
                for (int j = 0; j < IconWidth; ++j) {
                    uint8_t cur_byte = *src++;

                    if (cur_byte) {
                        *dst = cur_byte;
                    }

                    ++dst;
                }

                dst += blitpitch;
            }
        } else {
            for (int32_t i = 0; i < IconHeight; ++i) {
                memcpy(dst, src, IconWidth);
                dst += fullpitch;
                src += IconWidth;
            }
        }
    }
}

void Buffer_Draw_Stamp_Clip(void const* thisptr,
                            void const* icondata,
                            int icon,
                            int x,
                            int y,
                            const void* remapper,
                            int left,
                            int top,
                            int right,
                            int bottom)
{
    const GraphicViewPortClass& viewport = *static_cast<const GraphicViewPortClass*>(thisptr);
    const IControl_Type* tileset = static_cast<const IControl_Type*>(icondata);

    if (!tileset) {
        return;
    }

    if (LastIconset != tileset) {
        Init_Stamps(tileset);
    }

    int icon_index = MapPtr != nullptr ? MapPtr[icon] : icon;

    if (icon_index < IconCount) {
        int blit_height = IconHeight;
        int blit_width = IconWidth;
        const uint8_t* src = &StampPtr[IconSize * icon_index];
        int width = left + right;
        int xstart = left + x;
        int height = top + bottom;
        int ystart = top + y;

        if (xstart < width && ystart < height && IconHeight + ystart > top && IconWidth + xstart > left) {
            if (xstart < left) {
                src += left - xstart;
                blit_width -= left - xstart;
                xstart = left;
            }

            int src_pitch = IconWidth - blit_width;

            if (blit_width + xstart > width) {
                src_pitch += blit_width - (width - xstart);
                blit_width = width - xstart;
            }

            if (top > ystart) {
                blit_height = IconHeight - (top - ystart);
                src += IconWidth * (top - ystart);
                ystart = top;
            }

            if (blit_height + ystart > height) {
                blit_height = height - ystart;
            }

            int full_pitch = viewport.Get_Pitch() + viewport.Get_XAdd() + viewport.Get_Width();
            uint8_t* dst = xstart + ystart * full_pitch + reinterpret_cast<uint8_t*>(viewport.Get_Offset());
            int dst_pitch = full_pitch - blit_width;

            if (remapper) {
                const uint8_t* remap = static_cast<const uint8_t*>(remapper);
                for (int i = 0; i < blit_height; ++i) {
                    for (int j = 0; j < blit_width; ++j) {
                        uint8_t cur_byte = remap[*src++];
                        if (cur_byte) {
                            *dst = cur_byte;
                        }

                        ++dst;
                    }
                    dst += dst_pitch;
                }

            } else if (TransFlagPtr[icon_index]) {
                for (int i = 0; i < blit_height; ++i) {
                    for (int j = 0; j < blit_width; ++j) {
                        uint8_t cur_byte = *src++;
                        if (cur_byte) {
                            *dst = cur_byte;
                        }

                        ++dst;
                    }
                    src += src_pitch;
                    dst += dst_pitch;
                }

            } else {
                for (int i = 0; i < blit_height; ++i) {
                    memcpy(dst, src, blit_width);
                    dst += full_pitch;
                    src += IconWidth;
                }
            }
        }
    }
}
