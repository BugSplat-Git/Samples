#pragma once

#include "MainWindow.g.h"

#include <set>
#include <string>

namespace winrt::MyWinUI3Crasher::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        int32_t MyProperty();
        void MyProperty(int32_t value);

        // Event-card click handlers (wired in MainWindow.xaml)
        winrt::fire_and_forget CrashCard_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void NonCrashErrorCard_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        winrt::fire_and_forget UserFeedbackCard_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void HangCard_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ViewDashboard_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);

    private:
        // ---- Feedback dialog (two-state form / thank-you) ----
        winrt::Windows::Foundation::IAsyncAction ShowFeedbackDialogAsync();

        // ---- Crash-types sheet ----
        winrt::Windows::Foundation::IAsyncAction ShowCrashSheetAsync();

        // ---- "Splat the keyboard" gesture ----
        void InitializeSplatTracking();
        void OnContentKeyDown(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& args);
        void OnContentKeyUp(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& args);
        void OnSplatResetTick(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::Foundation::IInspectable const& args);

        std::set<winrt::Windows::System::VirtualKey> m_heldKeys;
        bool m_feedbackDialogOpen = false;
        winrt::Microsoft::UI::Xaml::DispatcherTimer m_splatResetTimer{ nullptr };

        static constexpr size_t kSplatThreshold = 5;

        // ---- Recent activity ----
        void SeedRecentActivity();
        void AddRecentActivity(winrt::hstring const& kind, winrt::hstring const& title, winrt::hstring const& timeText, bool atTop);

        // ---- Dashboard ----
        void OpenDashboard(int crashId);

        // ---- Window icon ----
        void SetWindowIcon();
    };
}

namespace winrt::MyWinUI3Crasher::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
