# MyDotNetCrasher

A headless console (.NET 10) sample that reports crashes, non-fatal errors, and user feedback
with the `BugSplatDotNet` SDK (`BugSplatDotNet.dll`, in `..\..\BugSplat\dotnet\`). It is the
terminal companion to the GUI [`MyDotNetWinUI3Crasher`](../MyDotNetWinUI3Crasher) and mirrors the
C++ `MyConsoleCrasher` sample from the native BugSplat SDK.

## Run

```
dotnet run -- <mode>
```

Set your database at the top of `Program.cs` (`Database`), then watch events arrive at
`https://app.bugsplat.com/v2/dashboard?database=<Database>`. In the SDK download the `Database`
line is commented out behind an `#error`: set it, uncomment it, and delete the `#error`. It is **x64 only** — it
links the native `BugSplat.dll`, which ships x64 in the SDK.

## Symbols

So crash stacks show file names and line numbers, each build uploads the exe, DLLs and PDBs to
BugSplat with `Scripts\SymbolUpload.ps1`, using `Database`, `Application` and `Version` from
`Program.cs`. Create a Client ID and Client Secret for your database at
<https://app.bugsplat.com/v2/database/integrations#oauth>, then create `Scripts\env.ps1`:

```powershell
$BUGSPLAT_CLIENT_ID = "{{id}}"
$BUGSPLAT_CLIENT_SECRET = "{{secret}}"
```

or set the `BUGSPLAT_CLIENT_ID` and `BUGSPLAT_CLIENT_SECRET` environment variables. To build
without uploading, pass `/p:BugSplatSymbolUpload=false`.

## What it demonstrates

Every report is a minidump that BugSplat symbolicates from uploaded symbols, so managed frames
show file names and line numbers without shipping PDBs.

| Mode | Path exercised |
| --- | --- |
| `managed` | Unhandled C# exception → BugSplat's in-process filter captures a minidump. |
| `handled` | A caught C# exception reported with `BugSplat.Post(ex)` from inside the `catch` → a minidump with the throw-site frames; the process keeps running. |
| `native` | A hardware access violation in C# code. The CLR treats it as a fatal error and ends the process through WER, so it's captured only by `BugSplatWer.dll` (see *WER* below). |
| `feedback` | `BugSplat.PostFeedback(...)`, returning a report id + info URL (no crash). |
| `deep` / `generic` / `lambda` / `async` | Managed faults that fault deep / from a closed generic / a LINQ lambda / an async state machine — fixtures for the managed stack walk + method naming. |

### Mixed-mode C#/C++ (P/Invoke into `MyDotNetCrasherNative.dll`)

These fault across the managed→native boundary, so the crash stack is one unified C#/C++ trace —
the cases BugSplat and [`bugsplat-cdb`](https://github.com/BugSplat-Git/bugsplat-cdb)
symbolicate as a single interleaved stack:

| Mode | Shape |
| --- | --- |
| `native-av` | Managed → native access violation (native frame on top). |
| `native-deep` | Several native C++ frames beneath the managed transition. |
| `native-callback` | Managed → native → managed callback that throws (M/N/M interleave). |
| `cpp-throw` | Uncaught C++ exception in native code. |
| `native-so` | Cross-boundary stack overflow (native ↔ managed mutual recursion). Needs WER, see below. |
| `native-thread` | A native background thread with no managed frames faults. |

**WER-class** mixed-mode modes — fail-fasts the OS routes straight through WerFault, bypassing
the CLR and BugSplat's in-process filter, so they are captured **only** by the allowlisted
`BugSplatWer.dll` runtime-exception helper (see *WER* below). They mirror `MyConsoleCrasher`'s
WER trio:

| Mode | Shape |
| --- | --- |
| `native-fastfail` | `__fastfail` from native code (`0xC0000409`). |
| `native-overrun` | `/GS` stack-cookie smash (`0xC0000409`). |
| `native-double-delete` | Heap-metadata corruption via double free (`0xC0000374`). |

The mixed-mode modes require `MyDotNetCrasherNative.dll`: build
[`..\MyDotNetCrasherNative`](../MyDotNetCrasherNative) (x64, MSBuild) first — this project copies
it next to the exe.

For the opposite direction — a native C++ program that hosts the runtime and crashes in C# code it
called — see [`MyDotNetFrameworkHostCrasher`](../MyDotNetFrameworkHostCrasher), which does it with
the .NET Framework.

## WER (for the access violation, fail-fast and stack overflow modes)

The `native` / `native-av` / `native-deep` / `native-fastfail` / `native-overrun` /
`native-double-delete` / `native-so` modes are captured through BugSplat's WER runtime-exception
helper (`BugSplatWer.dll`), which the SDK registers only when
the machine allowlists it under
`HKLM\SOFTWARE\Microsoft\Windows\Windows Error Reporting\RuntimeExceptionHelperModules` (a
`REG_DWORD` named with the full path to `BugSplatWer.dll`, created with administrator rights —
see step 3 of the [BugSplat for Windows (C++) guide](https://docs.bugsplat.com/integrations/desktop/cplusplus)).
The other modes use the in-process exception filter and need no such setup. .NET 10 treats an
access violation as a fatal error, even one in native code under a P/Invoke, so it never reaches
the in-process filter.

A stack overflow never reaches the in-process filter: the runtime ends the process through WER
itself. The runtime registers its own WER helper (`mscordaccore.dll`) when it starts, before
`BugSplat.dll` registers `BugSplatWer.dll`, and WER gives the crash to the first helper that
claims it, so `BugSplat.dll` re-registers the runtime's helper behind `BugSplatWer.dll`.

## Shipping the BugSplat native runtime

BugSplat captures crashes out-of-process: `BugSplat.dll` spawns `BugSplatMonitor.exe` from the
application's own directory, so `BugSplat.dll`, `BugSplatMonitor.exe`, `BugSplatReporter.exe`, and
`BugSplatWer.dll` **must sit next to your executable at run time**. This project copies them
(plus `MyDotNetCrasherNative.dll` + its PDB) into the output on build and publish from
`$(BugSplatBin)`: the SDK's `x64\<Configuration>\bin` when it exists, otherwise `x64\Release\bin`
(override the SDK location with `/p:BugSplatDir=C:\path\to\BugSplat\`).
