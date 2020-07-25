#ifndef BITBLIT_H
#define BITBLIT_H

#ifdef __cplusplus
extern "C" {
#endif

int Linear_Blit_To_Linear(void* thisptr,
                          void* dest,
                          int x_pixel,
                          int y_pixel,
                          int dx_pixel,
                          int dy_pixel,
                          int pixel_width,
                          int pixel_height,
                          int trans);

#ifdef __cplusplus
}
#endif

#endif