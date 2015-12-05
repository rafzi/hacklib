#ifndef HACKLIB_MAIN_H
#define HACKLIB_MAIN_H

#include "hacklib/Handles.h"
#include <string>


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


class StaticInitImpl
{
public:
    StaticInitImpl();
    void mainThread();
protected:
    virtual void constructAndRun() = 0;
    void protectedMainLoop(hl::Main& main);
private:
    void createThread();
    void runMain(hl::Main& main);
    void unloadSelf();
protected:
    hl::Main *m_pMain = nullptr;
};

/*
Helper for running a program defined by hl::Main. Make sure instantiate it once in your dynamic library:
StaticInit<MyMain> g_main;
*/
template <typename T>
class StaticInit : public StaticInitImpl
{
public:
    T *getMain()
    {
        return dynamic_cast<T*>(m_pMain);
    }
    const T *getMain() const
    {
        return dynamic_cast<T*>(m_pMain);
    }
protected:
    void constructAndRun() override
    {
        T main;
        protectedMainLoop(main);
    }
};

}

#endif
