#ifndef DRAWMISC_H
#define DRAWMISC_H

class GraphicViewPortClass;

extern unsigned char CurrentPalette[768];
extern unsigned char PaletteTable[1024];

void Buffer_Fill_Rect(void* thisptr, int sx, int sy, int dx, int dy, unsigned char color);
void Buffer_Clear(void* this_object, unsigned char color);
bool Linear_Blit_To_Linear(void* this_object,
                           void* dest,
                           int x_pixel,
                           int y_pixel,
                           int dest_x0,
                           int dest_y0,
                           int pixel_width,
                           int pixel_height,
                           bool trans);
bool Linear_Scale_To_Linear(void* this_object,
                            void* dest,
                            int src_x,
                            int src_y,
                            int dst_x,
                            int dst_y,
                            int src_width,
                            int src_height,
                            int dst_width,
                            int dst_height,
                            bool trans,
                            char* remap);

extern int LastIconset;
extern int StampPtr;
extern int IsTrans;
extern int MapPtr;
extern int IconWidth;
extern int IconHeight;
extern int IconSize;
extern int IconCount;

void Init_Stamps(unsigned int icondata);

void Fat_Put_Pixel(int x, int y, int value, int size, GraphicViewPortClass& gvp);

#endif
