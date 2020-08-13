#ifndef WWMOUSEDRAWABLE_H
#define WWMOUSEDRAWABLE_H

#include "wwmouse.h"

class WWMouseClassDrawable : public WWMouseClass
{
public:
    WWMouseClassDrawable(GraphicViewPortClass* scr, int mouse_max_width, int mouse_max_height);
    virtual ~WWMouseClassDrawable();

    virtual void* Set_Cursor(int xhotspot, int yhotspot, void* cursor);
    virtual void Draw_Mouse(GraphicViewPortClass* scr);
    virtual void Erase_Mouse(GraphicViewPortClass* scr, int forced = 0);

protected:
    virtual void Process_Mouse();
    virtual void Low_Hide_Mouse();
    virtual void Low_Show_Mouse(int x, int y);

    void* Set_Mouse_Cursor(int hotspotx, int hotspoty, void* cursor);
    void Mouse_Shadow_Buffer(GraphicViewPortClass* viewport, void* buffer, int x, int y, int hotx, int hoty, int store);
    void Draw_Mouse(GraphicViewPortClass* viewport, int x, int y);
};

#endif // WWMOUSEDRAWABLE_H
