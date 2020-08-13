#ifndef KEYFRAME_H
#define KEYFRAME_H

typedef enum
{
    KF_NUMBER = 0x08,
    KF_UNCOMP = 0x10,
    KF_DELTA = 0x20,
    KF_KEYDELTA = 0x40,
    KF_KEYFRAME = 0x80,
    KF_MASK = 0xF0
} KeyFrameType;

extern "C" {
extern unsigned int IsTheaterShape;
extern unsigned int UseBigShapeBuffer;
extern char* BigShapeBufferStart;
extern char* TheaterShapeBufferStart;
extern bool UseOldShapeDraw;
}

unsigned long Build_Frame(void const* dataptr, unsigned short framenumber, void* buffptr);
unsigned short Get_Build_Frame_Count(void const* dataptr);
unsigned short Get_Build_Frame_X(void const* dataptr);
unsigned short Get_Build_Frame_Y(void const* dataptr);
unsigned short Get_Build_Frame_Width(void const* dataptr);
unsigned short Get_Build_Frame_Height(void const* dataptr);
bool Get_Build_Frame_Palette(void const* dataptr, void* palette);
int Get_Last_Frame_Length(void);

#endif // KEYFRAME_H
