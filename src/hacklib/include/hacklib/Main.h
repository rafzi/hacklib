#ifndef HACKLIB_MAIN_H
#define HACKLIB_MAIN_H

#include "hacklib/Handles.h"
#include "hacklib/MessageBox.h"
#include "hacklib/make_unique.h"
#include <string>
#include <memory>

namespace hl
{


/*
 * This class can be used to define behaviour of a dynamic library inside a foreign process.
 * Use the StaticInit helper for actual initialization.
 */
class Main
{
public:
    virtual ~Main() { }

    // Is called on initialization. If returning false, the dll will detach.
    // The default implementation just returns true.
    virtual bool init();
    // Is called continuously sequenially while running. If returning false, the dll will detach.
    // The default implementation sleeps for 10 milliseconds and returns true.
    virtual bool step();
    // Is called on shutdown.
    // The default implementation does nothing.
    virtual void shutdown();

};

// Returns the module handle to the own dynamic library.
hl::ModuleHandle GetCurrentModule();

// Returns the abolute path with filename to the own dynamic library.
std::string GetModulePath();

class StaticInitBase
{
public:
    StaticInitBase();

    void mainThread();

    void unloadSelf(); // impl. specific

protected:
    // Override to perform initialization while the module is loaded.
    virtual bool init() { return true; }

    virtual std::unique_ptr<Main> createMainObj() const = 0;

private:
    void runMainThread(); // impl. specific
    bool protectedInit(); // impl. specific
 
protected:
    Main *m_pMain;

private:
    bool m_unloadPending = false;

};

/*
Helper for running a program defined by hl::Main. Make sure instantiate it once in your dynamic library:
StaticInit<MyMain> g_main;

This simple implementation will default construct your hl::Main subclass. For complex construction or if type erasure is needed,
directly inherit from StaticInitBase.
*/
template <typename T>
class StaticInit : public StaticInitBase
{
public:
    T* getMain()
    {
        return dynamic_cast<T*>(m_pMain);
    }
    const T* getMain() const
    {
        return dynamic_cast<T*>(m_pMain);
    }

protected:
    virtual std::unique_ptr<Main> createMainObj() const override
    {
        return std::make_unique<T>();
    }

};

}

#endif
