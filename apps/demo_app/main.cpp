#include <demo_app\VulkanApplicationDelegate.hpp>

#include <foundation\memory\Memory.hpp>

int main()
{
    DRE::InitializeGlobalMemory();

    HINSTANCE instance = GetModuleHandle(nullptr);

    bool imguiEnabled = true;

    VulkanApplicationDelegate* appDelegate = (VulkanApplicationDelegate*)DRE::g_PersistentDataAllocator.Alloc(sizeof(VulkanApplicationDelegate), alignof(VulkanApplicationDelegate));
    new (appDelegate) VulkanApplicationDelegate{ instance, "Bullet Manager", 1600u, 900u, 2u, DEBUG_OR_RELEASE(true, false), imguiEnabled };

    Application* application = (Application*)DRE::g_PersistentDataAllocator.Alloc(sizeof(Application), alignof(Application));
    new (application) Application{ appDelegate };

    application->run();

    application->~Application();;
    appDelegate->~VulkanApplicationDelegate();;

    DRE::TerminateGlobalMemory();

    return 0;
}