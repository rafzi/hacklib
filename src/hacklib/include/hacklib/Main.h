#ifndef HACKLIB_MAIN_H
#define HACKLIB_MAIN_H

#include "hacklib/Handles.h"
#include "hacklib/MessageBox.h"
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
    friend class RAIIHelper;

public:
    StaticInitBase();
 
    // public because this might has to be accessed by static functions
    void mainThread();

    void unloadSelf(); // impl. specific

protected:
    // override to perform initialization while the module is loaded
    virtual bool init() { return true; }

    virtual std::unique_ptr<Main> createMainObj() const = 0;

private:
    void runMainThread(); // impl. specific
    bool protectedInit(); // impl. specific
 
protected:
    std::unique_ptr<Main> m_pMain;
};

/*
Helper for running a program defined by hl::Main. Make sure instantiate it once in your dynamic library:
StaticInit<MyMain> g_main;

This simple implementation will default construct your hl::Main subclass. For complex construction or if type erasure is needed,
directly inherit from StaticInitBase.
*/
template <typename MainClass>
class StaticInit : public StaticInitBase
{
public:
    MainClass* getMain() { return dynamic_cast<T*>(m_pMain.get()); }
    const MainClass* getMain() const { return dynamic_cast<T*>(m_pMain.get()); }

protected:
    virtual std::unique_ptr<Main> createMainObj() const override { return std::make_unique<MainClass>(); }
};

}

#endif
