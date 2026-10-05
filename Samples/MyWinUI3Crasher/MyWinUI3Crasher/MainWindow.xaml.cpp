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
#include "BugSplat.h"
#include "Main.h"

#include <array>
#include <memory>

#pragma comment(lib, "comdlg32.lib")

extern BugSplat g_bugsplat;

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Media;
using namespace winrt::Microsoft::UI::Xaml::Media::Animation;
using namespace winrt::Microsoft::UI::Xaml::Shapes;

namespace
{
    constexpr uint64_t kMaxAttachmentSize = 10ULL * 1024 * 1024; // 10 MB

    // Minimal RAII guard: runs the action on scope exit, including via an
    // exception. (Avoids <wil/resource.h>, whose ::Microsoft namespace would
    // clash with the winrt::Microsoft using-directives above.)
    template <typename F>
    struct ScopeGuard
    {
        F action;
        ~ScopeGuard() { action(); }
    };
    template <typename F>
    ScopeGuard<F> MakeScopeGuard(F action) { return { std::move(action) }; }

    // --- Resource lookup helpers (pull the App.xaml design-system resources) ---
    SolidColorBrush BrushRes(std::wstring_view key)
    {
        return Application::Current().Resources().Lookup(box_value(hstring{ key })).as<SolidColorBrush>();
    }

    Microsoft::UI::Xaml::Style StyleRes(std::wstring_view key)
    {
        return Application::Current().Resources().Lookup(box_value(hstring{ key })).as<Microsoft::UI::Xaml::Style>();
    }

    SolidColorBrush AccentBrushWithOpacity(std::wstring_view key, double opacity)
    {
        SolidColorBrush b;
        b.Color(BrushRes(key).Color());
        b.Opacity(opacity);
        return b;
    }

    // --- Small text/layout factories to keep the programmatic UI readable ---
    TextBlock MakeText(hstring const& text, double size, SolidColorBrush const& brush, bool semibold = false)
    {
        TextBlock tb;
        tb.Text(text);
        tb.FontSize(size);
        tb.Foreground(brush);
        tb.TextWrapping(TextWrapping::Wrap);
        if (semibold)
        {
            tb.FontWeight(Microsoft::UI::Text::FontWeights::SemiBold());
        }
        return tb;
    }

    // --- String helpers ---
    std::wstring Trim(std::wstring s)
    {
        const wchar_t* ws = L" \t\r\n";
        size_t start = s.find_first_not_of(ws);
        if (start == std::wstring::npos) return L"";
        size_t end = s.find_last_not_of(ws);
        return s.substr(start, end - start + 1);
    }

    std::wstring FileNameOf(std::wstring const& path)
    {
        size_t pos = path.find_last_of(L"\\/");
        return pos == std::wstring::npos ? path : path.substr(pos + 1);
    }

    std::wstring FileExtUpper(std::wstring const& name)
    {
        size_t dot = name.find_last_of(L'.');
        if (dot == std::wstring::npos || dot + 1 >= name.size()) return L"FILE";
        std::wstring ext = name.substr(dot + 1);
        for (auto& c : ext) c = towupper(c);
        if (ext.size() > 4) ext = ext.substr(0, 4);
        return ext;
    }

    // Human-readable byte size, e.g. "287 KB" / "1.4 MB".
    std::wstring FormatBytes(uint64_t bytes)
    {
        const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB" };
        double size = static_cast<double>(bytes);
        int u = 0;
        while (size >= 1024.0 && u < 3) { size /= 1024.0; ++u; }
        wchar_t buf[64];
        if (u == 0) swprintf_s(buf, L"%.0f %s", size, units[u]);
        else        swprintf_s(buf, L"%.1f %s", size, units[u]);
        return buf;
    }

    // Middle-truncate, e.g. "signup-keyboard-bug.png" -> "signup-k...g-bug.png".
    std::wstring MiddleTruncate(std::wstring const& s, size_t maxLen)
    {
        if (s.size() <= maxLen) return s;
        if (maxLen <= 3) return s.substr(0, maxLen);
        size_t keep = maxLen - 3;
        size_t front = (keep + 1) / 2;
        size_t back = keep - front;
        return s.substr(0, front) + L"..." + s.substr(s.size() - back);
    }

    uint64_t GetFileSizeBytes(std::wstring const& path)
    {
        WIN32_FILE_ATTRIBUTE_DATA info{};
        if (GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &info))
        {
            return (static_cast<uint64_t>(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
        }
        return 0;
    }

    // Win32 file picker (WinUI3 FileOpenPicker needs extra HWND wiring; keep the
    // existing GetOpenFileNameW idiom from the original sample).
    std::wstring PickFile()
    {
        wchar_t filePath[MAX_PATH] = {};
        OPENFILENAMEW ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = nullptr;
        ofn.lpstrFilter = L"All Files\0*.*\0";
        ofn.lpstrFile = filePath;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        if (GetOpenFileNameW(&ofn))
        {
            return filePath;
        }
        return L"";
    }

    // Absolute path to a bundled asset, resolved against the package install location
    // (falls back to the executable directory when run unpackaged).
    std::wstring InstalledFilePath(std::wstring_view relative)
    {
        try
        {
            auto installed = Windows::ApplicationModel::Package::Current().InstalledLocation().Path();
            return std::wstring(installed.c_str()) + L"\\" + std::wstring(relative);
        }
        catch (...)
        {
            wchar_t exePath[MAX_PATH]{};
            GetModuleFileNameW(nullptr, exePath, MAX_PATH);
            std::wstring p = exePath;
            size_t pos = p.find_last_of(L"\\/");
            if (pos != std::wstring::npos) p = p.substr(0, pos);
            return p + L"\\" + std::wstring(relative);
        }
    }

    // Installed path of the bundled sample log (ms-appx Assets\sample.log).
    std::wstring GetSampleLogPath()
    {
        try
        {
            auto installed = Windows::ApplicationModel::Package::Current().InstalledLocation().Path();
            return std::wstring(installed.c_str()) + L"\\Assets\\sample.log";
        }
        catch (...)
        {
            wchar_t exePath[MAX_PATH]{};
            GetModuleFileNameW(nullptr, exePath, MAX_PATH);
            std::wstring p = exePath;
            size_t pos = p.find_last_of(L"\\/");
            if (pos != std::wstring::npos) p = p.substr(0, pos);
            return p + L"\\Assets\\sample.log";
        }
    }

    // A close (X) glyph button shared by both dialogs.
    Button MakeCloseButton()
    {
        Button x;
        x.Content(box_value(L"\u2715")); // multiplication-x close glyph
        x.FontSize(14);
        x.Background(SolidColorBrush(Microsoft::UI::Colors::Transparent()));
        x.BorderThickness(ThicknessHelper::FromUniformLength(0));
        x.Padding(ThicknessHelper::FromLengths(8, 4, 8, 4));
        x.Foreground(BrushRes(L"TextTertiaryBrush"));
        return x;
    }

    // Header row: title + divider, with an X close button on the right.
    StackPanel MakeDialogHeader(hstring const& titleText, Button const& closeButton)
    {
        StackPanel header;
        header.Spacing(12);

        Grid top;
        top.ColumnDefinitions().Append(ColumnDefinition());
        {
            ColumnDefinition auto1; auto1.Width(GridLengthHelper::FromValueAndType(0, GridUnitType::Auto));
            top.ColumnDefinitions().Append(auto1);
        }
        auto title = MakeText(titleText, 20, BrushRes(L"TextPrimaryBrush"), true);
        title.FontWeight(Microsoft::UI::Text::FontWeights::Bold());
        Grid::SetColumn(title, 0);
        Grid::SetColumn(closeButton, 1);
        top.Children().Append(title);
        top.Children().Append(closeButton);
        header.Children().Append(top);

        Border divider;
        divider.Height(1);
        divider.Background(BrushRes(L"CardStrokeBrush"));
        header.Children().Append(divider);

        return header;
    }

    // Make a ContentDialog match the app's light surface. WinUI's default dialog renders
    // dark on a dark system theme; this forces light theme + palette brushes and a tight,
    // uniform padding. The dialog then hugs its fixed-width content (set per dialog), so the
    // content is centered with no uneven side gaps.
    void ApplyDialogChrome(ContentDialog const& dialog)
    {
        dialog.RequestedTheme(ElementTheme::Light);

        auto res = dialog.Resources();
        res.Insert(box_value(L"ContentDialogBackground"), BrushRes(L"CardBgBrush"));
        res.Insert(box_value(L"ContentDialogForeground"), BrushRes(L"TextPrimaryBrush"));
        res.Insert(box_value(L"ContentDialogBorderBrush"), BrushRes(L"CardStrokeBrush"));
        res.Insert(box_value(L"ContentDialogBorderThickness"), box_value(ThicknessHelper::FromUniformLength(1)));
        res.Insert(box_value(L"ContentDialogPadding"), box_value(ThicknessHelper::FromUniformLength(16)));
        // Allow the dialog to size down to its content instead of a wide default.
        res.Insert(box_value(L"ContentDialogMinWidth"), box_value(0.0));
        res.Insert(box_value(L"ContentDialogMaxWidth"), box_value(640.0));
    }
}

namespace winrt::MyWinUI3Crasher::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();

        SetWindowIcon();
        SeedRecentActivity();
        InitializeSplatTracking();
    }

    void MainWindow::SetWindowIcon()
    {
        // Best-effort: never let icon setup take down app startup.
        try
        {
            auto windowNative = this->try_as<::IWindowNative>();
            if (!windowNative) return;

            HWND hwnd{};
            windowNative->get_WindowHandle(&hwnd);
            if (!hwnd) return;

            auto windowId = winrt::Microsoft::UI::GetWindowIdFromWindow(hwnd);
            auto appWindow = winrt::Microsoft::UI::Windowing::AppWindow::GetFromWindowId(windowId);
            if (appWindow)
            {
                appWindow.SetIcon(hstring{ InstalledFilePath(L"Assets\\bugsplat.ico") });
            }
        }
        catch (...)
        {
            // Packaged apps fall back to the package logo; nothing else to do.
        }
    }

    int32_t MainWindow::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void MainWindow::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    // ============================================================
    // Event cards
    // ============================================================

    fire_and_forget MainWindow::CrashCard_Click(IInspectable const&, RoutedEventArgs const&)
    {
        // Keep this alive across the suspension: the window can close mid-await.
        auto lifetime = get_strong();
        co_await ShowCrashSheetAsync();
    }

    void MainWindow::NonCrashErrorCard_Click(IInspectable const&, RoutedEventArgs const&)
    {
        // Non-fatal: the app keeps running after sending an XML report.
        CrashExamples::CreateXmlReport();
        AddRecentActivity(L"Error", L"Non-crash XML report sent", L"just now", true);
    }

    fire_and_forget MainWindow::UserFeedbackCard_Click(IInspectable const&, RoutedEventArgs const&)
    {
        // Keep this alive across the suspension: the window can close mid-await.
        auto lifetime = get_strong();
        co_await ShowFeedbackDialogAsync();
    }

    void MainWindow::HangCard_Click(IInspectable const&, RoutedEventArgs const&)
    {
        CrashExamples::ApplicationHang();
    }

    void MainWindow::ViewDashboard_Click(IInspectable const&, RoutedEventArgs const&)
    {
        OpenDashboard(0);
    }

    // ============================================================
    // Dashboard
    // ============================================================

    void MainWindow::OpenDashboard(int crashId)
    {
        std::wstring db = BUGSPLAT_DATABASE;
        std::wstring url;
        if (crashId > 0)
        {
            url = L"https://app.bugsplat.com/v2/crash?database=" + db + L"&id=" + std::to_wstring(crashId);
        }
        else
        {
            url = L"https://app.bugsplat.com/v2/dashboard?database=" + db;
        }
        Windows::System::Launcher::LaunchUriAsync(Uri{ hstring{ url } });
    }

    // ============================================================
    // Recent activity
    // ============================================================

    void MainWindow::SeedRecentActivity()
    {
        AddRecentActivity(L"Crash", L"NullReferenceException", L"11m ago", false);
        AddRecentActivity(L"Feedback", L"Keyboard collapses after autofill", L"1h ago", false);
    }

    void MainWindow::AddRecentActivity(hstring const& kind, hstring const& title, hstring const& timeText, bool atTop)
    {
        Grid row;
        row.Margin(ThicknessHelper::FromLengths(0, 7, 0, 7));
        row.ColumnSpacing(10);

        {
            ColumnDefinition c0; c0.Width(GridLengthHelper::FromValueAndType(0, GridUnitType::Auto)); row.ColumnDefinitions().Append(c0);
            ColumnDefinition c1; c1.Width(GridLengthHelper::FromValueAndType(0, GridUnitType::Auto)); row.ColumnDefinitions().Append(c1);
            ColumnDefinition c2; c2.Width(GridLengthHelper::FromValueAndType(1, GridUnitType::Star)); row.ColumnDefinitions().Append(c2);
            ColumnDefinition c3; c3.Width(GridLengthHelper::FromValueAndType(0, GridUnitType::Auto)); row.ColumnDefinitions().Append(c3);
        }

        Shapes::Ellipse dot;
        dot.Width(8);
        dot.Height(8);
        dot.VerticalAlignment(VerticalAlignment::Center);
        dot.Fill(BrushRes(L"LinkBrush"));
        Grid::SetColumn(dot, 0);
        row.Children().Append(dot);

        auto kindText = MakeText(kind, 13, BrushRes(L"TextPrimaryBrush"), true);
        kindText.VerticalAlignment(VerticalAlignment::Center);
        Grid::SetColumn(kindText, 1);
        row.Children().Append(kindText);

        auto titleText = MakeText(title, 13, BrushRes(L"TextSecondaryBrush"));
        titleText.VerticalAlignment(VerticalAlignment::Center);
        titleText.TextTrimming(TextTrimming::CharacterEllipsis);
        titleText.TextWrapping(TextWrapping::NoWrap);
        Grid::SetColumn(titleText, 2);
        row.Children().Append(titleText);

        auto when = MakeText(timeText, 12, BrushRes(L"TextTertiaryBrush"));
        when.VerticalAlignment(VerticalAlignment::Center);
        Grid::SetColumn(when, 3);
        row.Children().Append(when);

        if (atTop)
        {
            RecentActivityPanel().Children().InsertAt(0, row);
        }
        else
        {
            RecentActivityPanel().Children().Append(row);
        }
        RecentActivityEmpty().Visibility(Visibility::Collapsed);
    }

    // ============================================================
    // "Splat the keyboard" gesture
    // ============================================================

    void MainWindow::InitializeSplatTracking()
    {
        if (auto content = this->Content())
        {
            content.KeyDown({ this, &MainWindow::OnContentKeyDown });
            content.KeyUp({ this, &MainWindow::OnContentKeyUp });
        }

        m_splatResetTimer = DispatcherTimer();
        m_splatResetTimer.Interval(std::chrono::milliseconds(500));
        m_splatResetTimer.Tick({ this, &MainWindow::OnSplatResetTick });
    }

    void MainWindow::OnContentKeyDown(IInspectable const&, Input::KeyRoutedEventArgs const& args)
    {
        // Ignore key-DOWN triggering while the feedback dialog is open, but keep
        // tracking accurate via key-UP below.
        if (m_feedbackDialogOpen)
        {
            return;
        }

        // Start a fresh cluster (and its stale-cluster safety timer) when empty.
        if (m_heldKeys.empty())
        {
            m_splatResetTimer.Start();
        }

        m_heldKeys.insert(args.Key());

        if (m_heldKeys.size() >= kSplatThreshold)
        {
            // Clear so the gesture doesn't immediately re-fire.
            m_heldKeys.clear();
            m_splatResetTimer.Stop();

            [](MainWindow* self) -> fire_and_forget
            {
                co_await self->ShowFeedbackDialogAsync();
            }(this);
        }
    }

    void MainWindow::OnContentKeyUp(IInspectable const&, Input::KeyRoutedEventArgs const& args)
    {
        m_heldKeys.erase(args.Key());
        if (m_heldKeys.empty())
        {
            m_splatResetTimer.Stop();
        }
    }

    void MainWindow::OnSplatResetTick(IInspectable const&, IInspectable const&)
    {
        // A key-up was dropped and the cluster went stale; reset.
        m_heldKeys.clear();
        m_splatResetTimer.Stop();
    }

    // ============================================================
    // Crash-types sheet
    // ============================================================

    IAsyncAction MainWindow::ShowCrashSheetAsync()
    {
        ContentDialog dialog;
        dialog.XamlRoot(this->Content().XamlRoot());
        ApplyDialogChrome(dialog);

        Grid root;
        root.Width(420);
        {
            RowDefinition r0; r0.Height(GridLengthHelper::FromValueAndType(0, GridUnitType::Auto));
            root.RowDefinitions().Append(r0);   // header
            RowDefinition r1; r1.Height(GridLengthHelper::FromValueAndType(0, GridUnitType::Auto));
            root.RowDefinitions().Append(r1);   // scrollable list
        }

        Button closeButton = MakeCloseButton();
        closeButton.Click([dialog](auto&&, auto&&) { dialog.Hide(); });
        auto header = MakeDialogHeader(L"Trigger a crash", closeButton);
        Grid::SetRow(header, 0);
        root.Children().Append(header);

        StackPanel list;
        list.Spacing(14);
        list.Margin(ThicknessHelper::FromLengths(0, 14, 0, 0));
        Grid::SetRow(list, 1);

        struct CrashRow { const wchar_t* title; const wchar_t* desc; void (*fn)(); };
        struct CrashGroup { const wchar_t* name; std::vector<CrashRow> rows; };

        const std::vector<CrashGroup> groups = {
            { L"Common", {
                { L"Memory Exception",       L"Read from an invalid memory address",        [] { CrashExamples::MemoryException(); } },
                { L"Divide By Zero",         L"Integer division by zero",                    [] { CrashExamples::DivideByZero(); } },
                { L"Stack Overflow",         L"Unbounded recursion exhausts the stack",      [] { CrashExamples::StackOverflow(nullptr); } },
                { L"Throw C++ Exception",    L"Uncaught std::exception terminates",          [] { CrashExamples::ThrowByUser(); } },
                { L"Vector Out of Bounds",   L"Indexing past the end of a std::vector",      [] { CrashExamples::OutOfBoundsVectorCrash(); } },
            } },
            { L"Memory corruption", {
                { L"Use After Free",         L"Dereference freed heap memory",               [] { CrashExamples::UseAfterFree(); } },
                { L"Double Delete",          L"Delete the same pointer twice",               [] { CrashExamples::DoubleDelete(); } },
                { L"Heap Corruption (ASAN)", L"Detected on AddressSanitizer builds",         [] { CrashExamples::HeapCorruption(); } },
                { L"Stack Overrun",          L"Write past a stack buffer",                   [] { CrashExamples::StackOverrun(); } },
            } },
            { L"System-level", {
                { L"Privileged Instruction", L"Execute a ring-0 instruction",                [] { CrashExamples::PrivilegedInstruction(); } },
                { L"Invalid Function Pointer", L"Call through a bad function pointer",       [] { CrashExamples::InvalidFunctionPointer(); } },
                { L"Fast Fail",              L"__fastfail security check",                   [] { CrashExamples::FastFail(); } },
                { L"Invalid Parameters",     L"CRT invalid-parameter handler",               [] { CrashExamples::InvalidParameters(); } },
                { L"Pure Virtual Call",      L"Call a pure virtual during construction",     [] { CrashExamples::VirtualFunctionCallCrash(); } },
            } },
            { L"Threading", {
                { L"Thread Exception",       L"Crash on a worker thread",                    [] { CrashExamples::ThreadException(1); } },
            } },
            { L"Other", {
                { L"abort()",                L"Call the CRT abort()",                        [] { CrashExamples::CallAbort(); } },
                { L"Exhaust Memory",         L"Allocate until out of memory",                [] { CrashExamples::ExhaustMemory(); } },
            } },
            { L"Advanced", {
                { L"Custom SEH Exception",   L"Raise a custom structured exception",         [] { CrashExamples::CustomSEHException(); } },
                { L"Create XML Report",      L"Non-fatal \u2013 app keeps running",          [] { CrashExamples::CreateXmlReport(); } },
            } },
        };

        for (auto const& group : groups)
        {
            list.Children().Append(MakeText(hstring{ group.name }, 11, BrushRes(L"TextTertiaryBrush"), true));

            Border card;
            card.Style(StyleRes(L"DialogCardStyle"));
            StackPanel rows;
            rows.Spacing(0);

            for (size_t i = 0; i < group.rows.size(); ++i)
            {
                auto const& cr = group.rows[i];

                Button rowButton;
                rowButton.HorizontalAlignment(HorizontalAlignment::Stretch);
                rowButton.HorizontalContentAlignment(HorizontalAlignment::Left);
                rowButton.Background(SolidColorBrush(Microsoft::UI::Colors::Transparent()));
                rowButton.BorderThickness(ThicknessHelper::FromUniformLength(0));
                rowButton.Padding(ThicknessHelper::FromLengths(12, 10, 12, 10));

                StackPanel rowContent;
                rowContent.Spacing(2);
                rowContent.Children().Append(MakeText(hstring{ cr.title }, 14, BrushRes(L"TextPrimaryBrush"), true));
                rowContent.Children().Append(MakeText(hstring{ cr.desc }, 12, BrushRes(L"TextSecondaryBrush")));
                rowButton.Content(rowContent);

                auto fn = cr.fn;
                rowButton.Click([dialog, fn](auto&&, auto&&)
                {
                    dialog.Hide();
                    fn();
                });
                rows.Children().Append(rowButton);

                if (i + 1 < group.rows.size())
                {
                    Border sep;
                    sep.Height(1);
                    sep.Background(BrushRes(L"CardStrokeBrush"));
                    sep.Margin(ThicknessHelper::FromLengths(12, 0, 12, 0));
                    rows.Children().Append(sep);
                }
            }

            card.Child(rows);
            list.Children().Append(card);
        }

        ScrollViewer scroller;
        scroller.VerticalScrollBarVisibility(ScrollBarVisibility::Auto);
        scroller.MaxHeight(460);
        scroller.Content(list);
        Grid::SetRow(scroller, 1);
        root.Children().Append(scroller);

        dialog.Content(root);
        co_await dialog.ShowAsync();
    }

    // ============================================================
    // Feedback dialog (two-state: form -> thank-you)
    // ============================================================

    IAsyncAction MainWindow::ShowFeedbackDialogAsync()
    {
        // Keep this alive across the suspensions below; members are touched after the await.
        auto lifetime = get_strong();

        if (m_feedbackDialogOpen)
        {
            co_return;
        }
        m_feedbackDialogOpen = true;

        // Reset the open flag (and gesture state) on every exit path — including if
        // ShowAsync throws (e.g. another ContentDialog is already open). Otherwise the
        // flag stays true and the feedback dialog/gesture is suppressed for the session.
        auto reset = MakeScopeGuard([this]
        {
            m_feedbackDialogOpen = false;
            m_heldKeys.clear();
        });

        ContentDialog dialog;
        dialog.XamlRoot(this->Content().XamlRoot());
        ApplyDialogChrome(dialog);

        // Shared, mutable state captured by the inner lambdas.
        auto attachmentPath = std::make_shared<std::wstring>();
        auto category = std::make_shared<std::wstring>(L"Bug");
        auto resultCrashId = std::make_shared<int>(0);

        // ---- Root overlay: form panel + thank-you panel ----
        Grid root;
        root.Width(460);

        // =========================== FORM ===========================
        StackPanel form;
        form.Spacing(14);

        Button closeButton = MakeCloseButton();
        closeButton.Click([dialog](auto&&, auto&&) { dialog.Hide(); });
        form.Children().Append(MakeDialogHeader(L"Send feedback", closeButton));

        // Category segmented control (Bug / Feature / Other)
        Border segWrap;
        segWrap.Background(BrushRes(L"BadgeBgBrush"));
        segWrap.CornerRadius(CornerRadiusHelper::FromUniformRadius(8));
        segWrap.Padding(ThicknessHelper::FromUniformLength(3));
        Grid segGrid;
        segGrid.ColumnSpacing(3);
        std::array<hstring, 3> segLabels{ L"Bug", L"Feature", L"Other" };
        auto segButtons = std::make_shared<std::array<Button, 3>>();
        for (int i = 0; i < 3; ++i)
        {
            ColumnDefinition c; c.Width(GridLengthHelper::FromValueAndType(1, GridUnitType::Star));
            segGrid.ColumnDefinitions().Append(c);
        }
        auto selectSegment = [segButtons, category, segLabels](int idx)
        {
            for (int i = 0; i < 3; ++i)
            {
                bool sel = (i == idx);
                auto& b = (*segButtons)[i];
                b.Background(sel ? BrushRes(L"CardBgBrush") : SolidColorBrush(Microsoft::UI::Colors::Transparent()));
                b.Foreground(sel ? BrushRes(L"TextPrimaryBrush") : BrushRes(L"TextSecondaryBrush"));
                b.FontWeight(sel ? Microsoft::UI::Text::FontWeights::SemiBold() : Microsoft::UI::Text::FontWeights::Normal());
            }
            *category = std::wstring(segLabels[idx]);
        };
        for (int i = 0; i < 3; ++i)
        {
            Button seg;
            seg.Content(box_value(segLabels[i]));
            seg.HorizontalAlignment(HorizontalAlignment::Stretch);
            seg.BorderThickness(ThicknessHelper::FromUniformLength(0));
            seg.CornerRadius(CornerRadiusHelper::FromUniformRadius(6));
            seg.Padding(ThicknessHelper::FromLengths(0, 6, 0, 6));
            seg.FontSize(13);
            int idx = i;
            seg.Click([selectSegment, idx](auto&&, auto&&) { selectSegment(idx); });
            (*segButtons)[i] = seg;
            Grid::SetColumn(seg, i);
            segGrid.Children().Append(seg);
        }
        segWrap.Child(segGrid);
        form.Children().Append(segWrap);
        selectSegment(0);

        // Title (required)
        {
            StackPanel labelRow;
            labelRow.Orientation(Orientation::Horizontal);
            labelRow.Spacing(3);
            labelRow.Children().Append(MakeText(L"Title", 13, BrushRes(L"TextSecondaryBrush"), true));
            labelRow.Children().Append(MakeText(L"*", 13, BrushRes(L"AsteriskBrush"), true));
            form.Children().Append(labelRow);
        }
        TextBox titleBox;
        titleBox.Style(StyleRes(L"BoxedTextBoxStyle"));
        titleBox.PlaceholderText(L"Brief summary");
        form.Children().Append(titleBox);

        // Description (optional)
        form.Children().Append(MakeText(L"Description", 13, BrushRes(L"TextSecondaryBrush"), true));
        TextBox descriptionBox;
        descriptionBox.Style(StyleRes(L"BoxedTextBoxStyle"));
        descriptionBox.PlaceholderText(L"What were you doing when the issue occurred? (optional)");
        descriptionBox.AcceptsReturn(true);
        descriptionBox.TextWrapping(TextWrapping::Wrap);
        descriptionBox.Height(84);
        descriptionBox.VerticalContentAlignment(VerticalAlignment::Top);
        form.Children().Append(descriptionBox);

        // Name + Email (optional)
        Grid nameEmail;
        nameEmail.ColumnSpacing(12);
        {
            ColumnDefinition a; a.Width(GridLengthHelper::FromValueAndType(1, GridUnitType::Star)); nameEmail.ColumnDefinitions().Append(a);
            ColumnDefinition b; b.Width(GridLengthHelper::FromValueAndType(1, GridUnitType::Star)); nameEmail.ColumnDefinitions().Append(b);
        }
        StackPanel nameCol; nameCol.Spacing(6);
        nameCol.Children().Append(MakeText(L"Name", 13, BrushRes(L"TextSecondaryBrush"), true));
        TextBox nameBox; nameBox.Style(StyleRes(L"BoxedTextBoxStyle")); nameBox.PlaceholderText(L"optional");
        nameCol.Children().Append(nameBox);
        Grid::SetColumn(nameCol, 0);
        nameEmail.Children().Append(nameCol);

        StackPanel emailCol; emailCol.Spacing(6);
        emailCol.Children().Append(MakeText(L"Email", 13, BrushRes(L"TextSecondaryBrush"), true));
        TextBox emailBox; emailBox.Style(StyleRes(L"BoxedTextBoxStyle")); emailBox.PlaceholderText(L"optional");
        emailCol.Children().Append(emailBox);
        Grid::SetColumn(emailCol, 1);
        nameEmail.Children().Append(emailCol);
        form.Children().Append(nameEmail);

        // Attachment row
        form.Children().Append(MakeText(L"Attachment", 13, BrushRes(L"TextSecondaryBrush"), true));
        Border attachCard;
        attachCard.Style(StyleRes(L"InputCardStyle"));
        Grid attachGrid;
        attachGrid.ColumnSpacing(12);
        {
            ColumnDefinition c0; c0.Width(GridLengthHelper::FromValueAndType(0, GridUnitType::Auto)); attachGrid.ColumnDefinitions().Append(c0);
            ColumnDefinition c1; c1.Width(GridLengthHelper::FromValueAndType(1, GridUnitType::Star)); attachGrid.ColumnDefinitions().Append(c1);
            ColumnDefinition c2; c2.Width(GridLengthHelper::FromValueAndType(0, GridUnitType::Auto)); attachGrid.ColumnDefinitions().Append(c2);
        }

        Border typeChip;
        typeChip.Width(40);
        typeChip.Height(40);
        typeChip.CornerRadius(CornerRadiusHelper::FromUniformRadius(6));
        typeChip.Background(BrushRes(L"BadgeBgBrush"));
        typeChip.VerticalAlignment(VerticalAlignment::Center);
        auto typeChipText = MakeText(L"", 10, BrushRes(L"TextTertiaryBrush"), true);
        typeChipText.HorizontalAlignment(HorizontalAlignment::Center);
        typeChipText.VerticalAlignment(VerticalAlignment::Center);
        typeChipText.TextAlignment(TextAlignment::Center);
        typeChip.Child(typeChipText);
        typeChip.Visibility(Visibility::Collapsed);
        Grid::SetColumn(typeChip, 0);
        attachGrid.Children().Append(typeChip);

        StackPanel attachInfo;
        attachInfo.Spacing(2);
        attachInfo.VerticalAlignment(VerticalAlignment::Center);
        Grid::SetColumn(attachInfo, 1);
        auto attachName = MakeText(L"No file selected", 13, BrushRes(L"TextSecondaryBrush"));
        attachName.TextTrimming(TextTrimming::CharacterEllipsis);
        attachName.TextWrapping(TextWrapping::NoWrap);
        auto attachDetail = MakeText(L"", 12, BrushRes(L"TextTertiaryBrush"));
        attachDetail.Visibility(Visibility::Collapsed);
        attachInfo.Children().Append(attachName);
        attachInfo.Children().Append(attachDetail);
        attachGrid.Children().Append(attachInfo);

        Button addButton;
        addButton.Content(box_value(L"Add"));
        addButton.VerticalAlignment(VerticalAlignment::Center);
        Grid::SetColumn(addButton, 2);
        attachGrid.Children().Append(addButton);
        attachCard.Child(attachGrid);
        form.Children().Append(attachCard);

        addButton.Click([=](auto&&, auto&&)
        {
            std::wstring picked = PickFile();
            if (picked.empty()) return;

            uint64_t size = GetFileSizeBytes(picked);
            if (size > kMaxAttachmentSize)
            {
                attachName.Text(L"File must be under 10 MB");
                attachName.Foreground(BrushRes(L"AsteriskBrush"));
                return;
            }

            *attachmentPath = picked;
            std::wstring name = FileNameOf(picked);
            attachName.Text(hstring{ MiddleTruncate(name, 36) });
            attachName.Foreground(BrushRes(L"TextPrimaryBrush"));
            attachDetail.Text(hstring{ FormatBytes(size) });
            attachDetail.Visibility(Visibility::Visible);
            typeChipText.Text(hstring{ FileExtUpper(name) });
            typeChip.Visibility(Visibility::Visible);
            addButton.Content(box_value(L"Replace"));
        });

        // Include logs toggle (default ON)
        Grid logsRow;
        logsRow.ColumnSpacing(12);
        {
            ColumnDefinition a; a.Width(GridLengthHelper::FromValueAndType(1, GridUnitType::Star)); logsRow.ColumnDefinitions().Append(a);
            ColumnDefinition b; b.Width(GridLengthHelper::FromValueAndType(0, GridUnitType::Auto)); logsRow.ColumnDefinitions().Append(b);
        }
        StackPanel logsText;
        logsText.Spacing(2);
        logsText.VerticalAlignment(VerticalAlignment::Center);
        logsText.Children().Append(MakeText(L"Include logs", 13, BrushRes(L"TextPrimaryBrush"), true));
        logsText.Children().Append(MakeText(L"Attach the app's sample log", 12, BrushRes(L"TextTertiaryBrush")));
        Grid::SetColumn(logsText, 0);
        logsRow.Children().Append(logsText);
        ToggleSwitch logsToggle;
        logsToggle.IsOn(true);
        logsToggle.OffContent(box_value(L""));
        logsToggle.OnContent(box_value(L""));
        logsToggle.VerticalAlignment(VerticalAlignment::Center);
        // The default ToggleSwitch reserves a wide (~154px) content area to the right of the
        // switch; drop MinWidth to 0 and right-align so the knob sits flush with the form edge.
        logsToggle.MinWidth(0);
        logsToggle.HorizontalAlignment(HorizontalAlignment::Right);
        Grid::SetColumn(logsToggle, 1);
        logsRow.Children().Append(logsToggle);
        form.Children().Append(logsRow);

        // Inline error (hidden by default)
        auto errorLabel = MakeText(L"", 13, BrushRes(L"AsteriskBrush"));
        errorLabel.Visibility(Visibility::Collapsed);
        form.Children().Append(errorLabel);

        // Footer bar with full-width accent Send button + spinner
        Border footer;
        footer.Background(BrushRes(L"FooterBgBrush"));
        footer.CornerRadius(CornerRadiusHelper::FromUniformRadius(8));
        footer.Padding(ThicknessHelper::FromUniformLength(12));
        footer.Margin(ThicknessHelper::FromLengths(0, 4, 0, 0));
        Grid footerGrid;
        Button sendButton;
        sendButton.Style(StyleRes(L"AccentPrimaryButtonStyle"));
        sendButton.Content(box_value(L"Send feedback \u2192"));
        sendButton.IsEnabled(false);
        footerGrid.Children().Append(sendButton);
        ProgressRing spinner;
        spinner.Width(22);
        spinner.Height(22);
        spinner.IsActive(false);
        spinner.Visibility(Visibility::Collapsed);
        spinner.HorizontalAlignment(HorizontalAlignment::Center);
        spinner.VerticalAlignment(VerticalAlignment::Center);
        footerGrid.Children().Append(spinner);
        footer.Child(footerGrid);
        form.Children().Append(footer);

        // Enable Send only when the trimmed title is non-empty.
        titleBox.TextChanged([titleBox, sendButton](auto&&, auto&&)
        {
            sendButton.IsEnabled(!Trim(std::wstring(titleBox.Text().c_str())).empty());
        });

        // =========================== THANK YOU ===========================
        StackPanel thankYou;
        thankYou.Spacing(14);
        thankYou.Visibility(Visibility::Collapsed);
        thankYou.Opacity(0);

        // Green check in a circle
        Grid checkWrap;
        checkWrap.Width(56);
        checkWrap.Height(56);
        checkWrap.HorizontalAlignment(HorizontalAlignment::Center);
        Shapes::Ellipse circle;
        circle.Width(56);
        circle.Height(56);
        circle.Fill(AccentBrushWithOpacity(L"FeedbackAccentBrush", 0.14));
        circle.Stroke(AccentBrushWithOpacity(L"FeedbackAccentBrush", 0.35));
        circle.StrokeThickness(1.5);
        checkWrap.Children().Append(circle);
        auto checkMark = MakeText(L"\u2713", 26, BrushRes(L"FeedbackAccentBrush"), true);
        checkMark.HorizontalAlignment(HorizontalAlignment::Center);
        checkMark.VerticalAlignment(VerticalAlignment::Center);
        checkWrap.Children().Append(checkMark);
        thankYou.Children().Append(checkWrap);

        auto thanksTitle = MakeText(L"Feedback sent. Thanks!", 22, BrushRes(L"TextPrimaryBrush"), true);
        thanksTitle.FontWeight(Microsoft::UI::Text::FontWeights::Bold());
        thanksTitle.HorizontalAlignment(HorizontalAlignment::Center);
        thanksTitle.TextAlignment(TextAlignment::Center);
        thankYou.Children().Append(thanksTitle);

        auto thanksMessage = MakeText(L"Your note made it to the BugSplat team. We reply within a day.", 14, BrushRes(L"TextSecondaryBrush"));
        thanksMessage.HorizontalAlignment(HorizontalAlignment::Center);
        thanksMessage.TextAlignment(TextAlignment::Center);
        thankYou.Children().Append(thanksMessage);

        // Report id card
        Border idCard;
        idCard.Background(BrushRes(L"BadgeBgBrush"));
        idCard.CornerRadius(CornerRadiusHelper::FromUniformRadius(8));
        idCard.Padding(ThicknessHelper::FromLengths(14, 10, 14, 10));
        Grid idGrid;
        idGrid.ColumnSpacing(12);
        {
            ColumnDefinition a; a.Width(GridLengthHelper::FromValueAndType(1, GridUnitType::Star)); idGrid.ColumnDefinitions().Append(a);
            ColumnDefinition b; b.Width(GridLengthHelper::FromValueAndType(0, GridUnitType::Auto)); idGrid.ColumnDefinitions().Append(b);
        }
        StackPanel idText;
        idText.Spacing(2);
        idText.VerticalAlignment(VerticalAlignment::Center);
        auto idLabel = MakeText(L"REPORT ID", 10, BrushRes(L"TextTertiaryBrush"), true);
        auto idValue = MakeText(L"", 15, BrushRes(L"TextPrimaryBrush"), true);
        idValue.FontFamily(Media::FontFamily(L"Consolas"));
        idText.Children().Append(idLabel);
        idText.Children().Append(idValue);
        Grid::SetColumn(idText, 0);
        idGrid.Children().Append(idText);
        Button copyButton;
        copyButton.Content(box_value(L"Copy"));
        copyButton.VerticalAlignment(VerticalAlignment::Center);
        Grid::SetColumn(copyButton, 1);
        idGrid.Children().Append(copyButton);
        idCard.Child(idGrid);
        thankYou.Children().Append(idCard);

        copyButton.Click([idValue](auto&&, auto&&)
        {
            Windows::ApplicationModel::DataTransfer::DataPackage pkg;
            pkg.SetText(idValue.Text());
            Windows::ApplicationModel::DataTransfer::Clipboard::SetContent(pkg);
        });

        // Thank-you footer: View on dashboard + Close + Powered by BugSplat
        StackPanel thankFooter;
        thankFooter.Spacing(10);

        // Primary action on top (full-width accent), Close underneath.
        Button dashButton;
        dashButton.Style(StyleRes(L"AccentPrimaryButtonStyle"));
        dashButton.Content(box_value(L"View on dashboard \u2197"));
        dashButton.Click([this, resultCrashId](auto&&, auto&&) { OpenDashboard(*resultCrashId); });
        thankFooter.Children().Append(dashButton);

        Button closeThanks;
        closeThanks.Content(box_value(L"Close"));
        closeThanks.Foreground(BrushRes(L"TextSecondaryBrush"));
        closeThanks.Background(SolidColorBrush(Microsoft::UI::Colors::Transparent()));
        closeThanks.BorderThickness(ThicknessHelper::FromUniformLength(0));
        closeThanks.HorizontalAlignment(HorizontalAlignment::Stretch);
        closeThanks.Click([dialog](auto&&, auto&&) { dialog.Hide(); });
        thankFooter.Children().Append(closeThanks);

        HyperlinkButton poweredBy;
        poweredBy.Content(box_value(L"Powered by BugSplat"));
        poweredBy.NavigateUri(Uri{ L"https://bugsplat.com" });
        poweredBy.HorizontalAlignment(HorizontalAlignment::Center);
        poweredBy.FontSize(12);
        thankFooter.Children().Append(poweredBy);

        thankYou.Children().Append(thankFooter);

        // ---- Assemble overlay ----
        root.Children().Append(form);
        root.Children().Append(thankYou);
        dialog.Content(root);

        // ---- Submit handler ----
        sendButton.Click([=](auto&&, auto&&) -> fire_and_forget
        {
            // Keep this alive across resume_background(): the window can close mid-upload.
            auto lifetime = get_strong();
            std::wstring title = Trim(std::wstring(titleBox.Text().c_str()));
            if (title.empty())
            {
                co_return;
            }

            // Enter loading state: disable inputs, show spinner.
            form.IsHitTestVisible(false);
            form.Opacity(0.6);
            sendButton.Visibility(Visibility::Collapsed);
            spinner.Visibility(Visibility::Visible);
            spinner.IsActive(true);
            errorLabel.Visibility(Visibility::Collapsed);

            std::wstring description = std::wstring(descriptionBox.Text().c_str());
            std::wstring name = std::wstring(nameBox.Text().c_str());
            std::wstring email = std::wstring(emailBox.Text().c_str());
            std::wstring cat = *category;
            bool includeLogs = logsToggle.IsOn();

            // Build attachment list: picked file + sample log (if toggle on).
            std::vector<std::wstring> attachStore;
            if (!attachmentPath->empty())
            {
                attachStore.push_back(*attachmentPath);
            }
            if (includeLogs)
            {
                std::wstring logPath = GetSampleLogPath();
                if (GetFileAttributesW(logPath.c_str()) != INVALID_FILE_ATTRIBUTES)
                {
                    attachStore.push_back(logPath);
                }
            }

            g_bugsplat.SetUser(name.c_str());
            g_bugsplat.SetEmail(email.c_str());
            g_bugsplat.SetAttribute(L"category", cat.c_str());

            std::vector<const wchar_t*> attachments;
            for (auto const& a : attachStore) attachments.push_back(a.c_str());

            // PostFeedback blocks on the monitor process; run it off the UI thread.
            co_await winrt::resume_background();
            FeedbackResult fr = g_bugsplat.PostFeedbackWithResult(title.c_str(), description.c_str(), attachments);
            co_await wil::resume_foreground(this->DispatcherQueue());

            spinner.IsActive(false);
            spinner.Visibility(Visibility::Collapsed);

            if (!fr.success)
            {
                // Stay on the form, keep input, surface the error.
                form.IsHitTestVisible(true);
                form.Opacity(1.0);
                sendButton.Visibility(Visibility::Visible);
                errorLabel.Text(L"Couldn't send your feedback. Please try again.");
                errorLabel.Visibility(Visibility::Visible);
                co_return;
            }

            // Success: record id, log activity, cross-fade to thank-you.
            *resultCrashId = fr.crashId;
            if (fr.crashId > 0)
            {
                idValue.Text(hstring{ std::to_wstring(fr.crashId) });
                copyButton.Visibility(Visibility::Visible);
            }
            else
            {
                idValue.Text(L"Unavailable");
                copyButton.Visibility(Visibility::Collapsed);
            }

            AddRecentActivity(L"Feedback", hstring{ L"\u201C" + title + L"\u201D" }, L"just now", true);

            thankYou.Visibility(Visibility::Visible);

            auto makeFade = [](UIElement const& target, double from, double to)
            {
                DoubleAnimation da;
                da.From(from);
                da.To(to);
                da.Duration(Duration{ std::chrono::milliseconds(280) });
                Storyboard::SetTarget(da, target);
                Storyboard::SetTargetProperty(da, L"Opacity");
                return da;
            };
            Storyboard sb;
            sb.Children().Append(makeFade(form, 1.0, 0.0));
            sb.Children().Append(makeFade(thankYou, 0.0, 1.0));
            sb.Completed([form](auto&&, auto&&) { form.Visibility(Visibility::Collapsed); });
            sb.Begin();
        });

        co_await dialog.ShowAsync();
        // m_feedbackDialogOpen / m_heldKeys reset by the ScopeGuard above.
    }
}
