#include "settings.h"
#include "ini.h"

SettingsClass Settings;

SettingsClass::SettingsClass()
{
    Video.WindowWidth = 640;
    Video.WindowHeight = 400;
    Video.Windowed = false;
    Video.Width = 0;
    Video.Height = 0;
    Video.FrameLimit = 120;
    Video.HardwareCursor = true;
    Video.RawInput = false;
    Video.Sensitivity = 1.0f;
}

void SettingsClass::Load(INIClass& ini)
{
    Video.WindowWidth = ini.Get_Int("Video", "WindowWidth", Video.WindowWidth);
    Video.WindowHeight = ini.Get_Int("Video", "WindowHeight", Video.WindowHeight);
    Video.Windowed = ini.Get_Bool("Video", "Windowed", Video.Windowed);
    Video.Width = ini.Get_Int("Video", "Width", Video.Width);
    Video.Height = ini.Get_Int("Video", "Height", Video.Height);
    Video.FrameLimit = ini.Get_Int("Video", "FrameLimit", Video.FrameLimit);
    Video.HardwareCursor = ini.Get_Bool("Video", "HardwareCursor", Video.HardwareCursor);
    Video.RawInput = ini.Get_Bool("Video", "RawInput", Video.RawInput);
    Video.Sensitivity = ini.Get_Float("Video", "Sensitivity", Video.Sensitivity);

    /*
    ** Raw input requires software cursor.
    */
    if (Video.RawInput) {
        Video.HardwareCursor = false;
    }
}

void SettingsClass::Save(INIClass& ini)
{
    ini.Put_Int("Video", "WindowWidth", Video.WindowWidth);
    ini.Put_Int("Video", "WindowHeight", Video.WindowHeight);
    ini.Put_Bool("Video", "Windowed", Video.Windowed);
    ini.Put_Int("Video", "Width", Video.Width);
    ini.Put_Int("Video", "Height", Video.Height);
    ini.Put_Int("Video", "FrameLimit", Video.FrameLimit);
    ini.Put_Bool("Video", "HardwareCursor", Video.HardwareCursor);
    ini.Put_Bool("Video", "RawInput", Video.RawInput);
    ini.Put_Float("Video", "Sensitivity", Video.Sensitivity);
}
