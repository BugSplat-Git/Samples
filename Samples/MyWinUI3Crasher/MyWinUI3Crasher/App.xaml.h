#pragma once

#include "App.xaml.g.h"

namespace winrt::MyWinUI3Crasher::implementation
{
    struct App : AppT<App>
    {
        App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

    private:
        void OnUnhandledException(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::UnhandledExceptionEventArgs const& e);

        winrt::Microsoft::UI::Xaml::Window window{ nullptr };
    };
}
