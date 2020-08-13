#include "wwmouse.h"

class WWMouseClassNull : public WWMouseClass
{
public:
    WWMouseClassNull(GraphicViewPortClass* scr, int mouse_max_width, int mouse_max_height);
    virtual ~WWMouseClassNull();

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

WWMouseClassNull::WWMouseClassNull(GraphicViewPortClass* scr, int mouse_max_width, int mouse_max_height)
    : WWMouseClass(scr, mouse_max_width, mouse_max_height)
{
}

WWMouseClassNull::~WWMouseClassNull()
{
}

WWMouseClass* WWMouseClass::CreateMouse(GraphicViewPortClass* scr, int mouse_max_width, int mouse_max_height)
{
    return new WWMouseClassNull(scr, mouse_max_width, mouse_max_height);
}

void* WWMouseClassNull::Set_Cursor(int xhotspot, int yhotspot, void* cursor)
{
    return cursor;
}

void WWMouseClassNull::Draw_Mouse(GraphicViewPortClass* scr)
{
}

void WWMouseClassNull::Erase_Mouse(GraphicViewPortClass* scr, int forced)
{
}

void WWMouseClassNull::Process_Mouse()
{
}

void WWMouseClassNull::Low_Hide_Mouse()
{
    State++;
}

void WWMouseClassNull::Low_Show_Mouse(int x, int y)
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

void WWMouseClassNull::GetPosition(int& x, int& y)
{
}

void WWMouseClassNull::Block()
{
}

void WWMouseClassNull::Unblock()
{
}

void WWMouseClassNull::Clip(const Rect& rect)
{
}
