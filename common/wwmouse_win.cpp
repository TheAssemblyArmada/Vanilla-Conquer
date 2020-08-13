#include "wwmousedrawable.h"

#include "rect.h"

#include <windows.h>

class WWMouseClassWin : public WWMouseClassDrawable
{
public:
    WWMouseClassWin(GraphicViewPortClass* scr, int mouse_max_width, int mouse_max_height);
    virtual ~WWMouseClassWin();

protected:
    virtual void GetPosition(int& x, int& y);
    virtual void Block();
    virtual void Unblock();
    virtual void Clip(const Rect& rect);

private:
    CRITICAL_SECTION MouseCriticalSection; // Control for mouse re-enterancy
    unsigned TimerHandle;

    static void CALLBACK TimerCallback(UINT event_id, UINT res1, DWORD_PTR user, DWORD_PTR res2, DWORD_PTR res3);
};

WWMouseClassWin::WWMouseClassWin(GraphicViewPortClass* scr, int mouse_max_width, int mouse_max_height)
    : WWMouseClassDrawable(scr, mouse_max_width, mouse_max_height)
{
    timeBeginPeriod(1000 / 60);
    InitializeCriticalSection(&MouseCriticalSection);
    TimerHandle = timeSetEvent(1000 / 60, 1, TimerCallback, (DWORD_PTR)this, TIME_PERIODIC | TIME_KILL_SYNCHRONOUS);
}

WWMouseClassWin::~WWMouseClassWin()
{
    if (TimerHandle) {
        timeKillEvent(TimerHandle);
        TimerHandle = 0;
    }
    timeEndPeriod(1000 / 60);
    DeleteCriticalSection(&MouseCriticalSection);

    /*
    ** Free up the windows mouse pointer movement
    */
    Clear_Cursor_Clip();
}

WWMouseClass* WWMouseClass::CreateMouse(GraphicViewPortClass* scr, int mouse_max_width, int mouse_max_height)
{
    WWMouseClass* instance = new WWMouseClassWin(scr, mouse_max_width, mouse_max_height);
    /*
    ** Force the windows mouse pointer to stay withing the graphic view port region
    */
    instance->Set_Cursor_Clip();
    return instance;
}

void WWMouseClassWin::GetPosition(int& x, int& y)
{
    POINT point = {0};    // define a structure to hold current cursor pos
    GetCursorPos(&point); // get the current cursor position
    x = point.x;
    y = point.y;
}

void WWMouseClassWin::Block()
{
    EnterCriticalSection(&MouseCriticalSection);
}

void WWMouseClassWin::Unblock()
{
    LeaveCriticalSection(&MouseCriticalSection);
}

void WWMouseClassWin::Clip(const Rect& rect)
{
    if (!rect.Is_Valid()) {
        ClipCursor(NULL);
        return;
    }

    RECT region = {0};
    region.left = rect.X;
    region.top = rect.Y;
    region.right = rect.X + rect.Width;
    region.bottom = rect.Y + rect.Height;
    ClipCursor(&region);
}

void CALLBACK WWMouseClassWin::TimerCallback(UINT event_id, UINT res1, DWORD_PTR user, DWORD_PTR res2, DWORD_PTR res3)
{
    static bool in_mouse_callback = false;
    WWMouseClassWin* _this = (WWMouseClassWin*)user;
    if ((_this == nullptr) || in_mouse_callback) {
        return;
    }

    in_mouse_callback = true;
    _this->Process_Mouse();
    in_mouse_callback = false;
}
