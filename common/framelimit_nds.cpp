#include "framelimit.h"
#include "wwmouse.h"
#include "settings.h"

#include <nds.h>

void Update_HWCursor();

void Frame_Limiter(FrameLimitFlags flags)
{
    Update_HWCursor();
}
