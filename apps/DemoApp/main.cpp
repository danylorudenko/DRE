#include <DemoApp\VulkanApplicationDelegate.hpp>

#include <foundation\memory\Memory.hpp>

int main()
{
    DRE::InitializeGlobalMemory();

    HINSTANCE instance = GetModuleHandle(nullptr);

    bool imguiEnabled = true;

    auto* appDelegate = new VulkanApplicationDelegate{ instance, "Bullet Manager", 1600, 900, 2, DEBUG_OR_RELEASE(true, false), imguiEnabled };
    auto* application = new Application{ appDelegate };

    application->run();

    delete application;
    delete appDelegate;

    DRE::TerminateGlobalMemory();

    return 0;
}