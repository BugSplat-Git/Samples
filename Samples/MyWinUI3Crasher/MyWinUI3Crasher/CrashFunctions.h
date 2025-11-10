#pragma once

namespace CrashExamples
{

// Crash example functions from MyConsoleCrasher
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
void HeapCorruption();
void PrivilegedInstruction();
void DoubleDelete();
void UseAfterFree();
void InvalidFunctionPointer();
void FastFail();
void CreateXmlReport();
void ApplicationHang();

// Helper for thread crashes
DWORD WINAPI MyThreadCrasher(LPVOID);

} // namespace CrashExamples
