#include "hacklib/CrashHandler.h"
#include <Windows.h>


void hl::CrashHandler(const std::function<void()>& body, const std::function<void(uint32_t)>& handler)
{
    __try
    {
        body();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        handler(GetExceptionCode());
    }
}