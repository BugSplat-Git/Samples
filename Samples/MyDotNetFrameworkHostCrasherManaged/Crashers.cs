using System;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading;

namespace MyDotNetFrameworkHostCrasherManaged;

// The managed half of MyDotNetFrameworkHostCrasher. The native host starts the .NET Framework
// runtime through the CLR hosting interfaces and calls Dispatch through a function pointer, so
// every crash below starts in C# with the host's native frames (wmain and the runtime) beneath it. That mixed-mode call stack is
// what the samples exist to put in front of BugSplat.
public static unsafe class Crashers
{
    // A non-canonical x64 address is unmappable in every process, so a write to it faults
    // deterministically. A null pointer would not do: the CLR turns null-page faults into a
    // catchable NullReferenceException.
    private const ulong UnmappedAddress = 0xDEADBEEFDEADBEEF;

    // The expected-stack file goes next to this assembly, where the native host registered it as
    // an attachment. A hosted runtime has no entry assembly, so AppContext.BaseDirectory can't be used.
    private static readonly string OutputDirectory =
        Path.GetDirectoryName(typeof(Crashers).Assembly.Location) ?? Environment.CurrentDirectory;

    // The host function that faults, used by the NativeCallback mode.
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void NativeFaultDelegate();

    // ExecuteInDefaultAppDomain can only call "int Method(string)", and it turns an exception
    // escaping that call into an HRESULT instead of crashing. So the host calls Install with the
    // address of a pointer-sized slot, gets a function pointer to Dispatch written into it, and then
    // calls Dispatch through it. An exception escaping that call is unhandled.
    [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    private delegate int RunDelegate(string mode, IntPtr nativeFault);

    // Keeps the delegate behind the function pointer alive for the life of the process.
    private static readonly RunDelegate s_run = Dispatch;

    public static int Install(string slotAddressHex)
    {
        var slot = new IntPtr(Convert.ToInt64(slotAddressHex, 16));
        Marshal.WriteIntPtr(slot, Marshal.GetFunctionPointerForDelegate(s_run));
        return 0;
    }

    // mode is the command line option without its slash and nativeFault is a host function that
    // crashes, used by the NativeCallback mode. Returns 0 when the mode finished without crashing
    // and 1 for an unknown mode.
    private static int Dispatch(string scenario, IntPtr nativeFault)
    {
        Console.WriteLine($"[managed] {RuntimeInformation.FrameworkDescription} ({RuntimeInformation.ProcessArchitecture}) running {scenario}");

        switch (scenario.ToLowerInvariant())
        {
            case "nocrash":
                CrashExpectation.Record(scenario, OutputDirectory);
                Console.WriteLine("[managed] returning to native code without crashing");
                return 0;

            case "throw":
                Throw(scenario);
                return 0;

            case "deepthrow":
                DeepA(scenario);
                return 0;

            case "accessviolation":
                AccessViolation(scenario);
                return 0;

            case "nativecallback":
                NativeCallback(scenario, nativeFault);
                return 0;

            case "thread":
                ThrowOnThread(scenario);
                return 0;

            default:
                Console.WriteLine($"[managed] unknown crash option '{scenario}'");
                return 1;
        }
    }

    private static void Throw(string scenario)
    {
        CrashExpectation.Record(scenario, OutputDirectory);
        Console.WriteLine("[managed] throwing an unhandled exception");
        throw new InvalidOperationException("MyDotNetHostCrasher managed crash");
    }

    // A chain of distinct methods, kept out of line, so the walked stack has several named
    // managed frames between the throw and the native host.
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static void DeepA(string scenario) => DeepB(scenario);

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static void DeepB(string scenario) => DeepC(scenario);

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static void DeepC(string scenario) => DeepD(scenario);

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static void DeepD(string scenario)
    {
        CrashExpectation.Record(scenario, OutputDirectory);
        Console.WriteLine("[managed] throwing an unhandled exception four frames below Dispatch");
        throw new InvalidOperationException("MyDotNetHostCrasher deep managed crash");
    }

    private static void AccessViolation(string scenario)
    {
        CrashExpectation.Record(scenario, OutputDirectory);
        Console.WriteLine("[managed] writing to an unmapped address");
        *(int*)UnmappedAddress = 42;
    }

    // Native -> managed -> native: the fault is in the host's own code, called from here.
    private static void NativeCallback(string scenario, IntPtr nativeFault)
    {
        CrashExpectation.Record(scenario, OutputDirectory);
        Console.WriteLine("[managed] calling back into native code that faults");
        Marshal.GetDelegateForFunctionPointer<NativeFaultDelegate>(nativeFault)();
    }

    private static void ThrowOnThread(string scenario)
    {
        Console.WriteLine("[managed] throwing on a managed worker thread");
        var thread = new Thread(() =>
        {
            CrashExpectation.Record(scenario, OutputDirectory);
            throw new InvalidOperationException("MyDotNetHostCrasher worker thread crash");
        });
        thread.Start();
        thread.Join();
    }
}
