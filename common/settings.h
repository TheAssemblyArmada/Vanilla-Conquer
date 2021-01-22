#ifndef SETTINGS_H
#define SETTINGS_H

class INIClass;
#include "fixed.h"

class SettingsClass
{
public:
    SettingsClass();

    void Load(INIClass& ini);
    void Save(INIClass& ini);

    struct
    {
        int WindowWidth;
        int WindowHeight;
        bool Windowed;
        int Width;
        int Height;
        int FrameLimit;
        bool HardwareCursor;
        bool RawInput;
        float Sensitivity;
    } Video;
};

extern SettingsClass Settings;

#endif
