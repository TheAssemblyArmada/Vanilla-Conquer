#include "wwmouse.h"
#include <string.h>

#ifndef min
#define min(x, y) (x <= y ? x : y)
#endif

typedef struct
{
    long GVPOffset;   // offset to graphic page
    int GVPWidth;     // width of graphic page
    int GVPHeight;    // height of graphic page
    int GVPXAdd;      // xadd for graphic page (0)
    int GVPXPos;      // x offset in relation to graphicbuff
    int GVPYPos;      // y offset in relation to graphicbuff
    long GVPPitch;    // Distance from one line to the next
    void* GVPBuffPtr; // related graphic buff
} GraphicViewPort;

typedef struct
{
    char* MouseCursor;       // pointer to the mouse cursor in memory
    int MouseXHot;           // X hot spot of the current mouse cursor
    int MouseYHot;           // Y hot spot of the current mouse cursor
    int CursorWidth;         // width of the mouse cursor in pixels
    int CursorHeight;        // height of the mouse cursor in pixels

    char* MouseBuffer;       // pointer to background buffer in memory
    int MouseBuffX;          // pixel x mouse buffer was preserved at
    int MouseBuffY;          // pixel y mouse buffer was preserved at
    int MaxWidth;            // maximum width of mouse background buffer
    int MaxHeight;           // maximum height of mouse background buffer

    int MouseCXLeft;         // left x pos if conditional hide mouse in effect
    int MouseCYUpper;        // upper y pos if conditional hide mouse in effect
    int MouseCXRight;        // right x pos if conditional hide mouse in effect
    int MouseCYLower;        // lower y pos if conditional hide mouse in effect
    char MCFlags;            // conditional hide mouse flags
    char MCCount;            // nesting count for conditional hide mouse

    GraphicViewPort* Screen; // pointer to the surface mouse was init'd with
    char* PrevCursor;        // pointer to previous cursor shape
    int MouseUpdate;
    int State;

    char* EraseBuffer; // Buffer which holds background to restore to hidden page
    int EraseBuffX;    // X position of the hidden page background
    int EraseBuffY;    // Y position of the hidden page background
    int EraseBuffHotX; // X position of the hidden page background
    int EraseBuffHotY; // Y position of the hidden page background

    int EraseFlags;    // Records whether mutex has been released
} MouseType;

#pragma pack(push, 1)
typedef struct
{
    unsigned short ShapeType;     // 0 = normal, 1 = 16 colors,
                                  //  2 = uncompressed, 4 = 	<16 colors
    unsigned char Height;         // Height of the shape in scan lines
    unsigned short Width;         // Width of the shape in bytes
    unsigned char OriginalHeight; // Original height of shape in scan lines
    unsigned short ShapeSize;     // Size of the shape, including header
    unsigned short DataLength;    // Size of the uncompressed shape (just data)
} Shape_Type;
#pragma pack(pop)

// Some forward declarations for stuff defined elsewhere.
int Get_Shape_Width(void const* shape);
int Get_Shape_Original_Height(void const* shape);
int Get_Shape_Uncomp_Size(void const* shape);
long LCW_Uncompress(void* source, void* dest, unsigned long length_);
extern char* _ShapeBuffer;

void Mouse_Shadow_Buffer(void* thisptr, void* srcdst, void* buffer, int x, int y, int hotx, int hoty, int store)
{
    MouseType* mouse = (MouseType*)thisptr;
    GraphicViewPort* viewport = (GraphicViewPort*)srcdst;
    int xstart = x - hotx;
    int ystart = y - hoty;
    int yend = mouse->CursorHeight + ystart - 1;
    int xend = mouse->CursorWidth + xstart - 1;
    int ms_img_offset = 0;
    unsigned char* mouse_img = (unsigned char*)(buffer);

    // If we aren't drawing within the viewport, return
    if (xstart >= viewport->GVPWidth || ystart >= viewport->GVPHeight || xend <= 0 || yend <= 0) {
        return;
    }

    if (xstart < 0) {
        ms_img_offset = -xstart;
        xstart = 0;
    }

    if (ystart < 0) {
        mouse_img += mouse->CursorWidth * (-ystart);
        ystart = 0;
    }

    xend = min(xend, viewport->GVPWidth - 1);
    yend = min(yend, viewport->GVPHeight - 1);

    int blit_height = yend - ystart + 1;
    int blit_width = xend - xstart + 1;
    int src_pitch;
    int dst_pitch;
    unsigned char* src;
    unsigned char* dst;

    // Bool will control if we are copying to or from shadow buffer, set vars here
    if (store) {
        src_pitch = (viewport->GVPPitch + viewport->GVPXAdd + viewport->GVPWidth);
        dst_pitch = mouse->CursorWidth;
        src = (unsigned char*)(viewport->GVPOffset) + src_pitch * ystart + xstart;
        dst = mouse_img + ms_img_offset;
    } else {
        src_pitch = mouse->CursorWidth;
        dst_pitch = (viewport->GVPPitch + viewport->GVPXAdd + viewport->GVPWidth);
        src = mouse_img + ms_img_offset;
        dst = (unsigned char*)(viewport->GVPOffset) + dst_pitch * ystart + xstart;
    }

    while (blit_height--) {
        memcpy(dst, src, blit_width);
        src += src_pitch;
        dst += dst_pitch;
    }
}

void Draw_Mouse(void* thisptr, void* srcdst, int x, int y)
{
    MouseType* mouse = (MouseType*)thisptr;
    GraphicViewPort* viewport = (GraphicViewPort*)srcdst;
    int xstart = x - mouse->MouseXHot;
    int ystart = y - mouse->MouseYHot;
    int yend = mouse->CursorHeight + ystart - 1;
    int xend = mouse->CursorWidth + xstart - 1;
    int ms_img_offset = 0;
    unsigned char* mouse_img = mouse->MouseCursor;

    // If we aren't drawing within the viewport, return
    if (xstart >= viewport->GVPWidth || ystart >= viewport->GVPHeight || xend <= 0 || yend <= 0) {
        return;
    }

    if (xstart < 0) {
        ms_img_offset = -xstart;
        xstart = 0;
    }

    if (ystart < 0) {
        mouse_img += mouse->CursorWidth * (-ystart);
        ystart = 0;
    }

    xend = min(xend, viewport->GVPWidth - 1);
    yend = min(yend, viewport->GVPHeight - 1);

    int pitch = viewport->GVPPitch + viewport->GVPXAdd + viewport->GVPWidth;
    unsigned char* dst = xstart + pitch * ystart + (unsigned char*)(viewport->GVPOffset);
    unsigned char* src = ms_img_offset + mouse_img;
    int blit_pitch = xend - xstart + 1;

    if ((xend > xstart) && (yend > ystart)) {
        int blit_height = yend - ystart + 1;
        int dst_pitch = pitch - blit_pitch;
        int src_pitch = mouse->CursorWidth - blit_pitch;

        while (blit_height--) {
            int blit_width = blit_pitch;
            while (blit_width--) {
                unsigned char current = *src++;
                if (current) {
                    *dst = current;
                }

                ++dst;
            }

            src += src_pitch;
            dst += dst_pitch;
        }
    }
}

void* ASM_Set_Mouse_Cursor(void* thisptr, int hotspotx, int hotspoty, void* cursor)
{
    MouseType* mouse = (MouseType*)thisptr;
    Shape_Type* cursor_header = (Shape_Type*)(cursor);
    void* result = NULL;
    unsigned char* cursor_buff;
    unsigned char* data_buff;
    unsigned char* decmp_buff;
    unsigned char* lcw_buff;
    short frame_flags;
    int height;
    int width;
    int uncompsz;

    // Get the dimensions of our frame, mouse shp format can have variable
    // dimensions for each frame.
    uncompsz = Get_Shape_Uncomp_Size(cursor);
    width = Get_Shape_Width(cursor);
    height = Get_Shape_Original_Height(cursor);

    if (width <= mouse->MaxWidth && height <= mouse->MaxHeight) {
        cursor_buff = mouse->MouseCursor;
        data_buff = (unsigned char*)(cursor);
        frame_flags = cursor_header->ShapeType;

        // Flag bit 2 is flag for no compression on frame, decompress to
        // intermediate buffer if flag is clear
        if (!(cursor_header->ShapeType & 2)) {
            decmp_buff = _ShapeBuffer;
            lcw_buff = (unsigned char*)(cursor_header);
            frame_flags = cursor_header->ShapeType | 2;

            memcpy(decmp_buff, lcw_buff, sizeof(Shape_Type));
            decmp_buff += sizeof(Shape_Type);
            lcw_buff += sizeof(Shape_Type);

            // Copies a small lookup table if it exists, probably not in RA.
            if (frame_flags & 1) {
                memcpy(decmp_buff, lcw_buff, 16);
                decmp_buff += 16;
                lcw_buff += 16;
            }

            LCW_Uncompress(lcw_buff, decmp_buff, uncompsz);
            data_buff = _ShapeBuffer;
        }

        if (frame_flags & 1) {
            unsigned char* data_start = data_buff + sizeof(Shape_Type);
            unsigned char* image_start = data_buff + sizeof(Shape_Type) + 16;
            int image_size = height * width;
            int run_len = 0;

            while (image_size) {
                unsigned char current_byte = *image_start++;

                if (current_byte) {
                    *cursor_buff++ = data_start[current_byte];
                    --image_size;
                    continue;
                }

                if (!image_size) {
                    break;
                }

                run_len = *image_start;
                image_size -= run_len;
                ++image_start;

                while (run_len--) {
                    *cursor_buff++ = 0;
                }
            }
        } else {
            unsigned char* data_start = data_buff + sizeof(Shape_Type);
            int image_size = height * width;
            int run_len = 0;

            while (image_size) {
                unsigned char current_byte = *data_start++;

                if (current_byte) {
                    *cursor_buff++ = current_byte;
                    --image_size;
                    continue;
                }

                if (!image_size) {
                    break;
                }

                run_len = *data_start;
                image_size -= run_len;
                ++data_start;

                while (run_len--) {
                    *cursor_buff++ = 0;
                }
            }
        }

        mouse->MouseXHot = hotspotx;
        mouse->MouseYHot = hotspoty;
        mouse->CursorHeight = height;
        mouse->CursorWidth = width;
    }

    result = mouse->PrevCursor;
    mouse->PrevCursor = cursor;

    return result;
}
