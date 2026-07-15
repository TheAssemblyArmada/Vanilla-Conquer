/* Compile-only coverage that the public ABI remains a C header. */
#include "cnc_web.h"
#include "cnc_web_protocol.h"

int main(void)
{
    cnc_web_handle_t handle = CNC_WEB_INVALID_HANDLE;
    cnc_web_status_t status = CNC_WEB_OK;
    cnc_web_status_t (*set_transition)(cnc_web_handle_t, int32_t, uint32_t) =
        &cnc_web_set_campaign_transition;
    return (handle == 0u && status == 0 && set_transition != 0
            && cnc_web_abi_version() == CNC_WEB_ABI_VERSION)
        ? 0
        : 1;
}
