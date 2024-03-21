#include "wwkeyboard.h"

class WWKeyboardClassWin32 : public WWKeyboardClass
{
public:
    virtual ~WWKeyboardClassWin32();

    virtual void Fill_Buffer_From_System(void);
    virtual KeyASCIIType To_ASCII(unsigned short key);

private:
    /*
    **	This is a keyboard state array that is used to aid in translating
    **	KN_ keys into KA_ keys.
    */
    unsigned char KeyState[256];
};
