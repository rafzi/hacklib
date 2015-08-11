#include "hacklib/Injector.h"
#include <iostream>
#include <string>




int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cout << "Too few arguments" << std::endl;
        return 1;
    }

    int pid = 0;
    std::string pname;
    std::string wtitle;
    std::string wclass;
    std::string lib;

    for (int i = 1; i < argc; i++)
    {
        std::string cmd = argv[i];

        if (cmd == "-pid")
        {
            if (++i >= argc)
            {
                std::cout << "Too few arguments for -pid" << std::endl;
                return 1;
            }
            pid = std::stoi(argv[i]);
        }
        else if (cmd == "-pname")
        {
            if (++i >= argc)
            {
                std::cout << "Too few arguments for -pname" << std::endl;
                return 1;
            }
            pname = argv[i];
        }
        else if (cmd == "-wtitle")
        {
            if (++i >= argc)
            {
                std::cout << "Too few arguments for -wtitle" << std::endl;
                return 1;
            }
            wtitle = argv[i];
        }
        else if (cmd == "-wclass")
        {
            if (++i >= argc)
            {
                std::cout << "Too few arguments for -wclass" << std::endl;
                return 1;
            }
            wclass = argv[i];
        }
        else if (cmd == "-lib")
        {
            if (++i >= argc)
            {
                std::cout << "Too few arguments for -lib" << std::endl;
                return 1;
            }
            lib = argv[i];
        }
    }

    std::string errorMsg;
    if (lib != "")
    {
        if (pid != 0)
        {
            if (!hl::Inject(pid, lib, &errorMsg))
            {
                std::cout << "Injection failed: " << errorMsg << std::endl;
                std::cin.ignore();
            }
        }

        if (pname != "")
        {
            auto pids = hl::GetPIDsByProcName(pname);
            if (pids.empty())
            {
                std::cout << "No process with name: " << pname << std::endl;
                std::cin.ignore();
            }
            for (auto p : pids)
            {
                if (!hl::Inject(p, lib, &errorMsg))
                {
                    std::cout << "Injection into " << p << " failed: " << errorMsg << std::endl;
                    std::cin.ignore();
                }
            }
        }

        if (wtitle != "")
        {

        }

        if (wclass != "")
        {

        }
    }
    else
    {
        std::cout << "No library specified" << std::endl;
    }

    return 0;
}