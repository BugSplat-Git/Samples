#pragma optimize( "", off) // prevent optimizer from interfering with our crash-producing code

#include "pch.h"
#include "CrashFunctions.h"
#include "BugSplat.h"
#include "Main.h"
#include <vector>

#ifdef ASAN
#include "sanitizer/asan_interface.h"
#endif

extern BugSplat g_bugsplat;

namespace CrashExamples
{

void MemoryException()
{
    // Dereferencing a null pointer results in a memory exception
    OutputDebugStringW(L"MemoryException!\n");
    *(volatile int*)0 = 0;
}

void DivideByZero()
{
    OutputDebugStringW(L"DivideByZero!\n");
    volatile int x, y, z;
    x = 1;
    y = 0;
    z = x / y;
}

void StackOverflow(void* p)
{
    // Calling a recursive function with no exit results in a stack overflow
    OutputDebugStringW(L"StackOverflow!\n");
    p = (int *)p + 1;
    volatile char q[10000];
    while (true) {
        StackOverflow((void*)q);
    }
}

void PrivilegedInstruction()
{
    // Try to execute a privileged instruction from user mode
    OutputDebugStringW(L"PrivilegedInstruction!\n");
#if _M_AMD64
    __halt();
#else
    __hlt(0);
#endif
}

void UseAfterFree()
{
    // Requires compiler flag /sdl to be enabled
    OutputDebugStringW(L"UseAfterFree!\n");
    char* buffer = new char[1024];
    delete[] buffer;
    memset((void*)buffer, 0xCC, 1024); // Use after free
}

void InvalidFunctionPointer()
{
    // Crash when trying to execute invalid function
    OutputDebugStringW(L"InvalidFunctionPointer!\n");
    void (*invalidFunc)() = (void(*)())0xDEADBEEF;
    invalidFunc();
}

void StackOverrun()
{
    OutputDebugStringW(L"StackOverrun!\n");
    char buffer[10];
    volatile char* p = buffer;
    p--;
    memset((void*)p, 'A', 2000);  // Overwrite the 10-byte buffer
}

void DoubleDelete()
{
    // This error will always be caught for Debug builds
    OutputDebugStringW(L"DoubleDelete!\n");
    char* p1 = new char[100];
    delete[] p1;
    delete[] p1; // Double delete
}

void FastFail()
{
    OutputDebugStringW(L"FastFail!\n");
    __fastfail(1);
}

void ExhaustMemory()
{
    OutputDebugStringW(L"ExhaustMemory!\n");

    // Loop until memory exhausted
    while (true)
    {
        char* a = new char[1024 * 1024];
        a[0] = 'X';
    }
}

void ThrowByUser()
{
    OutputDebugStringW(L"Throw user generated exception!\n");
    throw("User generated exception!");
}

DWORD WINAPI MyThreadCrasher(LPVOID)
{
    int msec = 200;
    Sleep(msec);

    OutputDebugStringW(L"MyThreadCrasher creating memory exception after 200 milliseconds!\n");
    MemoryException();
    return 0;
}

void ThreadException(int max_threads)
{
    DWORD   dwThreadIdArray[100];
    HANDLE  hThreadArray[100];
    if (max_threads > 100) max_threads = 100;

    // Create worker threads.
    for (int i = 0; i < max_threads; i++)
    {
        hThreadArray[i] = CreateThread(
            NULL,                   // default security attributes
            0,                      // use default stack size  
            MyThreadCrasher,        // thread function name
            NULL,                   // argument to thread function 
            0,                      // use default creation flags 
            &dwThreadIdArray[i]);   // returns the thread identifier 

        if (hThreadArray[i] == NULL)
        {
            OutputDebugStringW(L"CreateThread failed\n");
            ExitProcess(3);
        }
    }

    // Wait until all threads have terminated.
    WaitForMultipleObjects(max_threads, hThreadArray, TRUE, INFINITE);
}

void CallAbort()
{
    OutputDebugStringW(L"abort()!\n");
    abort();
}

void OutOfBoundsVectorCrash()
{
    OutputDebugStringW(L"std::vector out of bounds!\n");
    std::vector<int> v;
    v[0] = 5;
}

void InvalidParameters()
{
    OutputDebugStringW(L"Invalid parameters!\n");
    char* fmt = NULL;
    printf(fmt);
}

void VirtualFunctionCallCrash()
{
    struct Base {
        Base()
        {
            OutputDebugStringW(L"Pure Virtual Function Call crash!");
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

DWORD SEHFilterFunction(EXCEPTION_POINTERS* exp)
{
    g_bugsplat.SetQuietMode(true); // Allows BugSplat to continue monitoring after the exception
    g_bugsplat.GenerateDump(exp, MiniDumpNormal);
    g_bugsplat.PostCrash();
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
            0, NULL);      // no arguments
    }
    __except (SEHFilterFunction(GetExceptionInformation()))
    {
        OutputDebugStringW(L"Recovered from SEH exception\n");
        return;
    }
}

#ifdef ASAN
void asanCallback(const char* message)
{
    if (g_bugsplat != nullptr)
    {
        g_bugsplat.CreateAsanReport(message);
    }
}
#endif

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
            OutputDebugStringW(L"Exception handling test successful.\n");
        }
    }
}

void CreateXmlReport()
{
    const __wchar_t* xml = L"<report><process>"
        "<exception>"
        "<code>FATAL ERROR</code>"
        "<explanation>This is an error code explanation</explanation>"
        "<func><![CDATA[myWinUI3Crasher!CreateXmlReport]]></func>"
        "<file>/www/bugsplatAutomation/myWinUI3Crasher/myWinUI3Crasher.cpp</file>"
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
        "<name>myWinUI3Crasher</name>"
        "<order>1</order>"
        "<address>01320000-01457000</address>"
        "<path>C:/www/BugsplatAutomation/BugsplatAutomation/bin/x64/Release/temp/BugSplat/bin/myWinUI3Crasher.exe</path>"
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
        "<symbol><![CDATA[myWinUI3Crasher!CreateXmlReport]]></symbol>"
        "<file>/www/bugsplatAutomation/myWinUI3Crasher/myWinUI3Crasher.cpp</file>"
        "<line>143</line>"
        "<offset>0x35</offset>"
        "</frame>"
        "<frame>"
        "<symbol><![CDATA[myWinUI3Crasher!wmain]]></symbol>"
        "<file>C:/www/BugsplatAutomation/BugsplatAutomation/BugSplat/samples/myWinUI3Crasher/myWinUI3Crasher.cpp</file>"
        "<line>83</line>"
        "<offset>0x239</offset>"
        "</frame>"
        "<frame>"
        "<symbol><![CDATA[myWinUI3Crasher!__scrt_wide_environment_policy::initialize_environment]]></symbol>"
        "<file>d:/agent/_work/4/s/src/vctools/crt/vcstartup/src/startup/exe_common.inl</file>"
        "<line>90</line>"
        "<offset>0x43</offset>"
        "</frame>"
        "</thread>"
        "<thread id=\"1\" current=\"no\" event=\"no\" framecount=\"3\">"
        "<frame>"
        "<symbol><![CDATA[my2ConsoleCrasher!CreateXmlReport]]></symbol>"
        "<file>/www/bugsplatAutomation/myWinUI3Crasher/myWinUI3Crasher.cpp</file>"
        "<line>143</line>"
        "<offset>0x35</offset>"
        "</frame>"
        "<frame>"
        "<symbol><![CDATA[my2ConsoleCrasher!wmain]]></symbol>"
        "<file>C:/www/BugsplatAutomation/BugsplatAutomation/BugSplat/samples/myWinUI3Crasher/myWinUI3Crasher.cpp</file>"
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

    g_bugsplat.CreateXmlReport(xml);
    OutputDebugStringW(L"Created XML report\n");
}

void ApplicationHang()
{
    // Infinite loop with sleep to simulate application hang
    // This will trigger BugSplat hang detection if enabled
    OutputDebugStringW(L"ApplicationHang - entering infinite loop!\n");
    while (true)
    {
        Sleep(1000); // Sleep for 1 second per iteration
    }
}

} // namespace CrashExamples
