# MyDotNetWinUI3Crasher

A WinUI 3 (.NET 10) sample that reports crashes, non-fatal errors, and user feedback with the
`BugSplatDotNet` SDK (`BugSplatDotNet.dll`, in `..\..\BugSplat\dotnet\`). It is the GUI companion
to the headless [`MyDotNetCrasher`](../MyDotNetCrasher) console sample and mirrors the C++
`MyWinUI3Crasher` sample from the native BugSplat SDK.

## Run

```
dotnet run -c Debug -p:Platform=x64
```

The app is **unpackaged** and **self-contained**, so it launches with no MSIX/certificate setup
and no separate Windows App SDK Runtime install. It is **x64 only** — it links the native
`BugSplat.dll`, which ships x64 in the SDK.

Set your database in `App.xaml.cs` (`App.Database`), then watch events arrive at
`https://app.bugsplat.com/v2/dashboard?database=<Database>`. In the SDK download the `Database`
line is commented out behind an `#error`: set it, uncomment it, and delete the `#error`.

## Symbols

So crash stacks show file names and line numbers, each build uploads the exe, DLLs and PDBs to
BugSplat with `Scripts\SymbolUpload.ps1`, using `App.Database`, `App.Application` and
`App.Version` from `App.xaml.cs`. Create a Client ID and Client Secret for your database at
<https://app.bugsplat.com/v2/database/integrations#oauth>, then create `Scripts\env.ps1`:

```powershell
$BUGSPLAT_CLIENT_ID = "{{id}}"
$BUGSPLAT_CLIENT_SECRET = "{{secret}}"
```

or set the `BUGSPLAT_CLIENT_ID` and `BUGSPLAT_CLIENT_SECRET` environment variables. To build
without uploading, pass `/p:BugSplatSymbolUpload=false`.

## Configure WER (required for crash capture)

A WinUI 3 app's crashes are captured through BugSplat's WER runtime-exception helper
(`BugSplatWer.dll`), which the SDK registers only when this machine allowlists it under

```
HKLM\SOFTWARE\Microsoft\Windows\Windows Error Reporting\RuntimeExceptionHelperModules
```

Add a `REG_DWORD` value **named with the full path** to the `BugSplatWer.dll` sitting next to
the built exe (value data `0`). This needs administrator rights; see step 3 of the
[BugSplat for Windows (C++) guide](https://docs.bugsplat.com/integrations/desktop/cplusplus)
for details. If it isn't configured, `App` checks `BugSplat.IsWerEnabled` at startup and shows
a warning — crashes will not be reported until the key is present. (Most of the console sample's
modes are caught by BugSplat's application exception handler, but its fail-fast modes need the
same registration.)

## What it demonstrates

| Card | Path exercised |
| --- | --- |
| **Crash** | An unhandled managed exception: WinUI fail-fasts and BugSplat captures a minidump through WER. |
| **Non-Crash Error** | `BugSplat.Post(exception)` inside the `catch` writes a minidump of the caught exception and uploads it; the app keeps running. |
| **User Feedback** | `BugSplat.PostFeedback(...)`, returning a crash id + info URL. |
| **Hang** | Freezes the UI thread for good. BugSplatMonitor notices within about 10 seconds. Because the sample doesn't set `QuietMode`, it then asks whether to end the app and send a report, and shows the Crash Report dialog; the report goes out once both are answered, and the app is ended. Answering **No** waits 30 seconds before the normal 5-second polling cycle resumes. |
| **Mixed-Mode Crash** | P/Invokes `MyDotNetCrasherNative.dll` into a native access violation; BugSplat symbolicates one unified C#/C++ stack. Build `MyDotNetWinUI3Crasher.sln`, which builds `..\MyDotNetCrasherNative` first. |

A crash report shows BugSplatMonitor's Crash Report dialog before it is posted, so the user can
describe what happened: `App` sets `BugSplat.QuietMode = false`, as a GUI app usually would. Set it
to `true` to post without asking, as the console [`MyDotNetCrasher`](../MyDotNetCrasher) does for
unattended runs.

Every report is a minidump that BugSplat symbolicates from uploaded symbols. Crash reporting
is wired once, in `App`:

```csharp
BugSplat = new BugSplat(Database, Application, Version);
```

To report a handled exception, call `Post` from inside its `catch`, while the throw-site frames
are still on the stack:

```csharp
try { DoWork(); }
catch (Exception ex) { App.BugSplat.Post(ex); }
```

## Shipping the BugSplat native runtime (reference pattern)

BugSplat captures crashes out-of-process: `BugSplat.dll` spawns `BugSplatMonitor.exe` from the
application's own directory, so `BugSplat.dll`, `BugSplatMonitor.exe`, `BugSplatReporter.exe`, and
`BugSplatWer.dll` **must sit next to your executable at run time**.

This sample declares them as `Content` with `CopyToOutputDirectory`:

```xml
<Content Include="$(BugSplatBin)BugSplat.dll" Link="BugSplat.dll" CopyToOutputDirectory="PreserveNewest" />
<!-- BugSplatMonitor.exe, BugSplatReporter.exe, BugSplatWer.dll -->
```

Use this pattern rather than a post-build `<Copy>` target: `Content` items flow to `dotnet build`,
`dotnet publish` (into `publish\`), **and** MSIX packaging automatically, so the runtime files
travel with your app in every configuration. `$(BugSplatBin)` points at the SDK's
`x64\<Configuration>\bin` when it exists, otherwise `x64\Release\bin`; override the SDK location
with `/p:BugSplatDir=C:\path\to\BugSplat\`.

## Packaging as MSIX

To ship packaged instead of unpackaged, remove `<WindowsPackageType>None</WindowsPackageType>`
and `<WindowsAppSDKSelfContained>true</WindowsAppSDKSelfContained>` from the `.csproj` and add a
Windows Application Packaging Project. The `Content` items above are included in the package
unchanged; `BugSplatMonitor.exe` is packaged as a plain data file (do not list it as an
`<Executable>` in the manifest).
