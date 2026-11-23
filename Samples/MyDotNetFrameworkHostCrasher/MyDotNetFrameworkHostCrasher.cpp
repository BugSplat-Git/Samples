//
//        This sample hosts the .NET Framework 4 runtime from a native C++ program, calls into C# code, and crashes
//        there.  BugSplat is initialized natively, before the runtime loads, so the report's minidump has managed
//        frames on top of native ones: the mixed-mode call stack that BugSplat presents as a single trace.
//
//        To build this sample:
//        1. Edit MyDotNetFrameworkHostCrasher.h and provide your own value for BUGSPLAT_DATABASE.  (The database can be
//           managed by logging into the BugSplat web application.)
//        2. Create a Client ID and Client Secret pair for your BugSplat database at https://app.bugsplat.com/v2/database/integrations#oauth
//        3. Create a file MyDotNetFrameworkHostCrasher\Scripts\env.ps1 and populate it with the following (being sure to
//           substitute your {{id}} and {{secret}} values from the previous step):
//                                             $BUGSPLAT_CLIENT_ID = "{{id}}"
//                                             $BUGSPLAT_CLIENT_SECRET = "{{secret}}"
//        4. Build MyDotNetFrameworkHostCrasher.sln for x64.  The C++ project builds the managed half of the sample,
//           ..\MyDotNetFrameworkHostCrasherManaged, and copies it next to the exe.
//
//        In order to assure that crashes sent to the BugSplat website yield stack traces with file/line # information,
//        a Visual Studio post build event sends the exe, the managed assembly and their .pdb files to BugSplat using
//        the symbol-upload-windows utility.  If you do not want to send these files, simply disable the post build event.
//
//        More information is available online at https://www.bugsplat.com

#include "MyDotNetFrameworkHostCrasher.h"

#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <string>

#include "BugSplat.h"

// CLR hosting interfaces, from the .NET Framework SDK that installs with Visual Studio
#include <metahost.h>
#pragma comment(lib, "mscoree.lib")

// MyDotNetFrameworkHostCrasherManaged.Crashers.Dispatch, reached through the function pointer Crashers.Install hands out
typedef int (__cdecl* ManagedRunFn)(const wchar_t* mode, void (__cdecl* nativeFault)());

std::wstring GetExeDirectory();
ManagedRunFn LoadManagedEntryPoint(const std::wstring& exeDir);
void __cdecl NativeFault();

BugSplat g_BugSplat(BUGSPLAT_DATABASE, APPLICATION_NAME, APPLICATION_VERSION);

int wmain(int argc, wchar_t** argv)
{
	if (argc == 1) {
		wprintf(L"\nUsage: MyDotNetFrameworkHostCrasher.exe {CrashOption} [/Quiet (no crash dialog)]\n");

		wprintf(L"\nEvery crash starts in C# code called from this native program.\n");
		wprintf(L"\nCrashOption may be one of the following: \n\n");
		wprintf(L"\t/Throw - Throws an unhandled C# exception\n");
		wprintf(L"\t/DeepThrow - Throws an unhandled C# exception four managed frames deep\n");
		wprintf(L"\t/Thread - A managed worker thread throws an unhandled exception\n");
		wprintf(L"\t/AccessViolation - Writes to an unmapped address from C#\n");
		wprintf(L"\t/NativeCallback - C# calls back into a native function that faults (native -> managed -> native)\n");
		wprintf(L"\t/NoCrash - Calls into C# and returns normally\n");

		wprintf(L"\nAdd /DumpType:{hex MINIDUMP_TYPE flags} to override the default MiniDumpNormal, e.g. /DumpType:41 for\n");
		wprintf(L"MiniDumpNormal | MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory.\n");

		wprintf(L"\nExample: MyDotNetFrameworkHostCrasher.exe /Throw /Quiet\n\n");

		return 0;
	}

	if (IsDebuggerPresent())
	{
		wprintf(L"Run this application without the debugger attached to enable BugSplat exception handling.\n");
		DebugBreak();
		exit(0);
	}

	const wchar_t* mode = nullptr;
	bool quietMode = false;
	// MiniDumpNormal carries the thread stacks, which is all BugSplat needs for the mixed-mode call stack
	MINIDUMP_TYPE dumpType = MiniDumpNormal;
	for (int i = 1; i < argc; i++) {
		if (!_wcsicmp(argv[i], L"/Quiet")) {
			// Don't let the BugSplat dialog appear
			quietMode = true;
			g_BugSplat.SetQuietMode(true);
		}
		else if (!_wcsnicmp(argv[i], L"/DumpType:", 10)) {
			dumpType = static_cast<MINIDUMP_TYPE>(wcstoul(argv[i] + 10, nullptr, 16));
		}
		else if (argv[i][0] == L'/' || argv[i][0] == L'-') {
			mode = argv[i] + 1;
		}
	}

	if (!mode) {
		wprintf(L"No crash option given. Run without arguments for usage.\n");
		return 1;
	}

	if (!quietMode)
	{
		wprintf(L"BugSplat MyDotNetFrameworkHostCrasher Sample Application. Press any key to continue\n");
		wchar_t ch = _getwch();
	}

	g_BugSplat.PostAllCrashesAsync();

	// The following calls add support for collecting crashes for abort(), out of memory,
	// pure virtual function calls, and for invalid parameters for OS functions.
	SetGlobalCRTExceptionBehavior();
	SetPerThreadCRTExceptionBehavior();

	// Crash type 8 (.NET) tells BugSplat to resolve the managed frames in the dump
	g_BugSplat.SetCrashType(8);

	g_BugSplat.SetMiniDumpType(dumpType);
	wchar_t dumpTypeText[16];
	swprintf_s(dumpTypeText, L"0x%X", static_cast<unsigned>(dumpType));
	wprintf(L"Minidump type %s\n", dumpTypeText);

	// Set optional default values for user, email, and user description of the crash.
	g_BugSplat.SetUser(L"Fred");
	g_BugSplat.SetEmail(L"fred@bugsplat.com");
	std::wstring description = std::wstring(L"Testing ") + mode + L" crash";
	g_BugSplat.SetUserDescription(description.c_str());

	// Set optional custom crash attributes
	g_BugSplat.SetAttribute(L"host", L"native C++ via CLR hosting API");
	g_BugSplat.SetAttribute(L"dumpType", dumpTypeText);

	// The managed code writes the stack it expects the crash to have just before crashing.
	// Registering the path now attaches that file to the report.
	std::wstring exeDir = GetExeDirectory();
	std::wstring expectedStack = exeDir + L"bugsplat-expected-stack.txt";
	g_BugSplat.AddAttachment(expectedStack.c_str());

	ManagedRunFn run = LoadManagedEntryPoint(exeDir);
	if (!run) {
		return 1;
	}

	wprintf(L"Calling into managed code: /%s\n", mode);
	fflush(stdout); // managed Console output shares this handle

	int result = run(mode, NativeFault);

	wprintf(L"Managed code returned %d\n", result);
	return result;
}

// Starts the .NET Framework 4 runtime in this process and returns Crashers.Dispatch, or nullptr
// after printing which step failed.
ManagedRunFn LoadManagedEntryPoint(const std::wstring& exeDir)
{
	ICLRMetaHost* metaHost = nullptr;
	HRESULT hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_PPV_ARGS(&metaHost));
	if (FAILED(hr)) {
		wprintf(L"CLRCreateInstance failed (0x%08X). Is the .NET Framework 4 installed?\n", hr);
		return nullptr;
	}

	// Every 4.x release shares the v4.0.30319 runtime version
	ICLRRuntimeInfo* runtimeInfo = nullptr;
	hr = metaHost->GetRuntime(L"v4.0.30319", IID_PPV_ARGS(&runtimeInfo));
	metaHost->Release();
	if (FAILED(hr)) {
		wprintf(L"ICLRMetaHost::GetRuntime failed (0x%08X)\n", hr);
		return nullptr;
	}

	BOOL loadable = FALSE;
	hr = runtimeInfo->IsLoadable(&loadable);
	if (FAILED(hr) || !loadable) {
		wprintf(L"The v4.0.30319 runtime cannot be loaded into this process (0x%08X)\n", hr);
		runtimeInfo->Release();
		return nullptr;
	}

	// The host interface stays alive for the life of the process
	ICLRRuntimeHost* host = nullptr;
	hr = runtimeInfo->GetInterface(CLSID_CLRRuntimeHost, IID_PPV_ARGS(&host));
	runtimeInfo->Release();
	if (FAILED(hr)) {
		wprintf(L"ICLRRuntimeInfo::GetInterface failed (0x%08X)\n", hr);
		return nullptr;
	}

	hr = host->Start();
	if (FAILED(hr)) {
		wprintf(L"ICLRRuntimeHost::Start failed (0x%08X)\n", hr);
		return nullptr;
	}

	// ExecuteInDefaultAppDomain can only call "int Method(string)" and reports an exception that
	// escapes the call as an HRESULT, so it is used once, to fetch a function pointer to Dispatch.
	// Crashers.Install writes the pointer into the slot whose address it receives.
	std::wstring assemblyPath = exeDir + MANAGED_ASSEMBLY L".dll";
	ManagedRunFn run = nullptr;
	wchar_t slotAddress[32];
	swprintf_s(slotAddress, L"%llX", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(&run)));
	DWORD returnValue = 0;
	hr = host->ExecuteInDefaultAppDomain(assemblyPath.c_str(),
		MANAGED_ASSEMBLY L".Crashers", L"Install", slotAddress, &returnValue);
	if (FAILED(hr) || !run) {
		wprintf(L"ExecuteInDefaultAppDomain failed (0x%08X) for %s\n", hr, assemblyPath.c_str());
		return nullptr;
	}

	wprintf(L"Loaded %s\n", assemblyPath.c_str());
	return run;
}

// Called from C# for /NativeCallback, so the fault sits in native code above managed frames.
__declspec(noinline) void __cdecl NativeFault()
{
	wprintf(L"NativeFault: writing to an unmapped address\n");
	fflush(stdout);
	*reinterpret_cast<volatile int*>(0xDEADBEEFDEADBEEFull) = 42;
}

// Directory of the running exe, with a trailing backslash
std::wstring GetExeDirectory()
{
	wchar_t path[MAX_PATH];
	DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
	std::wstring dir(path, length);
	return dir.substr(0, dir.find_last_of(L'\\') + 1);
}
