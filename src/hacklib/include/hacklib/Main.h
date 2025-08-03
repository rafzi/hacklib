#ifndef HACKLIB_MAIN_H
#define HACKLIB_MAIN_H

#include "hacklib/Handles.h"
#include "hacklib/MessageBox.h"
#include <memory>
#include <string>
#include <mutex>
#include <condition_variable>


namespace hl
{
/**
 * This class can be used to define behavior of a dynamic library inside a foreign process.
 * Use the StaticInit helper for actual initialization.
 */
class Main
{
public:
    virtual ~Main() = default;

    /// \brief Is called on initialization. The default implementation just returns true.
    /// \return A return value of false will cause the dll to detach.
    virtual bool init();
    /// Is called continuously sequenially while running. The default implementation sleeps for 10 milliseconds and
    /// returns true.
    /// \return A return value of false will cause the dll to detach.
    virtual bool step();
    /// Is called on shutdown. Is still called when init returns false.
    /// The default implementation does nothing.
    virtual void shutdown();
};

/// Returns the module handle to the own dynamic library.
hl::ModuleHandle GetCurrentModule();

/// Returns the abolute path with filename to the own dynamic library.
std::string GetCurrentModulePath();

// Implementation detail.
class StaticInitImpl
{
public:
    StaticInitImpl();
    virtual ~StaticInitImpl() = default;
    void mainThread();

protected:
    void notifyDerivedConstructed();
    [[nodiscard]] virtual std::unique_ptr<hl::Main> makeMain() const = 0;

private:
    void runMainThread();
    static void unloadSelf();

protected:
    hl::Main* m_pMain = nullptr;
    std::mutex m_mutex;
    std::condition_variable m_condVar;
    bool m_derivedConstructed = false;
};

/**
 * Helper for running a program defined by hl::Main. Make sure to define it once in your dynamic library:
 * hl::StaticInit<MyMain> g_main;
 */
template <typename T>
class StaticInit final : private StaticInitImpl
{
public:
    StaticInit() { notifyDerivedConstructed(); }

    /// Returns the instance of hl::Main.
    [[nodiscard]] T* getMain() { return dynamic_cast<T*>(m_pMain); }
    /// \overload
    [[nodiscard]] const T* getMain() const { return dynamic_cast<T*>(m_pMain); }

protected:
    /// Override this for non-default constructors.
    [[nodiscard]] std::unique_ptr<hl::Main> makeMain() const override { return std::make_unique<T>(); }
};
}

#endif
