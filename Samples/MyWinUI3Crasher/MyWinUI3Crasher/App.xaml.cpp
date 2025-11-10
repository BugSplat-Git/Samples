#include "pch.h"
#include "App.xaml.h"
#include "MainWindow.xaml.h"
#include "BugSplat.h"
#include "Main.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

// Initialize BugSplat
BugSplat g_bugsplat(BUGSPLAT_DATABASE, APPLICATION_NAME, APPLICATION_VERSION);

namespace winrt::MyWinUI3Crasher::implementation
{
	void App::OnUnhandledException(
		winrt::Windows::Foundation::IInspectable const& sender,
		winrt::Microsoft::UI::Xaml::UnhandledExceptionEventArgs const& e)
	{
        // Log the exception details
        auto errorMessage = e.Message();

        // For certain exceptions, you may decide to handle them and prevent termination.
        // For example, display an error message to the user.
        // NOTE: Setting Handled to true does not prevent termination for all exceptions.
        // Serious system or framework exceptions will cause the app to terminate regardless.
        
        // Mark as handled to prevent app crash during startup issues
        e.Handled(true);
    }

    /// <summary>
    /// Initializes the singleton application object.  This is the first line of authored code
    /// executed, and as such is the logical equivalent of main() or WinMain().
    /// </summary>
    App::App()
    {
        SetGlobalCRTExceptionBehavior();
        SetPerThreadCRTExceptionBehavior();

        if (!g_bugsplat.IsWerEnabled()) {
            // Crash handling requires configuring a Windows Registry key.  Refer to BugSplat documentation for details.
            // The key must be named with the path to BugSplatWer.dll and should be created in:
            // Computer\HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting\RuntimeExceptionHelperModules
            MessageBoxW(NULL, L"BugSplat WER is not configured!  Crashes will not be handled by BugSplat.", L"Warning", MB_OK | MB_ICONWARNING);
        }
    }

    /// <summary>
    /// Invoked when the application is launched.
    /// </summary>
    /// <param name="e">Details about the launch request and process.</param>
    void App::OnLaunched([[maybe_unused]] LaunchActivatedEventArgs const& e)
    {
        window = make<MainWindow>();
        window.Activate();
    }
}
