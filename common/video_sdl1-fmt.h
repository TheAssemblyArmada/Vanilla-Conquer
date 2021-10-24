//
// SDL2 Backports
//
#ifndef VIDEO_SDL1_H
#define VIDEO_SDL1_H

#include <SDL.h>

#define SDL_InvalidParamError(x)      SDL_SetError("Parameter '%s' is invalid", (x))
#define SDL_WINDOW_FULLSCREEN_DESKTOP SDL_FULLSCREEN
#define SDL_WINDOW_FULLSCREEN         SDL_FULLSCREEN

static inline void SDL_FreePalette(SDL_Palette* palette)
{
    if (!palette) {
        SDL_InvalidParamError("palette");
        return;
    }
    SDL_free(palette->colors);
}

static inline int SDL_SetPixelFormatPalette(SDL_PixelFormat* format, SDL_Palette* palette)
{
    if (!format) {
        SDL_SetError("SDL_SetPixelFormatPalette() passed NULL format");
        return -1;
    }

    if (palette && palette->ncolors > (1 << format->BitsPerPixel)) {
        SDL_SetError("SDL_SetPixelFormatPalette() passed a palette that doesn't match the format");
        return -1;
    }

    if (format->palette == palette) {
        return 0;
    }

    format->palette = palette;

    return 0;
}

static inline int SDL_SetSurfacePalette(SDL_Surface* surface, SDL_Palette* palette)
{
    if (!surface) {
        SDL_SetError("SDL_SetSurfacePalette() passed a NULL surface");
        return -1;
    }
    if (SDL_SetPixelFormatPalette(surface->format, palette) < 0) {
        return -1;
    }

    return 0;
}

static inline int SDL_SetPaletteColors(SDL_Palette* palette, const SDL_Color* colors, int firstcolor, int ncolors)
{
    int status = 0;

    /* Verify the parameters */
    if (!palette) {
        return -1;
    }

    if (ncolors > (palette->ncolors - firstcolor)) {
        ncolors = (palette->ncolors - firstcolor);
        status = -1;
    }

    if (colors != (palette->colors + firstcolor)) {
        SDL_memcpy(palette->colors + firstcolor, colors, ncolors * sizeof(*colors));
    }

    return status;
}

static inline SDL_Palette* SDL_AllocPalette(int ncolors)
{
    SDL_Palette* palette;

    /* Input validation */
    if (ncolors < 1) {
        SDL_InvalidParamError("ncolors");
        return NULL;
    }

    palette = (SDL_Palette*)SDL_malloc(sizeof(*palette));

    if (!palette) {
        SDL_OutOfMemory();
        return NULL;
    }

    palette->colors = (SDL_Color*)SDL_malloc(ncolors * sizeof(*palette->colors));

    if (!palette->colors) {
        SDL_free(palette);
        return NULL;
    }

    palette->ncolors = ncolors;
    SDL_memset(palette->colors, 0xFF, ncolors * sizeof(*palette->colors));

    return palette;
}

#endif
