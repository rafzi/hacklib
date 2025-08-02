#ifndef HACKLIB_PROCESS_H
#define HACKLIB_PROCESS_H

#include <string>
#include <vector>
#include <cstdint>


namespace hl
{

// Represents an operating system process.
class Process
{
public:
    Process(const Process&) = delete;
    Process(Process&& other) = default;

    // Returns the PID.
    int id() const { return m_id; }
    // Waits for the process to exit and return its exit code.
    int join();

private:
    int m_id;
    uintptr_t m_handle;

private:
    Process(int id, uintptr_t handle) : m_id(id), m_handle(handle) {}
    friend Process LaunchProcess(const std::string&, const std::vector<std::string>&, const std::string&);
};

// Starts a new process of the given command and args inside initialDirectory. Throws on error.
Process LaunchProcess(const std::string& command, const std::vector<std::string>& args = {},
                      const std::string& initialDirectory = "");

}

#endif
