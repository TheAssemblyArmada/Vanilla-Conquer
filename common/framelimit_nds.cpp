#include "framelimit.h"
#include "wwmouse.h"
#include "settings.h"

#include <nds.h>

extern WWMouseClass* WWMouse;

void Video_Render_Frame();

void Update_HWCursor();

void Frame_Limiter(bool force_render)
{
    Update_HWCursor();
}
