#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
class INIClass;

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
        bool Boxing;
        int Width;
        int Height;
        int FrameLimit;
        bool HardwareCursor;
        std::string Scaler;
        std::string Driver;
        std::string PixelFormat;
    } Video;
};

extern SettingsClass Settings;

#endif
