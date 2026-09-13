#include <demo_app\DREApplicationDelegate.hpp>

#include <foundation\memory\Memory.hpp>

int main()
{
    //MessageBoxA(NULL, "Attach Debugger", "DRE", MB_OK | MB_ICONINFORMATION);

    DRE::InitializeGlobalMemory();

    HINSTANCE instance = GetModuleHandle(nullptr);

    bool constexpr      imguiEnabled = true;

    // error - 0, warning - 1, info - 2, verbose - 3
    // reduce this before doing graphis capture in nsight
    DRE::U32 constexpr  validationBreakSeverity = 3;

    DREApplicationDelegate* appDelegate = (DREApplicationDelegate*)DRE::g_PersistentDataAllocator.Alloc(sizeof(DREApplicationDelegate), alignof(DREApplicationDelegate));
    new (appDelegate) DREApplicationDelegate{ instance, "DRE", 1600u, 900u, 2u, DEBUG_OR_RELEASE(true, false), validationBreakSeverity, imguiEnabled };

    Application* application = (Application*)DRE::g_PersistentDataAllocator.Alloc(sizeof(Application), alignof(Application));
    new (application) Application{ appDelegate };

    application->run();

    application->~Application();;
    appDelegate->~DREApplicationDelegate();

    DRE::TerminateGlobalMemory();

    // to terminate all detached threads we don't care about
    std::exit(0);

    return 0;
}