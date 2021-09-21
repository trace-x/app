#ifndef STACK_TRACE_H
#define STACK_TRACE_H

#include "../trace_x.h"

#include <stdio.h>
#include <ctime>

#undef TRACE_X_MODULE_NAME
#define TRACE_X_MODULE_NAME "stack_trace"

#define TRACE_X_PRINT_ERROR(fd, ...) \
    if(fd) ::fprintf(fd, __VA_ARGS__); \
    ::fprintf(stderr, __VA_ARGS__); \
    X_ERROR_S_F(__VA_ARGS__);

namespace stack_trace
{

void get_time_string(char *string, size_t string_size)
{
    std::time_t rawtime;
    struct tm *timeinfo;

    std::time(&rawtime);
    timeinfo = std::localtime(&rawtime);

    std::strftime(string, string_size, "%Y.%m.%d_%H:%M:%S", timeinfo);
}

}

#ifdef _WIN32

#include <Windows.h>
#include <Dbghelp.h>
#include <signal.h>

#include "stack_walker.h"

namespace stack_trace
{

class TraceStackWalker : public StackWalker
{
public:
    TraceStackWalker() : StackWalker() {}

protected:
    virtual void OnOutput(LPCSTR text)
    {
        StackWalker::OnOutput(text);

        X_INFO_S_F("%s", text);
    }
};

void make_dump(EXCEPTION_POINTERS *e = 0)
{
    HMODULE dbghelp_h = LoadLibraryA("dbghelp");

    if(dbghelp_h == nullptr)
    {
        return;
    }

    auto dump_writer = (decltype(&MiniDumpWriteDump))GetProcAddress(dbghelp_h, "MiniDumpWriteDump");

    if(dump_writer == nullptr)
    {
        return;
    }

    //TODO: write to directory "logs"

    char name[MAX_PATH];

    {
        auto name_end = name + GetModuleFileNameA(GetModuleHandleA(0), name, MAX_PATH);
        SYSTEMTIME t;
        GetSystemTime(&t);
        wsprintfA(name_end - strlen(".exe"), "_%4d%02d%02d_%02d%02d%02d.dmp",
                  t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
    }

    fprintf(stderr, "dump created: %s\n", name);

    X_INFO_S_F("dump created: %s", name);

    HANDLE file_handle = CreateFileA(name, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

    if(file_handle == INVALID_HANDLE_VALUE)
    {
        return;
    }

    MINIDUMP_EXCEPTION_INFORMATION exception_information;

    exception_information.ThreadId = GetCurrentThreadId();
    exception_information.ExceptionPointers = e;
    exception_information.ClientPointers = FALSE;

    dump_writer(GetCurrentProcess(), GetCurrentProcessId(), file_handle,
                MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory),
                e ? &exception_information : nullptr, nullptr, nullptr);

    CloseHandle(file_handle);

    //

    TraceStackWalker walker;

    walker.ShowCallstack();
}

LONG CALLBACK unhandled_handler(EXCEPTION_POINTERS* e)
{
    X_ERROR_F("Caught unhandled exception");

    make_dump(e);

    return EXCEPTION_CONTINUE_SEARCH;
}


void abort_handler(int signum)
{
    const char *name = "unknown";

    switch(signum)
    {
    case SIGABRT: name = "SIGABRT";  break;
    case SIGSEGV: name = "SIGSEGV";  break;
    case SIGILL:  name = "SIGILL";   break;
    case SIGFPE:  name = "SIGFPE";   break;
    case SIGINT:  name = "SIGINT";   break;
#ifndef _WIN32
    case SIGBUS:  name = "SIGBUS";   break;
    case SIGPIPE: name = "SIGPIPE";   break;
#endif
    }

    fprintf(stderr, "Caught signal %d (%s)\n", signum, name);

    TRACE_X_PRINTF(trace_x::MESSAGE_CRASH, "Caught signal %d (%s)", signum, name);

#ifdef _WIN32
    make_dump();
#else
    print_stack_trace();
#endif

    exit(signum);
}

class StackTrace
{
public:
    StackTrace()
    {
        SetUnhandledExceptionFilter(unhandled_handler);

        signal(SIGABRT, abort_handler);
        signal(SIGSEGV, abort_handler);
        signal(SIGILL,  abort_handler);
        signal(SIGFPE,  abort_handler);

#ifndef _WIN32
        signal(SIGBUS,  abort_handler);
        signal(SIGPIPE, abort_handler);
#endif
    }
};

} // namespace stack_trace

#else // ifdef _WIN32

#include <errno.h>
#include <ucontext.h>
#include <cxxabi.h>
#include <execinfo.h>
#include <sys/prctl.h>
#include <sys/times.h>
#include <sys/wait.h>

namespace stack_trace
{

typedef void (*Handler)(int);

static struct sigaction g_prev_signal_handler;

bool get_command_output(const char *command, char *output, int output_size)
{
    FILE *stream = ::popen(command, "r");

    if(stream)
    {
        char *r = ::fgets(output, output_size, stream);

        ::pclose(stream);

        return true;
    }

    return false;
}

bool check_command_exists(const char *command)
{
    char command_buffer[256];

    ::sprintf(command_buffer, "which %s > /dev/null 2>&1", command);

    int system_error = ::system(command_buffer);

    return !system_error;
}

void print_trace_demangle(FILE *file = 0, bool with_filename = false)
{
    TRACE_X_PRINT_ERROR(file, "\n=========== backtrace ==========\n\n");

    const int skip = 3;

    void *callstack[128];

    int addrlen = ::backtrace(callstack, sizeof(callstack) / sizeof(void*));

    if(addrlen == 0)
    {
        return;
    }

    // resolve addresses into strings containing "filename(function+address)",
    // Actually it will be ## program address function + offset
    // this array must be free()-ed

    // TODO avoid backtrace_symbols
    char **symbollist = ::backtrace_symbols(callstack, addrlen);

    size_t funcnamesize = 1024;

    char funcname[funcnamesize];

    // iterate over the returned symbol lines. skip the first, it is the
    // address of this function.
    for(int i = skip; i < addrlen; i++)
    {
        char *begin_name   = 0;
        char *begin_offset = 0;
        char *end_address  = 0;

        for(char *p = symbollist[i]; *p; ++p)
        {
            if(*p == '(')
            {
                begin_name = p;
            }
            else if(*p == '+')
            {
                begin_offset = p;
            }
            else if(*p == ')' && (begin_offset || begin_name))
            {
                end_address = p;
            }
        }

        if(begin_name && end_address && (begin_name < end_address))
        {
            *begin_name++ = '\0';
            *end_address++ = '\0';

            if(begin_offset)
            {
                *begin_offset++ = '\0';
            }

            // mangled name is now in [begin_name, begin_offset) and caller
            // offset in [begin_offset, end_offset). now apply __cxa_demangle():

            int status = 0;

            char *ret = ::abi::__cxa_demangle(begin_name, funcname, &funcnamesize, &status);
            char *fname = begin_name;

            if(status == 0)
            {
                fname = ret;
            }

            char filename[1024];

            if(with_filename)
            {
                // find out source name by address

                char syscom[4096];

                ::sprintf(syscom,"addr2line %p -C -e \"%s\"", callstack[i], symbollist[i]);

                filename[0] = '\n';

                get_command_output(syscom, filename, sizeof(filename));
            }

            if (begin_offset)
            {
                if(with_filename)
                {
                    TRACE_X_PRINT_ERROR(file, "#%-4d %-18s in %s + %s from %s at %s", i, end_address, fname, begin_offset, symbollist[i], filename);
                }
                else
                {
                    TRACE_X_PRINT_ERROR(file, "#%-4d %-18s in %s + %s from %s\n", i, end_address, fname, begin_offset, symbollist[i]);
                }
            }
            else
            {
                TRACE_X_PRINT_ERROR(file, "#%-4d %-18s from %s\n", i, end_address, symbollist[i]);
            }
        }
        else
        {
            // couldn't parse the line? print the whole line.

            TRACE_X_PRINT_ERROR(file, "%-s\n", symbollist[i]);
        }
    }

    if(file)
    {
        ::fflush(file);
    }

    ::free(symbollist);
}

void print_trace_gdb(FILE *file = 0, bool full_backtrace = false)
{
    if(!check_command_exists("gdb"))
    {
        // gdb is not avialable

        return;
    }

    char pid_buf[30];

    ::sprintf(pid_buf, "%d", getpid());

    // take access to ptrace
    ::prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY, 0, 0, 0);

    int pipefd[2];

    int error = ::pipe(pipefd);

    if(error)
    {
        return;
    }

    int child_pid = fork();

    if(!child_pid)
    {
        ::close(pipefd[0]); // close reading end in the child

        ::dup2(pipefd[1], 1); // send stdout to the pipe
        ::dup2(pipefd[1], 2); // send stderr to the pipe

        ::close(pipefd[1]);

        if(full_backtrace)
        {
            ::execlp("gdb", "gdb", "--batch", "-n", "-ex", "thread", "-ex", "bt -4 full", program_invocation_name, pid_buf, NULL);
        }
        else
        {
            ::execlp("gdb", "gdb", "--batch", "-n", "-ex", "thread apply all bt", "-ex", "bt -4", program_invocation_name, pid_buf, NULL);
        }

        ::_Exit(-1);
    }
    else
    {
        waitpid(child_pid, 0, 0);

        char buffer[30564];

        close(pipefd[1]);

        if(read(pipefd[0], buffer, sizeof(buffer)))
        {
            TRACE_X_PRINT_ERROR(file, "\n=========== %s backtrace ==========\n\n", full_backtrace ? "gdb full" : "gdb");
            TRACE_X_PRINT_ERROR(file, "%s", buffer);
        }

        if(file)
        {
            ::fflush(file);
        }
    }
}

class StackTrace
{
public:
    static bool _use_gdb;
    static bool _use_gdb_full;
    static std::string _crash_report_prefix;

    StackTrace()
    {
        struct sigaction handler;

        handler.sa_sigaction = &StackTrace::signal_handler;
        handler.sa_flags = SA_SIGINFO;

        ::sigemptyset(&handler.sa_mask);

        install_handler(SIGSEGV, &handler);
        install_handler(SIGABRT, &handler);
        install_handler(SIGFPE, &handler);
        install_handler(SIGBUS, &handler);

        //  install_handler(SIGINT, &handler);
        //  install_handler(SIGILL, &handler);
        //  install_handler(SIGPIPE, &handler);
    }

    void install_handler(int code, const struct sigaction *handler)
    {
        struct sigaction prev_handler;

        ::sigaction(code, 0, &prev_handler);

        if(prev_handler.sa_handler != SIG_IGN)
        {
            ::sigaction(code, handler, &g_prev_signal_handler);
        }
    }

    static void signal_handler(int signum, siginfo_t *info, void *context)
    {
        if(_crash_report_prefix.empty())
        {
            return;
        }

        char date_time_string[128];
        char crash_report_file_name[1024];

        get_time_string(date_time_string, sizeof(date_time_string));

        ::sprintf(crash_report_file_name, "%s/%s_crash_report.txt", _crash_report_prefix.c_str(), date_time_string);

        FILE * g_stacktrace_file = ::fopen(crash_report_file_name, "w");

        TRACE_X_PRINT_ERROR(g_stacktrace_file, "crash report file created: %s\n", crash_report_file_name);

        struct tms times_buffer;

        times(&times_buffer);

        void *caller_address;
        ucontext_t *uc = (ucontext_t *)context;

        /* Get the address at the time the signal was raised */
#if defined(__i386__) // gcc specific
        caller_address = (void *) uc->uc_mcontext.eip; // EIP: x86 specific
#elif defined(__x86_64__) // gcc specific
        caller_address = (void *) uc->uc_mcontext.gregs[REG_RIP]; // RIP: x86_64 specific
#elif defined(__arm__)
        caller_address = (void *) uc->uc_mcontext.arm_pc;
#elif defined(__aarch64__)
        caller_address = (void *) uc->uc_mcontext.pc;
#else
#error Unsupported architecture. // TODO: Add support for other arch.
#endif

        TRACE_X_PRINT_ERROR(g_stacktrace_file, "error signal caught : %s (%d)\n\n", ::strsignal(signum), signum);
        TRACE_X_PRINT_ERROR(g_stacktrace_file, "signal code: %d\n", info->si_code);
        TRACE_X_PRINT_ERROR(g_stacktrace_file, "address: %p from %p\n", info->si_addr, caller_address);
        TRACE_X_PRINT_ERROR(g_stacktrace_file, "user time : %f\n", (float)times_buffer.tms_utime / ::sysconf(_SC_CLK_TCK));
        TRACE_X_PRINT_ERROR(g_stacktrace_file, "system time : %f\n", (float)times_buffer.tms_stime / ::sysconf(_SC_CLK_TCK));

        print_trace_demangle(g_stacktrace_file, true);

        if(_use_gdb)
        {
            print_trace_gdb(g_stacktrace_file, false);

            if(_use_gdb_full)
            {
                print_trace_gdb(g_stacktrace_file, true);
            }
        }

        fclose(g_stacktrace_file);

        if((g_prev_signal_handler.sa_handler != SIG_DFL) && (g_prev_signal_handler.sa_handler != SIG_IGN))
        {
            g_prev_signal_handler.sa_sigaction(signum, info, context);
        }
        else
        {
            ::exit(EXIT_FAILURE);
        }
    }
};

bool StackTrace::_use_gdb = true;
bool StackTrace::_use_gdb_full = false;
std::string StackTrace::_crash_report_prefix = std::string();

} // namespace stack_trace

#endif // ifdef _WIN32

#endif // STACK_TRACE_H


