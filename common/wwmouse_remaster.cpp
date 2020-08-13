#include "wwmouse.h"

class WWMouseClassRemaster : public WWMouseClass
{
public:
    WWMouseClassRemaster(GraphicViewPortClass* scr, int mouse_max_width, int mouse_max_height);
    virtual ~WWMouseClassRemaster();

    virtual void* Set_Cursor(int xhotspot, int yhotspot, void* cursor);
    virtual void Draw_Mouse(GraphicViewPortClass* scr);
    virtual void Erase_Mouse(GraphicViewPortClass* scr, int forced = 0);

protected:
    virtual void Process_Mouse();
    virtual void Low_Hide_Mouse();
    virtual void Low_Show_Mouse(int x, int y);

    virtual void GetPosition(int& x, int& y);
    virtual void Block();
    virtual void Unblock();
    virtual void Clip(const Rect& rect);
};

WWMouseClassRemaster::WWMouseClassRemaster(GraphicViewPortClass* scr, int mouse_max_width, int mouse_max_height)
    : WWMouseClass(scr, mouse_max_width, mouse_max_height)
{
}

WWMouseClassRemaster::~WWMouseClassRemaster()
{
}

WWMouseClass* WWMouseClass::CreateMouse(GraphicViewPortClass* scr, int mouse_max_width, int mouse_max_height)
{
    return new WWMouseClassRemaster(scr, mouse_max_width, mouse_max_height);
}

void* WWMouseClassRemaster::Set_Cursor(int xhotspot, int yhotspot, void* cursor)
{
    return cursor;
}

void WWMouseClassRemaster::Draw_Mouse(GraphicViewPortClass* scr)
{
}

void WWMouseClassRemaster::Erase_Mouse(GraphicViewPortClass* scr, int forced)
{
}

void WWMouseClassRemaster::Process_Mouse()
{
}

void WWMouseClassRemaster::Low_Hide_Mouse()
{
    State++;
}

void WWMouseClassRemaster::Low_Show_Mouse(int x, int y)
{
    //
    // If the mouse is already visible then just ignore the problem.
    //
    if (State == 0)
        return;
    //
    // Make the mouse a little bit more visible
    //
    State--;
}

extern int DLLForceMouseX;
extern int DLLForceMouseY;

void WWMouseClassRemaster::GetPosition(int& x, int& y)
{
    if (DLLForceMouseX >= 0) {
        x = DLLForceMouseX;
    }

    if (DLLForceMouseY >= 0) {
        y = DLLForceMouseY;
    }
}

void WWMouseClassRemaster::Block()
{
}

void WWMouseClassRemaster::Unblock()
{
}

void WWMouseClassRemaster::Clip(const Rect& rect)
{
}
