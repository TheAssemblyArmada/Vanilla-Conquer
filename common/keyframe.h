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
}

#endif // KEYFRAME_H
