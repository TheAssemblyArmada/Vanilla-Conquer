
#include "gbuffer.h"
#include "palette.h"
#include "video.h"
#include "video.h"
#include "wwkeyboard.h"
#include "wwmouse.h"
#include <cstdio>
#include <nds.h>
#include <stdarg.h>

/** Video interface for the Nintendo DSi.
  *
  * Author: mrparrot (aka giulianob)
  *
  * This video engine provides a drawable surface to the game's engine by
  * setting up a 512x256 8-bit background. When the game is ready to draw
  * to the screen, the game `Blt` the 'HidBuff' into this final plane
  * (SeenBuff, or frontSurface). We also provide functions for Zooming in
  * and out the screen, as the game expect at least a 320x200 screen, but
  * the DSi resolution is 256x192.  The default mode is use the GPU's
  * affine transformations to 'compress' the background in order to fit
  * into screen, with the consequence of losing a few lines.
  **/

// Uncomment this to show FPS on screen.
//#define SHOW_FPS

/* Function used to pause the console for debugging.  */
void DS_Pause(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    while (1) {
        swiWaitForVBlank();
        scanKeys();
        int keys = keysDown();
        if (keys & KEY_A)
            break;
    }
}

// Background 3 identifier. Set on video initialization, and used on the seen
// surface buffer.
static int bg3;

// Mark the status of zoom. True means we are zoomed in, false means we are
// zoomed out.
static bool ZoomState = false;

// Zoom in the Seen Surface.
void DS_SeenBuff_ZoomIn(void)
{
    int scale_x = (SCREEN_WIDTH * 256) / SCREEN_WIDTH;
    int scale_y = (SCREEN_HEIGHT * 256) / SCREEN_HEIGHT;

    bgSetRotateScale(bg3, 0, scale_x, scale_y);
    bgSetScroll(bg3, 32, 4);

    ZoomState = true;

    swiWaitForVBlank();
    bgUpdate();
}

// Zoom out the Seen Surface.
void DS_SeenBuff_ZoomOut(void)
{
    int scale_x = (320 * 256) / SCREEN_WIDTH;
    int scale_y = (200 * 256) / SCREEN_HEIGHT;

    bgSetRotateScale(bg3, 0, scale_x, scale_y);
    bgSetScroll(bg3, 0, 0);

    ZoomState = false;

    swiWaitForVBlank();
    bgUpdate();
}

void DS_SeenBuff_SwitchZoom()
{
    if (ZoomState == true) {
        DS_SeenBuff_ZoomOut();
    } else {
        DS_SeenBuff_ZoomIn();
    }
}

/*
 * We implement a 'hardware' cursor by using a sprite.  It looks better than
 * blitting the cursor into the seenbuff.
 */
class HardwareCursor
{
public:
    void Init()
    {

        // Allocate 16Kb for the mouse sprites.
        vramSetBankG(VRAM_G_MAIN_SPRITE_0x06400000);

        // Initialize the sprte engine.
        oamInit(&oamMain, SpriteMapping_1D_256, false);
        Surface = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_256Color);

        X = 160;
        Y = 100;

        oamSet(&oamMain,
               0,
               X,
               Y,
               0,
               0,
               SpriteSize_32x32,
               SpriteColorFormat_256Color,
               Surface,
               0,
               false,
               false,
               false,
               false,
               false);

        // Disable sprite scaling and rotating, we won't need it and we require
        // it to be disabled to hide the sprite.
        oamMain.oamMemory->isRotateScale = false;
    }

    inline void Set_Cursor_Palette(const u16* palette)
    {
        // Copy the palette to the sprite engine.
        dmaCopy(palette, SPRITE_PALETTE, 2 * 256);
    }

    inline void Set_Video_Cursor(void* cursor, int w, int h, int hotx, int hoty)
    {
        Raw = cursor;
        W = w;
        H = h;
        HotX = hotx;
        HotY = hoty;

        uint8_t* src = (uint8_t*)Raw;
        uint8_t* dst = (uint8_t*)Surface;

        // Mouse sprites aren't stored in VRAM so we only copy the new mouse
        // shape in case it changes.
        if (Raw != Last_Raw) {
            Raw = Last_Raw;

            // DS sprites are tiled, so we remap the texture to be displayed
            // correctly.
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    for (int ii = 0; ii < 8; ii++) {
                        for (int jj = 0; jj < 8; jj++) {
                            int real_j = 8 * j + jj;
                            int real_i = 8 * i + ii;

                            if (real_j < w && real_i < h)
                                dst[256 * i + 8 * ii + 64 * j + jj] = src[real_i * w + real_j];
                        }
                    }
                }
            }
        }
    }

    inline void Get_Video_Mouse(int& x, int& y)
    {
        x = X;
        y = Y;
    }

    inline void Set_Video_Cursor_Clip(bool clipped)
    {
        Clip = clipped;
    }

    inline void Update_HWCursor()
    {
        /* Update sprite representing the mouse cursor.  */
        int x_scaled;
        int y_scaled;

        // Transform the coordinates to show the cursor in the correct
        // screen position.
        if (ZoomState) {
            x_scaled = (X - HotX) - 32;
            y_scaled = (Y - HotY) - 4;
        } else {
            x_scaled = ((X - HotX) * SCREEN_WIDTH) / 320;
            y_scaled = ((Y - HotY) * SCREEN_HEIGHT) / 200;
        }

        // Hide or show the cursor accordingly.
        oamMain.oamMemory->isHidden = Get_Mouse_State();

        // Update cursor sprite position
        oamSetXY(&oamMain, 0, x_scaled, y_scaled);
        oamUpdate(&oamMain);
    }

    void Ensure_Mouse_Boundary(void)
    {
        if (X >= 320) {
            X = 319;
        } else if (X < 0) {
            X = 0;
        }

        if (Y >= 200) {
            Y = 199;
        } else if (Y < 0) {
            Y = 0;
        }
    }

    void Move_Video_Mouse(int xrel, int yrel)
    {
        X += xrel;
        Y += yrel;

        Ensure_Mouse_Boundary();
    }

    void Set_Video_Mouse(int x, int y)
    {
        if (ZoomState) {
            X = x + 32;
            Y = y + 4;
        } else {
            X = (x * 320) / SCREEN_WIDTH;
            Y = (y * 200) / SCREEN_HEIGHT;
        }

        // We don't need to ensure it.  The input comes from the screen.
        // Ensure_Mouse_Boundary();
    }

private:
    void* Raw;      // Stores the cursor sprite untouched.
    void* Surface;  // Stores the sprite in 8x8 tiled format for the DS.
    void* Last_Raw; // Stores the pointer to the last used sprite.

    bool Clip; // Flag to show or hide the mouse.
    int X;     // X position.
    int Y;
    int W;    // Width.
    int H;    // Height
    int HotX; // Cursor bias according to icon.
    int HotY;
};

static HardwareCursor HWCursor;

// Unsused.  Required by the game's engine.
class SurfaceMonitorClassNDS : public SurfaceMonitorClass
{

public:
    SurfaceMonitorClassNDS()
    {
    }

    virtual void Restore_Surfaces()
    {
    }

    virtual void Set_Surface_Focus(bool in_focus)
    {
    }

    virtual void Release()
    {
    }
};

SurfaceMonitorClassNDS AllSurfacesNDS;             // List of all direct draw surfaces
SurfaceMonitorClass& AllSurfaces = AllSurfacesNDS; // List of all direct draw surfaces

// VBlank timer function.
void Timer_VBlank(void);

void On_VBlank()
{
    // Update the number of ticks passed.  We use this to implement a 60FPS
    // timer. Altough vanilla-conquer uses a milliseconds based timer, original
    // game used a 60FPS timer.
    Timer_VBlank();
}

bool Set_Video_Mode(int w, int h, int bits_per_pixel)
{
    // Allocate memory for our console.  This has to be static because it must
    // persists when this function exit, even if only used here.
    static PrintConsole cs0;

    // Only turns on the 2D engine. The 3D chip is unused and disabling it
    // should save battery life.
    powerOn(POWER_ALL_2D);

    // If the ARM9 is set to 67MHz, set it to 133MHz now.
    setCpuClock(true);

#ifdef SHOW_FPS
    // Start timer 0, used to measure FPS.
    timerStart(0, ClockDivider_1024, 0, NULL);
#endif

    // Allocate 128Kb for the console on the upper screen.  It is a bit
    // overkill, but we got plenty of VRAM so far so it is OK.
    vramSetBankC(VRAM_C_SUB_BG_0x06200000);
    videoSetModeSub(MODE_0_2D);

    // Initialize the console on the top screen.
    consoleInit(&cs0, 0, BgType_Text4bpp, BgSize_T_256x256, 2, 0, false, true);

    // Setup what should run on a VBlank interrupt.
    irqSet(IRQ_VBLANK, On_VBlank);

    // Install the default Nintendo DS exception handler. It sucks, but that is
    // what we got.
    defaultExceptionHandler();

    // Allocate 128kb of VRAM for the background that will hold the visible surface.
    // The backgroung is 512x256, which is 128Kb, a full memory bank.
    vramSetBankA(VRAM_A_MAIN_BG_0x06000000);

    // Put DS into 2D mode with extended background scaling/rotation support. This
    // is necessary so we can downscale the game to fit into the DS low resolution
    // screen.
    videoSetMode(MODE_3_2D | DISPLAY_BG3_ACTIVE);
    bg3 = bgInit(3, BgType_Bmp8, BgSize_B8_512x256, 0, 0);

    // Set downscaling 320x200 => 256x192
    DS_SeenBuff_ZoomOut();

    // Initialize the Hardware cursor, which is basically a spite that is
    // displayed on top of the background.
    HWCursor.Init();

    // Swap the LCD screen so that the main 2D engine is on the bottom screen.
    lcdSwap();

    if (w != 320 || h != 200 || bits_per_pixel != 8)
        return false;

    return true;
}

bool Is_Video_Fullscreen()
{
    return true;
}

/***********************************************************************************************
 * Reset_Video_Mode -- Resets video mode and deletes Direct Draw Object                        *
 *                                                                                             *
 * INPUT:		none                                                                            *
 *                                                                                             *
 * OUTPUT:     none                                                                            *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/26/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
void Reset_Video_Mode(void)
{
}

/***********************************************************************************************
 * Get_Free_Video_Memory -- returns amount of free video memory                                *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   bytes of available video RAM                                                      *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/29/95 12:52PM ST : Created                                                            *
 *=============================================================================================*/
unsigned int Get_Free_Video_Memory(void)
{
    return 1000000000;
}

/***********************************************************************************************
 * Get_Video_Hardware_Caps -- returns bitmask of direct draw video hardware support            *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   hardware flags                                                                    *
 *                                                                                             *
 * WARNINGS: Must call Set_Video_Mode 1st to create the direct draw object                     *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    1/12/96 9:14AM ST : Created                                                              *
 *=============================================================================================*/
unsigned Get_Video_Hardware_Capabilities(void)
{
    return 0;
}

/***********************************************************************************************
 * Wait_Vert_Blank -- Waits for the start (leading edge) of a vertical blank                   *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *=============================================================================================*/
void Wait_Vert_Blank(void)
{
    swiWaitForVBlank();
}

/***********************************************************************************************
 * Set_Palette -- set a direct draw palette                                                    *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to 768 rgb palette bytes                                                      *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    10/11/95 3:33PM ST : Created                                                             *
 *=============================================================================================*/
void Set_DD_Palette(void* palette)
{
    unsigned char r, g, b;

    unsigned char* rcolors = (unsigned char*)palette;
    for (int i = 0; i < 256; i++) {
        r = (unsigned char)rcolors[i * 3] << 2;
        g = (unsigned char)rcolors[i * 3 + 1] << 2;
        b = (unsigned char)rcolors[i * 3 + 2] << 2;

        BG_PALETTE[i] = RGB8(r, g, b);
    }

    HWCursor.Set_Cursor_Palette(BG_PALETTE);
}

void Wait_Blit(void)
{
}

void Set_Video_Cursor_Clip(bool clipped)
{
    HWCursor.Set_Video_Cursor_Clip(clipped);
}

void Get_Video_Mouse(int& x, int& y)
{
    HWCursor.Get_Video_Mouse(x, y);
}

void Set_Video_Mouse(int x, int y)
{
    HWCursor.Set_Video_Mouse(x, y);
}

void Set_Video_Cursor(void* cursor, int w, int h, int hotx, int hoty)
{
    HWCursor.Set_Video_Cursor(cursor, w, h, hotx, hoty);
}

/***********************************************************************************************
 * SMC::SurfaceMonitorClass -- constructor for surface monitor class                           *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/3/95 3:23PM ST : Created                                                              *
 *=============================================================================================*/

SurfaceMonitorClass::SurfaceMonitorClass()
{
    SurfacesRestored = false;
}

void Update_HWCursor()
{
    HWCursor.Update_HWCursor();
}

/*
** VideoSurfaceDDraw
*/

class VideoSurfaceNDS;
static VideoSurfaceNDS* frontSurface = nullptr;

class VideoSurfaceNDS : public VideoSurface
{
public:
    VideoSurfaceNDS(int w, int h, GBC_Enum flags)
        : flags(flags)
        , windowSurface(nullptr)
    {
        if (w == 320 && h == 200) {
            // The DS renderer works as follows: we pass the background
            // buffer in VRAM to the game's software engine, which draws
            // things there. The background is a 512x256 surface, but
            // only 512x200 pixels are used.

            if (flags & GBC_VISIBLE) {
                Pitch = 512;
                surface = (char*)bgGetGfxPtr(bg3);
                windowSurface = surface;
                frontSurface = this;
            }
        } else {
            swiWaitForVBlank();
            printf("ERROR - Unsupported surface size\n");
            while (1)
                ;
        }
    }

    virtual ~VideoSurfaceNDS()
    {
        if (frontSurface == this) {
            frontSurface = NULL;
        }
    }

    virtual void* GetData() const
    {
        return surface;
    }
    virtual int GetPitch() const
    {
        return Pitch;
    }
    virtual bool IsAllocated() const
    {
        return false;
    }

    virtual void AddAttachedSurface(VideoSurface* surface)
    {
    }

    virtual bool IsReadyToBlit()
    {
        return false;
    }

    virtual bool LockWait()
    {
        return true;
    }

    virtual bool Unlock()
    {
        return true;
    }

    virtual void Blt(const Rect& destRect, VideoSurface* src, const Rect& srcRect, bool mask)
    {
    }

    virtual void FillRect(const Rect& rect, unsigned char color)
    {
    }

    inline void RenderSurface()
    {
    }

private:
    int Pitch;
    char* surface;
    char* windowSurface;
    GBC_Enum flags;
};

// Blits the argument page to the front buffer.  This function is optimized to
// use the DMA, which should be faster on larger copies.
void DS_Blit_Display(GraphicViewPortClass& HidPage, GraphicViewPortClass& SeenPage)
{
    //size_t ram_free = Get_Free_RAM();
    //printf("Free RAM: %d\n", ram_free);

    const unsigned char* src = (const unsigned char*)HidPage.Get_Offset();
    unsigned char* dst = (unsigned char*)frontSurface->GetData();

    int dst_pitch = frontSurface->GetPitch();
    int h = HidPage.Get_Height();
    int w = HidPage.Get_Width();

    DC_FlushRange(src, w * h);

    while (h > 0) {
        dmaCopyWordsAsynch(0, src, dst, w);
        dst += dst_pitch;
        src += w;
        dmaCopyWordsAsynch(1, src, dst, w);
        dst += dst_pitch;
        src += w;
        dmaCopyWordsAsynch(2, src, dst, w);
        dst += dst_pitch;
        src += w;
        dmaCopyWordsAsynch(3, src, dst, w);
        dst += dst_pitch;
        src += w;
        h -= 4;
    }

#ifdef SHOW_FPS
    static long long now;
    long long last, delta;
    char textbuf[16];

    const unsigned timer_speed = BUS_CLOCK / 1024;

    last = now;
    now += timerElapsed(0);
    delta = now - last;

    if (delta != 0) {
        unsigned fps = timer_speed / delta;
        if (fps > 99)
            fps = 99;
        snprintf(textbuf, 16, "%4llu", timer_speed / delta);
        SeenPage.Print(textbuf, 12, HidPage.Get_Height() - 20, GREEN, BLACK);
    }
#endif
}

/*
** Video
*/

Video::Video()
{
}

Video::~Video()
{
}

Video& Video::Shared()
{
    static Video video;
    return video;
}

VideoSurface* Video::CreateSurface(int w, int h, GBC_Enum flags)
{
    return new VideoSurfaceNDS(w, h, flags);
}
