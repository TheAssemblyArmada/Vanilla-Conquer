/* Browser builds receive normalized commands through cnc_web_submit_commands. */
#include "wwkeyboard.h"

namespace {

class WWKeyboardClassWeb : public WWKeyboardClass
{
public:
    virtual ~WWKeyboardClassWeb() {}

    virtual KeyASCIIType To_ASCII(unsigned short key)
    {
        (void)key;
        return KA_NONE;
    }

private:
    virtual void Fill_Buffer_From_System() {}
};

} // namespace

WWKeyboardClass* CreateWWKeyboardClass(void)
{
    return new WWKeyboardClassWeb();
}
