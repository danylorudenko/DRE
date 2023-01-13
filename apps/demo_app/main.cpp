#include <demo_app\VulkanApplicationDelegate.hpp>

#include <foundation\memory\Memory.hpp>

int main()
{
    DRE::InitializeGlobalMemory();

    HINSTANCE instance = GetModuleHandle(nullptr);

    bool imguiEnabled = true;

    auto* appDelegate = DRE::g_PersistentDataAllocator.Alloc<VulkanApplicationDelegate>(instance, "Bullet Manager", 1600u, 900u, 2u, DEBUG_OR_RELEASE(true, false), imguiEnabled);
    auto* application = DRE::g_PersistentDataAllocator.Alloc<Application>(appDelegate);

    application->run();

    application->~Application();;
    appDelegate->~VulkanApplicationDelegate();;

    DRE::TerminateGlobalMemory();

    return 0;
}