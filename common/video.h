#ifndef VIDEO_H
#define VIDEO_H

#include "rect.h"
#include <cstdint>

enum GBC_Enum
{
    GBC_NONE = 0,
    GBC_VIDEOMEM = 1,
    GBC_VISIBLE = 2,
};

class VideoSurface;

class Video
{
public:
    Video();
    virtual ~Video();

    static Video& Shared();

    VideoSurface* CreateSurface(int w, int h, GBC_Enum flags);
};

class VideoSurface
{
public:
    virtual void* GetData() const = 0;
    virtual long GetPitch() const = 0;
    virtual bool IsAllocated() const = 0;

    virtual void AddAttachedSurface(VideoSurface* surface) = 0;
    virtual bool IsReadyToBlit() = 0;
    virtual bool LockWait() = 0;
    virtual bool Unlock() = 0;
    virtual void Blt(const Rect& destRect, VideoSurface* src, const Rect& srcRect, bool mask) = 0;
    virtual void FillRect(const Rect& rect, unsigned char color) = 0;
};

class SurfaceMonitorClass
{
public:
    SurfaceMonitorClass();

    virtual void Restore_Surfaces() = 0;
    virtual void Set_Surface_Focus(bool in_focus) = 0;
    virtual void Release() = 0;

    bool SurfacesRestored;
};

extern SurfaceMonitorClass& AllSurfaces; // List of all surfaces

bool Set_Video_Mode(int w, int h, int bits_per_pixel);
bool Is_Video_Fullscreen();
void Reset_Video_Mode();
unsigned Get_Free_Video_Memory();
void Wait_Blit();

/*
** Set desired cursor image in game palette.
*/
void Set_Video_Cursor(void* cursor, int w, int h, int hotx, int hoty);

/*
 *  Flags returned by Get_Video_Hardware_Capabilities
 */
/* Hardware blits supported? */
#define VIDEO_BLITTER 1

/* Hardware blits asyncronous? */
#define VIDEO_BLITTER_ASYNC 2

/* Can palette changes be synced to vertical refresh? */
#define VIDEO_SYNC_PALETTE 4

/* Is the video cards memory bank switched? */
#define VIDEO_BANK_SWITCHED 8

/* Can the blitter do filled rectangles? */
#define VIDEO_COLOR_FILL 16

/* Is there no hardware assistance avaailable at all? */
#define VIDEO_NO_HARDWARE_ASSIST 32

unsigned Get_Video_Hardware_Capabilities();

void Wait_Vert_Blank();
void Set_DD_Palette(void* palette);

#endif // VIDEO_H