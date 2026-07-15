#include "determinism.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

int main()
{
    cnc::web::BeginDeterministicStartup(UINT32_C(0xf1234567));
    assert(cnc_web_legacy_time_ms() == UINT32_C(0xf1234567));
    assert(cnc_web_legacy_time_seconds(NULL) == static_cast<time_t>(UINT32_C(0xf1234567)));
    cnc::web::EndDeterministicStartup();
    assert(cnc::web::LogicalMilliseconds() == 0u);
    cnc::web::AdvanceLogicalClock();
    assert(cnc::web::LogicalMilliseconds() == 66u);
    for (uint32_t tick = 1u; tick < 15u; ++tick) {
        cnc::web::AdvanceLogicalClock();
    }
    assert(cnc::web::LogicalMilliseconds() == 1000u);
    cnc::web::SetLogicalTick(150u);
    assert(cnc::web::LogicalMilliseconds() == 10000u);
    cnc::web::ResetLogicalClock();
    assert(cnc::web::LogicalMilliseconds() == 0u);
    return 0;
}
