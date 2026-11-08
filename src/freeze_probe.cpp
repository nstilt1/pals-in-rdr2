// Original local diagnostics. Reads thread stacks only; no game assets, process
// patching, DRM changes, or remote uploads. Every suspended thread is resumed.
#include <windows.h>
#include <dbghelp.h>
#include <tlhelp32.h>
#include <iostream>
#include <iomanip>
#include <cstdlib>

struct Handle {
    HANDLE h;
    ~Handle() { if (h && h != INVALID_HANDLE_VALUE) CloseHandle(h); }
};
struct ResumeGuard {
    HANDLE thread;
    ~ResumeGuard() { ResumeThread(thread); }
};
void address(HANDLE process, DWORD64 pc) {
    IMAGEHLP_MODULE64 info{}; info.SizeOfStruct = sizeof(info);
    if (SymGetModuleInfo64(process, pc, &info))
        std::cout << info.ModuleName << "+0x" << std::hex << (pc-info.BaseOfImage);
    else std::cout << "0x" << std::hex << pc;
    std::cout << std::dec;
}
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: freeze_probe PROCESS_ID\n"; return 2; }
    const DWORD pid = std::strtoul(argv[1], nullptr, 10);
    Handle process{OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid)};
    if (!process.h) { std::cerr << "OpenProcess failed: " << GetLastError(); return 1; }
    SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_UNDNAME);
    if (!SymInitialize(process.h, "", TRUE)) { std::cerr << "SymInitialize failed"; return 1; }
    Handle snapshot{CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0)};
    THREADENTRY32 entry{}; entry.dwSize=sizeof(entry);
    if (!Thread32First(snapshot.h, &entry)) return 1;
    do {
        if (entry.th32OwnerProcessID != pid) continue;
        Handle thread{OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
                                  FALSE, entry.th32ThreadID)};
        if (!thread.h || SuspendThread(thread.h) == DWORD(-1)) continue;
        ResumeGuard resume{thread.h};
        CONTEXT context{}; context.ContextFlags=CONTEXT_FULL;
        if (!GetThreadContext(thread.h, &context)) continue;
        std::cout << "THREAD " << entry.th32ThreadID << " RIP=";
        address(process.h, context.Rip);
        std::cout << " RCX=0x" << std::hex << context.Rcx << " RDX=0x" << context.Rdx
                  << " R8=0x" << context.R8 << " R9=0x" << context.R9 << std::dec << '\n';
        STACKFRAME64 frame{};
        frame.AddrPC={context.Rip,0,AddrModeFlat};
        frame.AddrFrame={context.Rbp,0,AddrModeFlat};
        frame.AddrStack={context.Rsp,0,AddrModeFlat};
        for (unsigned i=0; i<24; ++i) {
            if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64,process.h,thread.h,&frame,&context,
                             nullptr,SymFunctionTableAccess64,SymGetModuleBase64,nullptr)
                || !frame.AddrPC.Offset) break;
            std::cout << "  " << i << ' '; address(process.h,frame.AddrPC.Offset); std::cout << '\n';
        }
    } while (Thread32Next(snapshot.h,&entry));
    SymCleanup(process.h);
}
