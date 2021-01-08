#ifndef SETTINGS_H
#define SETTINGS_H

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
        int Width;
        int Height;
        int FrameLimit;
    } Video;

    struct
    {
        bool MouseWheelScrolling;
    } Options;
};

extern SettingsClass Settings;

#endif
