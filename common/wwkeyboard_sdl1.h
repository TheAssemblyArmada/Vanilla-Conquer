#include "wwkeyboard.h"

class WWKeyboardClassSDL1 : public WWKeyboardClass
{
public:
    virtual ~WWKeyboardClassSDL1();

    virtual void Fill_Buffer_From_System(void);
    virtual KeyASCIIType To_ASCII(unsigned short key);
};
