#include <demo_app\DREApplicationDelegate.hpp>

#include <foundation\memory\Memory.hpp>

int main()
{
    DRE::InitializeGlobalMemory();

    HINSTANCE instance = GetModuleHandle(nullptr);

    bool imguiEnabled = true;

    DREApplicationDelegate* appDelegate = (DREApplicationDelegate*)DRE::g_PersistentDataAllocator.Alloc(sizeof(DREApplicationDelegate), alignof(DREApplicationDelegate));
    new (appDelegate) DREApplicationDelegate{ instance, "DRE", 1600u, 900u, 2u, DEBUG_OR_RELEASE(true, false), imguiEnabled };

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