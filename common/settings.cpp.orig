#include "settings.h"
#include "ini.h"
#include "miscasm.h"

SettingsClass Settings;

SettingsClass::SettingsClass()
{
    /*
    ** Mouse settings
    */
    Mouse.RawInput = true;
    Mouse.Sensitivity = 100;

    /*
    ** Video settings
    */
    Video.WindowWidth = 640;
    Video.WindowHeight = 400;
    Video.Windowed = false;
    Video.Width = 0;
    Video.Height = 0;
    Video.Boxing = true;
    Video.FrameLimit = 120;
<<<<<<< HEAD
    Video.InterpolationMode = 2;
    Video.HardwareCursor = false;
    Video.DOSMode = false;
    Video.Scaler = "nearest";
    Video.Driver = "default";
    Video.PixelFormat = "default";
=======

    Options.MouseWheelScrolling = true;
>>>>>>> 9b7ca8f (Fix mousewheel handling and check for mousewheel in hotkey handling code for sidebar scrolling code, also add SettingsClass setting to disable mousewheel scrolling (using same keyword as CnCNet patches))
}

void SettingsClass::Load(INIClass& ini)
{
    /*
    ** Mouse settings
    */
    Mouse.RawInput = ini.Get_Bool("Mouse", "RawInput", Mouse.RawInput);
    Mouse.Sensitivity = ini.Get_Int("Mouse", "Sensitivity", Mouse.Sensitivity);

    /*
    ** Video settings
    */
    Video.WindowWidth = ini.Get_Int("Video", "WindowWidth", Video.WindowWidth);
    Video.WindowHeight = ini.Get_Int("Video", "WindowHeight", Video.WindowHeight);
    Video.Windowed = ini.Get_Bool("Video", "Windowed", Video.Windowed);
    Video.Boxing = ini.Get_Bool("Video", "Boxing", Video.Boxing);
    Video.Width = ini.Get_Int("Video", "Width", Video.Width);
    Video.Height = ini.Get_Int("Video", "Height", Video.Height);
    Video.FrameLimit = ini.Get_Int("Video", "FrameLimit", Video.FrameLimit);
<<<<<<< HEAD
    Video.HardwareCursor = ini.Get_Bool("Video", "HardwareCursor", Video.HardwareCursor);
    Video.DOSMode = ini.Get_Bool("Video", "DOSMode", Video.DOSMode);
    Video.Scaler = ini.Get_String("Video", "Scaler", Video.Scaler);
    Video.Driver = ini.Get_String("Video", "Driver", Video.Driver);
    Video.PixelFormat = ini.Get_String("Video", "PixelFormat", Video.PixelFormat);

    /*
    ** VQA and WSA interpolation mode 0 = scanlines, 1 = vertical doubling, 2 = linear
    */
    Video.InterpolationMode = Bound(ini.Get_Int("Video", "InterpolationMode", Video.InterpolationMode), 0, 2);

    /*
    ** Boxing and raw input require software cursor.
    */
    if (Video.Boxing || Mouse.RawInput) {
        Video.HardwareCursor = false;
    }
=======

    Options.MouseWheelScrolling = ini.Get_Bool("Options", "MouseWheelScrolling", Video.FrameLimit);
>>>>>>> 9b7ca8f (Fix mousewheel handling and check for mousewheel in hotkey handling code for sidebar scrolling code, also add SettingsClass setting to disable mousewheel scrolling (using same keyword as CnCNet patches))
}

void SettingsClass::Save(INIClass& ini)
{
    /*
    ** Mouse settings
    */
    ini.Put_Bool("Mouse", "RawInput", Mouse.RawInput);
    ini.Put_Int("Mouse", "Sensitivity", Mouse.Sensitivity);

    /*
    ** Video settings
    */
    ini.Put_Int("Video", "WindowWidth", Video.WindowWidth);
    ini.Put_Int("Video", "WindowHeight", Video.WindowHeight);
    ini.Put_Bool("Video", "Windowed", Video.Windowed);
    ini.Put_Bool("Video", "Boxing", Video.Boxing);
    ini.Put_Int("Video", "Width", Video.Width);
    ini.Put_Int("Video", "Height", Video.Height);
    ini.Put_Int("Video", "FrameLimit", Video.FrameLimit);
<<<<<<< HEAD
    ini.Put_Bool("Video", "HardwareCursor", Video.HardwareCursor);
    ini.Put_Bool("Video", "DOSMode", Video.DOSMode);
    ini.Put_String("Video", "Scaler", Video.Scaler);
    ini.Put_String("Video", "Driver", Video.Driver);
    ini.Put_String("Video", "PixelFormat", Video.PixelFormat);

    /*
    ** VQA and WSA interpolation mode 0 = scanlines, 1 = vertical doubling, 2 = linear
    */
    ini.Put_Int("Video", "InterpolationMode", Video.InterpolationMode);
=======

    ini.Put_Bool("Options", "MouseWheelScrolling", Video.FrameLimit);
>>>>>>> 9b7ca8f (Fix mousewheel handling and check for mousewheel in hotkey handling code for sidebar scrolling code, also add SettingsClass setting to disable mousewheel scrolling (using same keyword as CnCNet patches))
}
