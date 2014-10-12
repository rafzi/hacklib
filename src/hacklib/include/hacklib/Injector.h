#ifndef HACKLIB_INJECTOR_H
#define HACKLIB_INJECTOR_H

#include <string>


namespace hl
{
    bool Inject(int pid, const std::string& libFileName, std::string *error);
}

#endif