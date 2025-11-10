//
//        This sample project illustrates how to capture crashes (unhandled exceptions) in native Windows applications using BugSplat.
//
//		  To build this sample:
//		  1. Edit MyConsoleCrasher.h and provide your own value for BUGSPLAT_DATABASE.  (The database can be managed by logging into
//           the BugSplat web application.)
//	      2. Create a Client ID and Client Secret pair for your BugSplat database at https://app.bugsplat.com/v2/database/integrations#oauth
//        3. Create a file MyConsoleCrasher\Scripts\env.ps1 and populate it with the following (being sure to subsitute your {{id}} and {{secret}} 
//           values from the previous step):
//                                             $BUGSPLAT_CLIENT_ID = "{{id}}"
//                                             $BUGSPLAT_CLIENT_SECRET = "{{secret}}"
//		  4. Build the project
//    
//        In order to assure that crashes sent to the BugSplat website yield exception stack traces with file/line # information, 
//        a Visual Studio post build event is configured to send the resulting .exe and .pdb files to BugSplat using the SendPdbs utility. 
//        If you do not care about file/line # info or for any reason you do not want to send these files, 
//        simply disable the post build event.
//
//        More information is available online at https://www.bugsplat.com

#pragma optimize( "", off) // prevent optimizer from interfering with our crash-producing code

#include "stdafx.h"

#ifdef ASAN
// Enabling Asan error reports requires changes to the build settings, including the compiler option /fsanitize=address
#include "sanitizer/asan_interface.h"
#endif

#include "MyConsoleCrasher.h"
#include "BugSplat.h"
#include <mutex>

// An example ASSERT macro.  Adds an additional frame to the top of the stack allowing you to easily distinguish between asserts 
// in the same function, without relying on line numbers. 
//
#define NOINLINE __declspec(noinline)

#define ASSERT(expr, name) \
    do { \
        if (!(expr)) { \
            struct name##_assert { \
                NOINLINE static void execute(const char* expression, const char* message, const char* file, int line, const char* func) { \
                    static volatile int name_hash = (#name)[0]; /* Prevents code folding optimizations in RELEASE mode */\
                    wprintf(L"ASSERT %hs failed: %hs at %hs:%d in %hs\n", expression, message, file, line, func); \
                    volatile int* z = 0; *z = 13; \
                } \
            }; \
            name##_assert::execute(#expr, #name, __FILE__, __LINE__, __func__); \
        } \
        else { \
            wprintf(L"ASSERT %hs passed: %hs at %hs:%d in %hs\n", #expr, #name, __FILE__, __LINE__, __func__); \
        } \
    } while(0)

bool AddAttachments();
void AssertTests();
void MemoryException();
void StackOverflow(void* p);
void StackOverrun();
void DivideByZero();
void ExhaustMemory();
void ThrowByUser();
void ThreadException(int nthreads);
void CallAbort();
void InvalidParameters();
void OutOfBoundsVectorCrash();
void VirtualFunctionCallCrash();
void CustomSEHException();
void StdException();
void HeapCorruption();
void PrivilegedInstruction();
void DoubleDelete();
void UseAfterFree();
void InvalidFunctionPointer();
void FastFail();
void CreateXmlReport();
bool AddAttachments();
extern "C" LONG WINAPI GlobalExceptionFilter(LPEXCEPTION_POINTERS const exceptionPointers);

BugSplat g_BugSplat(BUGSPLAT_DATABASE, APPLICATION_NAME, APPLICATION_VERSION);

int wmain(int argc, wchar_t **argv)
{
	if (argc == 1) {
		wprintf(L"\nUsage: MyConsoleCrasher.exe {CrashOption} [/Quiet (no crash dialog)]\n");

		wprintf(L"\nCrashOption may be one of the following: \n\n");
		wprintf(L"\t/MemoryException - Causes an access violation by dereferencing a null pointer\n");
		wprintf(L"\t/StackOverflow - Causes a stack overflow exception\n");
		wprintf(L"\t/PrivilegedInstruction - Causes a privileged instruction exception\n");
		wprintf(L"\t/UseAfterFree - Causes a use after free heap corruption exception\n");
		wprintf(L"\t/InvalidFunctionPointer - Causes an access violation by calling through an invalid function pointer\n");

		wprintf(L"\t/DivByZero - Causes a divide by zero exception\n");
		wprintf(L"\t/OutOfMemory - Causes an out of memory exception\n");
		wprintf(L"\t/Throw - Causes a C++ exception to be thrown\n");
		wprintf(L"\t/Thread - Creates a thread that crashes\n");
		wprintf(L"\t/MultipleThreads - Creates multiple threads that crash\n");
		wprintf(L"\t/Abort - Calls abort() to cause abnormal program termination\n");
		wprintf(L"\t/Asan - Causes a heap corruption that is detected by Asan (requires Asan to be enabled in build settings)\n");
		wprintf(L"\t/VectorOutOfBounds - Causes an out of bounds access on a std::vector\n");
		wprintf(L"\t/InvalidParameters - Causes an invalid parameter exception\n");
		wprintf(L"\t/PureVirtual - Causes a pure virtual function call exception\n");
		wprintf(L"\t/SEH - Causes and recovers from a custom SEH exception\n");
		wprintf(L"\t/CreateXmlReport - Creates and sends an xml report to BugSplat\n");

		wprintf(L"\nThe following crash types require BugSplat WER integration to be enabled via registry settings:\n\n");
		wprintf(L"\t/StackOverrun - Causes a stack overrun exception\n");
		wprintf(L"\t/FastFail - Causes a fast fail exception\n");
		wprintf(L"\t/DoubleDelete - Causes a double delete heap corruption exception\n");

		wprintf(L"\nExample: MyConsoleCrasher.exe /MemoryException /Quiet\n\n");

		return 0;
	}

	if (IsDebuggerPresent())
	{
		wprintf(L"Run this application without the debugger attached to enable BugSplat exception handling.\n");
		DebugBreak();
		exit(0);
	}

	if (!g_BugSplat.IsWerEnabled())
	{
		wprintf(L"\n!!!Warning: BugSplat WER is not configured.  Some crashes will not be handled by BugSplat!!!\n\n");
	}

	wprintf(L"BugSplat MyConsoleCrasher Sample Application. Press any key to continue\n");
	wchar_t ch = _getwch();

	// The following calls add support for collecting crashes for abort(), vectored exceptions, out of memory,
	// pure virtual function calls, and for invalid parameters for OS functions.
	// These calls should be used for each module that links with a separate copy of the CRT.
	SetGlobalCRTExceptionBehavior();
	SetPerThreadCRTExceptionBehavior();  // This call may be needed in each thread of your app

	// If using std::exception, add this call
	std::set_terminate(terminator);

	// Set optional default values for user, email, and user description of the crash.
	g_BugSplat.SetUser(L"Fred");
	g_BugSplat.SetEmail(L"fred@bugsplat.com");
	g_BugSplat.SetUserDescription(L"This is the default user crash description.");

	// Set optional notes field
	g_BugSplat.SetNotes(L"Additional 'notes' data supplied through API");

	// Set optional custom crash attributes
	g_BugSplat.SetAttribute(L"GPU", L"GeForce '{}!@#4\t5(\rじみー\n で\"す。)678()<>{},./?[] RTX 4060 Ti");
	g_BugSplat.SetAttribute(L"Region", L"Europe");

	// Add attachments to the crash report
	AddAttachments();

	for (int i = 1; i < argc; i++) {
		if (!_wcsicmp(argv[i], L"/Quiet")) {
			// Don't let the BugSplat dialog appear
			g_BugSplat.SetQuietMode(true);
		}
	}

	// Force a crash, in a variety of ways
	for (int i = 1; i < argc; i++) {

		if (!_wcsicmp(argv[i], L"/AssertTests")) {
			AssertTests();
		}

		if (!_wcsicmp(argv[i], L"/MemoryException")) {
			MemoryException();
		}

		else if (!_wcsicmp(argv[i], L"/StackOverflow")) {
			StackOverflow(NULL);
		}

		else if (!_wcsicmp(argv[i], L"/StackOverrun")) {
			StackOverrun();
		}

		else if (!_wcsicmp(argv[i], L"/PrivilegedInstruction")) {
			PrivilegedInstruction();
		}

		else if (!_wcsicmp(argv[i], L"/DoubleDelete")) {
			DoubleDelete();
		}

		else if (!_wcsicmp(argv[i], L"/UseAfterFree")) {
			UseAfterFree();
		}

		else if (!_wcsicmp(argv[i], L"/InvalidFunctionPointer")) {
			InvalidFunctionPointer();
		}

		else if (!_wcsicmp(argv[i], L"/FastFail")) {
			FastFail();
		}

		else if (!_wcsicmp(argv[i], L"/DivByZero")) {
			DivideByZero();
		}

		else if (!_wcsicmp(argv[i], L"/OutOfMemory")) {
			ExhaustMemory();
		}

		else if (!_wcsicmp(argv[i], L"/Throw")) {
			ThrowByUser();
		}

		else if (!_wcsicmp(argv[i], L"/Thread")) {
			ThreadException(1);
		}

		else if (!_wcsicmp(argv[i], L"/MultipleThreads")) {
			ThreadException(10);
		}

		else if (!_wcsicmp(argv[i], L"/Abort")) {
			CallAbort();
		}

		else if (!_wcsicmp(argv[i], L"/Asan")) {
			HeapCorruption();	// / Generally this error goes undetected if Asan is not enabled
		}

		else if (!_wcsicmp(argv[i], L"/VectorOutOfBounds")) {
			OutOfBoundsVectorCrash();
		}

		else if (!_wcsicmp(argv[i], L"/InvalidParameters")) {
			InvalidParameters();
		}

		else if (!_wcsicmp(argv[i], L"/PureVirtual")) {
			VirtualFunctionCallCrash();
		}

		else if (!_wcsicmp(argv[i], L"/StdException")) {
			StdException();
		}

		else if (!_wcsicmp(argv[i], L"/SEH")) {

			g_BugSplat.SetUserDescription(_T("BugSplat ALERT - execution continues!"));

			for (int i = 0; i < 3; i++) {
				CustomSEHException();
				wprintf(L"Recovered from SEH exception %d\n", i + 1);
				Sleep(1000);
			}
			wprintf(L"Application normal exit.\n");
			return 0;
		}

		else if (!_wcsicmp(argv[i], L"/CreateXmlReport")) {

			for (int i = 0; i < 3; i++)
			{
				CreateXmlReport();
				wprintf(L"Sent report %d\n", i + 1);
				Sleep(1000);
			}
			wprintf(L"Application normal exit.\n");
			return 0;
		}
	}

	return 0;
}

void AssertTests()
{
	// Test asserts
	ASSERT(1 == 1, Assert1);  // Assert true, no failure but you might log this
	ASSERT(1 == 0, Assert2);  // Assert false, add to log and generate a BugSplat report 
}

void MemoryException()
{
	// Dereferencing a null pointer results in a memory exception
	wprintf(L"MemoryException!\n");
	*(volatile int*)0 = 0;
}

void DivideByZero()
{
	wprintf(L"DivideByZero!\n");
	volatile int x, y, z;
	x = 1;
	y = 0;
	z = x / y;
}

void StackOverflow(void *p)
{
	// Calling a recursive function with no exit results in a stack overflow
	wprintf(L"StackOverflow!\n");
	volatile char q[10000];
	while (true) {
		StackOverflow((void *)q);
	}
}

void PrivilegedInstruction()
{
	// Try to execute a privileged instruction from user mode
	wprintf(L"PrivilegedInstruction!\n");
#if _M_AMD64
	__halt();
#else
	__hlt(0);
#endif
}

void UseAfterFree()
{
	// Requires compiler flag /sdl to be enabled
	wprintf(L"UseAfterFree!\n");
	char* buffer = new char[1024];
	delete[] buffer;
	memset((void*)buffer, 0xCC, 1024); // Use after free
}

void InvalidFunctionPointer()
{
	// Crash when trying to execute invalid function
	wprintf(L"!InvalidFunctionPointer\n");
	void (*invalidFunc)() = (void(*)())0xDEADBEEF;
	invalidFunc();
}

// ====  WER Crash Examples  ====
// The following three functions require BugSplat WER integration to be enabled to get crash dumps
// automatically uploaded to BugSplat.  These types of crashes are not catchable in user code.
// 
// If the WER integration is not enabled, the program will simply terminate.
// In this case, WER may create a crash dump that can be sent to BugSplat manually.
// Look for these dumps in C:\Users\{user}\AppData\Local\CrashDumps
//
void StackOverrun()
{
	// Requires BugSplat WER integration to be enabled
	wprintf(L"StackOverrun!\n");
	char buffer[10];
	volatile char* p = buffer;
	p--;
	memset((void*)p, 'A', 2000);  // Overwrite the 10-byte buffer
}

void DoubleDelete()
{
	// This error will always be caught for Debug builds.  In Release builds,
	// BugSplat WER integration must be enabled.
	wprintf(L"DoubleDelete!\n");
	char* p1 = new char[100];
	delete[] p1;
	delete[] p1; // Double delete
}

void FastFail()
{
	// Requires BugSplat WER integration to be enabled
	wprintf(L"FastFail!\n");
	__fastfail(1);
}

// ====  End of WER Crash Examples  ====

void ExhaustMemory()
{
	wprintf(L"ExhaustMemory!\n");

	// Loop until memory exhausted
	while (true)
	{
		char* a = new char[1024 * 1024];
		a[0] = 'X';
	}
}

void ThrowByUser()
{
	wprintf(L"Throw user generated exception!\n");
	throw("User generated exception!");
}


DWORD WINAPI MyThreadCrasher( LPVOID )
{
	int msec = 2000;
	Sleep(msec);

	wprintf(L"MyThreadCrasher creating memory exception after %d milliseconds!\n", msec);
	MemoryException();
	return 0;
}

void ThreadException( int max_threads)
{
	DWORD   dwThreadIdArray[100];
	HANDLE  hThreadArray[100];
	if (max_threads > 100) max_threads = 100;

	// Create worker threads.
	for (int i = 0; i<max_threads; i++)
	{
		// Create the thread to begin execution on its own.
		hThreadArray[i] = CreateThread(
			NULL,                   // default security attributes
			0,                      // use default stack size  
			MyThreadCrasher,        // thread function name
			NULL,					// argument to thread function 
			0,                      // use default creation flags 
			&dwThreadIdArray[i]);   // returns the thread identifier 

		if (hThreadArray[i] == NULL)
		{
			wprintf(L"CreateThread failed");
			ExitProcess(3);
		}
	}

	// Wait until all threads have terminated.
	WaitForMultipleObjects(max_threads, hThreadArray, TRUE, INFINITE);
}

void CallAbort()
{
	wprintf(L"abort()!\n");
	abort();
}

void OutOfBoundsVectorCrash()
{
	wprintf(L"std::vector out of bounds!\n");
	std::vector<int> v;
	v[0] = 5;
}

void InvalidParameters()
{
	wprintf(L"Invalid parameters!\n");
	char *fmt = NULL;
	printf(fmt);
}

void VirtualFunctionCallCrash()
{
	struct Base {
		Base()
		{
			wprintf(L"Pure Virtual Function Call crash!");
			BaseFunc();
		}

		virtual void DerivedFunc() = 0;

		void BaseFunc()
		{
			DerivedFunc();
		}
	};

	struct Derived : public Base
	{
		void DerivedFunc() {}
	};

	Base* instance = new Derived;
	instance->DerivedFunc();
}

void StdException()
{
	throw std::out_of_range("std::out_of_range example!");
}

DWORD SEHFilterFunction(EXCEPTION_POINTERS* exp)
{
	g_BugSplat.SetQuietMode(true); // Allows BugSplat to continue monitoring after the exception
	g_BugSplat.GenerateDump(exp, MiniDumpNormal);
	g_BugSplat.PostCrash();
	return EXCEPTION_EXECUTE_HANDLER;
}

void CustomSEHException()
{
	__try
	{
		// Use to create a BugSplat report without exiting.
		RaiseException(
			0x123,         // exception code 
			0,             // continuable exception 
			0, NULL);      // no arguments ;
	}
	__except (SEHFilterFunction(GetExceptionInformation()))
	{
		return;
	}
}


// Address sanitizer test
void asanCallback(const char* message)
{
	g_BugSplat.CreateAsanReport(message);
}


// Generally this error goes undetected if Asan is not enabled
void HeapCorruption()
{
	void* pointerArray[20];
	struct simple_struct {
		double b;
		double c;
		char d;
	};

#ifdef ASAN
	__asan_set_error_report_callback(asanCallback);
#endif

	{
		auto a = new simple_struct;
		auto b = new simple_struct;

		pointerArray[0] = a;
		pointerArray[1] = b;
	}

	// Function attempts to use pointer from array.
	// Encounters an error and deletes the pointer without removing it from array
	{
		auto item = reinterpret_cast<simple_struct*>(pointerArray[1]);
		delete item;
	}

	// Program continues, address is reused
	constexpr auto numInts = 40;
	auto intArray = reinterpret_cast<int*>(pointerArray[1]);
	for (int i = 0; i < numInts; i++) {
		intArray[i] = 10;
	}

	// Function called again, encounters the same error and tries to delete the same memory again
	{
		auto item = reinterpret_cast<simple_struct*>(pointerArray[1]);

		__try {
			delete item;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			wprintf(L"exception handling test successful.\n");

		}
	}
}

void CreateXmlReport()
{
	const __wchar_t *xml = L"<report><process>"
		"<exception>"
		"<code>FATAL ERROR</code>"
		"<explanation>This is an error code explanation</explanation>"
		"<func><![CDATA[MyConsoleCrasher!MemoryException]]></func>"
		"<file>/www/bugsplatAutomation/MyConsoleCrasher/MyConsoleCrasher.cpp</file>"
		"<line>143</line>"
		"<registers>"
		"<cs>0023</cs>"
		"<ds>002b</ds>"
		"<eax>00000011</eax>"
		"<ebp>00affb58</ebp>"
		"<ebx>00858000</ebx>"
		"<ecx>43bf1e0e</ecx>"
		"<edi>00affb58</edi>"
		"<edx>014480b4</edx>"
		"<efl>00010202</efl>"
		"</registers>"
		"</exception>"
		"<modules numloaded=\"2\">"
		"<module>"
		"<name>MyConsoleCrasher</name>"
		"<order>1</order>"
		"<address>01320000-01457000</address>"
		"<path>C:/www/BugsplatAutomation/BugsplatAutomation/bin/x64/Release/temp/BugSplat/bin/MyConsoleCrasher.exe</path>"
		"<symbolsloaded>deferred</symbolsloaded>"
		"<fileversion/>"
		"<productversion/>"
		"<checksum>00000000</checksum>"
		"<timedatestamp>SatJun1501:18:092019</timedatestamp>"
		"</module>"
		"<module>"
		"<name>BugSplatRc</name>"
		"<order>2</order>"
		"<address>01320000-01457000</address>"
		"<path>C:/www/BugsplatAutomation/BugsplatAutomation/bin/x64/Release/BugSplatRc.dll</path>"
		"<symbolsloaded>deferred</symbolsloaded>"
		"<fileversion/>"
		"<productversion/>"
		"<checksum>00000000</checksum>"
		"<timedatestamp>SatJun1501:18:092019</timedatestamp>"
		"</module>"
		"</modules>"
		"<threads count=\"2\">"
		"<thread id=\"0\" current=\"yes\" event=\"yes\" framecount=\"3\">"
		"<frame>"
		"<symbol><![CDATA[MyConsoleCrasher!MemoryException]]></symbol>"
		"<file>/www/bugsplatAutomation/MyConsoleCrasher/MyConsoleCrasher.cpp</file>"
		"<line>143</line>"
		"<offset>0x35</offset>"
		"</frame>"
		"<frame>"
		"<symbol><![CDATA[MyConsoleCrasher!wmain]]></symbol>"
		"<file>C:/www/BugsplatAutomation/BugsplatAutomation/BugSplat/samples/MyConsoleCrasher/MyConsoleCrasher.cpp</file>"
		"<line>83</line>"
		"<offset>0x239</offset>"
		"</frame>"
		"<frame>"
		"<symbol><![CDATA[MyConsoleCrasher!__scrt_wide_environment_policy::initialize_environment]]></symbol>"
		"<file>d:/agent/_work/4/s/src/vctools/crt/vcstartup/src/startup/exe_common.inl</file>"
		"<line>90</line>"
		"<offset>0x43</offset>"
		"</frame>"
		"</thread>"
		"<thread id=\"1\" current=\"no\" event=\"no\" framecount=\"3\">"
		"<frame>"
		"<symbol><![CDATA[my2ConsoleCrasher!MemoryException]]></symbol>"
		"<file>/www/bugsplatAutomation/MyConsoleCrasher/MyConsoleCrasher.cpp</file>"
		"<line>143</line>"
		"<offset>0x35</offset>"
		"</frame>"
		"<frame>"
		"<symbol><![CDATA[my2ConsoleCrasher!wmain]]></symbol>"
		"<file>C:/www/BugsplatAutomation/BugsplatAutomation/BugSplat/samples/MyConsoleCrasher/MyConsoleCrasher.cpp</file>"
		"<line>83</line>"
		"<offset>0x239</offset>"
		"</frame>"
		"<frame>"
		"<symbol><![CDATA[my2ConsoleCrasher!__scrt_wide_environment_policy::initialize_environment]]></symbol>"
		"<file>d:/agent/_work/4/s/src/vctools/crt/vcstartup/src/startup/exe_common.inl</file>"
		"<line>90</line>"
		"<offset>0x43</offset>"
		"</frame>"
		"</thread>"
		"</threads></process></report>";

	g_BugSplat.CreateXmlReport(xml);
}

bool AddAttachments()
{
	// Create some files in the %temp% directory and attach them
	wchar_t cmdString[2 * MAX_PATH];
	wchar_t filePath[MAX_PATH];
	wchar_t tempPath[MAX_PATH];
	GetTempPathW(MAX_PATH, tempPath);

	// Write some sample text files to attach
	wsprintf(filePath, L"%sfile1.txt", tempPath);
	wsprintf(cmdString, L"echo A sample log file > \"%s\"", filePath);
	_wsystem(cmdString);
	g_BugSplat.AddAttachment(filePath);

	GetTempPathW(MAX_PATH, tempPath);
	wsprintf(filePath, L"%sfile2.txt", tempPath);
	wsprintf(cmdString, L"echo Crash reporting is so clutch! > \"%s\"", filePath);
	_wsystem(cmdString);
	g_BugSplat.AddAttachment(filePath);

	return true;
}


// An example of provding your own unhandled exception filter.  The code below is very similar to the
// default BugSplat exception filter.  To use this, provide the function name as the last parameter 
// to the BugSplat constructor e.g.
// 	BugSplat g_BugSplat = BugSplat(BUGSPLAT_DATABASE, APPLICATION_NAME, APPLICATION_VERSION, GlobalExceptionFilter);
//
// Note, certain types of crashes, such as stack overrun, are not catchable in user code.  BugSplat
// will continue to catch these types of crashes if WER integration is enabled.
//
extern "C" LONG WINAPI GlobalExceptionFilter(LPEXCEPTION_POINTERS const exceptionPointers)
{
	// Required for handling out-of-memory errors
	g_BugSplat.FreeGuardMemory();

	// Tell the BugSplat crash handler to create a crash dump.
	g_BugSplat.GenerateDump(exceptionPointers, MINIDUMP_TYPE::MiniDumpNormal);

	// Upload the crash to BugSplat.
	g_BugSplat.PostCrash();

	// Exit the program.
	exit(123);
}
