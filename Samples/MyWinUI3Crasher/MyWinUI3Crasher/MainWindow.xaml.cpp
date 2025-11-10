#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

// Include XAML generated implementations (conditional for clean rebuilds)
#if __has_include("Generated Files\\MainWindow.xaml.g.hpp")
#include "Generated Files\\MainWindow.xaml.g.hpp"
#endif

#include "CrashFunctions.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::MyWinUI3Crasher::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();
    }

    int32_t MainWindow::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void MainWindow::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    // Common Crashes
    void MainWindow::MemoryExceptionButton_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        CrashExamples::MemoryException();
    }

    void MainWindow::DivideByZeroButton_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        CrashExamples::DivideByZero();
    }

    void MainWindow::StackOverflowButton_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        CrashExamples::StackOverflow(NULL);
    }

    void MainWindow::ThrowButton_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        CrashExamples::ThrowByUser();
    }

    void MainWindow::VectorOutOfBoundsButton_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        CrashExamples::OutOfBoundsVectorCrash();
    }

    // Memory Corruption
    void MainWindow::UseAfterFreeButton_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        CrashExamples::UseAfterFree();
    }

    void MainWindow::DoubleDeleteButton_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        CrashExamples::DoubleDelete();
    }

    void MainWindow::HeapCorruptionButton_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        CrashExamples::HeapCorruption();
    }

    void MainWindow::StackOverrunButton_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        CrashExamples::StackOverrun();
    }

    // System-Level Crashes
    void MainWindow::PrivilegedInstructionButton_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        CrashExamples::PrivilegedInstruction();
    }

    void MainWindow::InvalidFunctionPointerButton_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        CrashExamples::InvalidFunctionPointer();
    }

    void MainWindow::FastFailButton_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        CrashExamples::FastFail();
    }

    void MainWindow::InvalidParametersButton_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        CrashExamples::InvalidParameters();
    }

    void MainWindow::PureVirtualButton_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        CrashExamples::VirtualFunctionCallCrash();
    }

    void MainWindow::ApplicationHangButton_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        CrashExamples::ApplicationHang();
    }

    // Threading
    void MainWindow::ThreadButton_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        CrashExamples::ThreadException(1);
    }

    // Other Crashes
    void MainWindow::AbortButton_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        CrashExamples::CallAbort();
    }

    void MainWindow::OutOfMemoryButton_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        CrashExamples::ExhaustMemory();
    }

    // Advanced Features
    void MainWindow::SEHButton_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        CrashExamples::CustomSEHException();
    }

    void MainWindow::CreateXmlReportButton_Click(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        CrashExamples::CreateXmlReport();
    }
}
