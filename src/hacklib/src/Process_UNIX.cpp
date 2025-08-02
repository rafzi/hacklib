#include "hacklib/Process.h"
#include <stdexcept>
#include <cstring>
#include <cstdio>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>


int hl::Process::join()
{
    (void)m_handle;

    if (!m_id)
    {
        throw std::runtime_error("Process is not joinable");
    }

    int status, result;
    do
    {
        result = waitpid(m_id, &status, 0);
    } while (result < 0 && errno == EINTR);

    if (result != m_id)
    {
        throw std::runtime_error("waitpid failed");
    }

    m_id = 0;
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
        return -WTERMSIG(status);
    else
        return -1;
}

hl::Process hl::LaunchProcess(const std::string& command, const std::vector<std::string>& args,
                              const std::string& initialDirectory)
{
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(command.c_str()));
    for (const auto& arg : args)
    {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);

    const char* initialDirectoryCStr = initialDirectory.empty() ? NULL : initialDirectory.c_str();

    int pid = fork();
    if (pid < 0)
    {
        throw std::runtime_error("fork failed");
    }
    else if (pid == 0)
    {
        if (initialDirectoryCStr)
        {
            if (chdir(initialDirectoryCStr) != 0)
            {
                printf("chdir failed: %s\n", strerror(errno));
                _exit(72);
            }
        }

        execvp(argv[0], argv.data());
        // Will only reach here on error.
        // Call special exit to prevent parent process doing double cleanup.
        printf("execvp failed: %s\n", strerror(errno));
        _exit(72);
    }

    return Process(pid, 0);
}
