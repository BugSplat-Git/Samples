# MyDotNetFrameworkHostCrasher

A native C++ console program that starts the .NET Framework 4.7.2 runtime, calls into C# code,
and crashes there. BugSplat is initialized natively, before the runtime loads, so the report's
minidump has managed frames on top of native ones: the mixed-mode call stack this sample exists to
put in front of BugSplat.

Two projects make up the sample:

| Project | What it is |
| --- | --- |
| `MyDotNetFrameworkHostCrasher` (this folder) | x64 C++ console app. Initializes BugSplat from `BugSplat.lib`, starts the .NET Framework runtime through the CLR hosting interfaces, and calls `Crashers.Dispatch`. |
| [`MyDotNetFrameworkHostCrasherManaged`](../MyDotNetFrameworkHostCrasherManaged) | `net472` class library with the entry point and the crash modes. |

## Build

1. Set `BUGSPLAT_DATABASE` in `MyDotNetFrameworkHostCrasher.h`.
2. Create `Scripts\env.ps1` with the OAuth client id and secret the post build event uses to
   upload symbols (the header comment in `MyDotNetFrameworkHostCrasher.cpp` shows the two lines).
3. Build `MyDotNetFrameworkHostCrasher.sln` for x64. The C++ project builds the managed project
   through a project reference and copies its assembly and PDB into the SDK's `bin` folder next to
   the exe.

Requirements: the `v143` toolset and a .NET SDK to build the managed project. The CLR hosting
headers (`metahost.h`) and `mscoree.lib` come with Visual Studio's .NET Framework SDK.

## Run

```
MyDotNetFrameworkHostCrasher.exe /Throw [/Quiet]
```

Every mode starts in C#. Just before crashing, the managed code writes
`bugsplat-expected-stack.txt` next to the exe, and the host attaches it to the report: it lists
the managed frames, with metadata tokens and source lines, that the symbolicated stack should show.

| Mode | What happens | Captured by |
| --- | --- | --- |
| `/Throw` | Unhandled C# exception in the method the host called. | In-process filter |
| `/DeepThrow` | Unhandled C# exception four managed frames below the entry point. | In-process filter |
| `/Thread` | A managed worker thread throws. | In-process filter |
| `/AccessViolation` | C# writes to an unmapped address. | In-process filter |
| `/NativeCallback` | C# calls back into a native host function that faults (native → managed → native). | In-process filter |
| `/NoCrash` | Calls into C# and returns. | — |

The host calls `SetCrashType(8)`, the .NET crash type, so BugSplat resolves the managed frames in
the dump. Without it the report is processed as a native crash and the managed frames stay unnamed.
The managed frames are named with the runtime's DAC (`mscordacwks.dll`), which BugSplat does from
the plain `MiniDumpNormal` dump the host requests. Pass `/DumpType:<hex MINIDUMP_TYPE flags>` to
try other shapes; the value used is stamped on the report as the `dumpType` attribute. Shapes that
include `MiniDumpWithPrivateReadWriteMemory` or `MiniDumpWithFullMemory` are written only when the
application's full dump flag is enabled in the BugSplat database; otherwise BugSplatMonitor falls
back to `MiniDumpNormal`.

### Crashes the in-process filter sees

An unhandled managed exception is a Win32 exception (`0xE0434352`) that the runtime lets reach the
process's unhandled exception filter, so BugSplat captures a minidump in-process. The .NET
Framework does not fail fast on access violations either, so `/AccessViolation` and
`/NativeCallback` arrive the same way, as an unhandled `AccessViolationException`. The crashing
thread of a `/DeepThrow` dump, before symbolication, shows the runtime's throw helper above the
managed frames and the reverse P/Invoke thunk beneath them:

```
KERNELBASE!RaiseException
clr!IL_Throw
<jitted>  Crashers.DeepD
<jitted>  Crashers.DeepC
<jitted>  Crashers.DeepB
<jitted>  Crashers.DeepA
<jitted>  Crashers.Dispatch
<jitted>  reverse P/Invoke IL stub for the Dispatch delegate
clr!UMThunkStub
MyDotNetFrameworkHostCrasher!wmain
```

### Stack overflow

The .NET Framework can't recover from a stack overflow. It ends the process without running any
exception handler, so BugSplat's in-process filter never sees it, and reports it through Windows
Error Reporting. BugSplat captures it there when `BugSplatWer.dll` is registered: WER gives a crash
to the first runtime-exception helper that claims it, and `BugSplat.dll` keeps the runtime's own
helper (`mscordacwks.dll`, which files `CLR20r3` reports) behind `BugSplatWer.dll`. The sample
doesn't have a stack overflow mode.

## How the hosting works

`LoadManagedEntryPoint` in `MyDotNetFrameworkHostCrasher.cpp` starts the runtime through the CLR
hosting interfaces: `CLRCreateInstance` → `ICLRMetaHost::GetRuntime("v4.0.30319")` →
`ICLRRuntimeHost::Start`. Every .NET Framework 4.x release shares the v4.0.30319 runtime, so the
`net472` assembly runs on whichever 4.x release is installed.

`ExecuteInDefaultAppDomain` can only call `int Method(string)`, and it reports an exception that
escapes the call as an HRESULT instead of crashing. So the host calls it once, for
`Crashers.Install`, passing the address of a pointer-sized slot; `Install` writes a function
pointer to `Crashers.Dispatch` into it, and the host calls `Dispatch` through that pointer. An
exception escaping that reverse P/Invoke is unhandled and reaches BugSplat. The BugSplat global
object is constructed before any of that, so its exception filter is in place when the runtime
starts.
