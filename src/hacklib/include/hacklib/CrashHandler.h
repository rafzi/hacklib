#ifndef HACKLIB_CRASHHANDLER_H
#define HACKLIB_CRASHHANDLER_H

#include <functional>
#include <cstdint>


namespace hl {

/*
 * Runs the body in a protected environment that catches native platform errors.
 * If the body causes an error, the handler is executed. The argument of the
 * handler is the native error code: NTSTATUS on Windows, signal id on Linux.
 * The errors are SEH exceptions on Windows and selected signals on Linux.
 * Note that no actions of the body will be reverted. No destructors are called.
 */
void CrashHandler(const std::function<void()>& body, const std::function<void(uint32_t)>& handler);

}

#endif