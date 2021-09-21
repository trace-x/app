#ifndef _TRACE_PLATFORM_H_
#define _TRACE_PLATFORM_H_

#include <stdint.h>
#include <string>
#include <cstdlib>
#include <iostream>
#include <cstdio>
#include <fstream>
#include <boost/filesystem.hpp>

#ifdef _WIN32

#include <windows.h>
#include <psapi.h>
#include <lmcons.h>

#else

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <pwd.h>

#endif

namespace trace_x
{

#ifdef _WIN32

class CriticalSection
{
public:
    CriticalSection()
    {
        InitializeCriticalSection(&_critical_section);
    }

    void lock()
    {
        EnterCriticalSection(&_critical_section);
    }

    void unlock()
    {
        LeaveCriticalSection(&_critical_section);
    }

private:
    CRITICAL_SECTION _critical_section;
};

#else

class CriticalSection
{
public:
    CriticalSection()
    {
        pthread_mutex_init(&_critical_section, 0);
    }

    void lock()
    {
        pthread_mutex_lock(&_critical_section);
    }

    void unlock()
    {
        pthread_mutex_unlock(&_critical_section);
    }

private:
    pthread_mutex_t _critical_section;
};

#endif

class MutexLocker
{
public:
    MutexLocker(CriticalSection &cs) : _cs(cs) { _cs.lock(); }

    ~MutexLocker() { _cs.unlock(); }

private:
    CriticalSection &_cs;
};

inline void sleep(int milliseconds)
{
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

inline uint64_t get_pid()
{
#ifdef _WIN32
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}

inline std::string get_process_name()
{
#ifdef _WIN32
    TCHAR buffer[MAX_PATH];

    GetModuleFileNameEx(GetCurrentProcess(), 0, buffer, MAX_PATH);

    return buffer;
#else
    char name[1024];

    sprintf(name, "/proc/%d/cmdline", int(getpid()));
    FILE* f = fopen(name,"r");

    if(f)
    {
        size_t size = fread(name, sizeof(char), 1024, f);

        if(size > 0)
        {
            if('\n' == name[size-1])
            {
                name[size-1]='\0';
            }
        }

        fclose(f);
    }

    if(boost::filesystem::exists(name))
    {
        return boost::filesystem::canonical(name).string();
    }

    return name;
#endif
}

inline uint64_t get_tid()
{
#ifdef _WIN32
    return GetCurrentThreadId();
#else
    return pthread_self();
#endif
}

inline uint64_t get_timestamp()
{
#ifdef _WIN32
    LARGE_INTEGER perf_count, perf_freq;
    QueryPerformanceCounter(&perf_count);
    QueryPerformanceFrequency(&perf_freq);

    LONGLONG perf_count_sec = perf_count.QuadPart / perf_freq.QuadPart;
    LONGLONG perf_count_subsec = perf_count.QuadPart % perf_freq.QuadPart;
    LONGLONG timestamp = perf_count_sec * 0x100000 + perf_count_subsec * 0x100000 / perf_freq.QuadPart;

    return  timestamp;
#else
    struct timeval tv;
    gettimeofday(&tv, 0);

    uint64_t timestamp = uint64_t(tv.tv_sec) * 1000000 + tv.tv_usec;

    return timestamp;
#endif
}

inline std::string get_user_name()
{
#ifdef _WIN32
    char username[UNLEN+1];

    DWORD username_len = UNLEN+1;

    GetUserName(username, &username_len);
    // GetUserNameEx()
    return username;
#else
    std::string username = "unknown_user";

    uid_t uid = geteuid();

    struct passwd *pw = getpwuid(uid);

    if(pw)
    {
        username = pw->pw_name;
    }

    return username;
#endif
}

#ifdef _WIN32

inline std::string get_error_string()
{
    DWORD error = GetLastError();

    if (error)
    {
        LPVOID lpMsgBuf;

        DWORD bufLen = FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    error,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR) &lpMsgBuf,
                    0, NULL );

        if (bufLen)
        {
            LPCSTR lpMsgStr = (LPCSTR)lpMsgBuf;
            std::string result(lpMsgStr, lpMsgStr+bufLen);

            LocalFree(lpMsgBuf);

            return result;
        }
    }

    return std::string();
}

class LocalSocket
{
public:
    LocalSocket():
        _handle(INVALID_HANDLE_VALUE)
    {}

    ~LocalSocket()
    {
        close();
    }

    // 1 - success, 0 - not found, -1 - error
    int connect(const std::string &server_name)
    {
        _handle = CreateFileA(("\\\\.\\pipe\\" + server_name).c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);

        DWORD error = GetLastError();

        if(error != ERROR_SUCCESS)
        {
            if(error != ERROR_FILE_NOT_FOUND)
            {
                std::fprintf(stderr, "can`t create pipe handle: %d\n", error);
            }

            _handle = INVALID_HANDLE_VALUE;
        }

        return is_open();
    }

    void close()
    {
        if(_handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(_handle);

            _handle = INVALID_HANDLE_VALUE;
        }
    }

    bool send(const void *data, size_t size)
    {
        DWORD n_write = 0;

        return (WriteFile(_handle, data, DWORD(size), &n_write, NULL) == TRUE);
    }

    bool wait_ack()
    {
        uint8_t reply_ack;
        DWORD n_read = 0;

        return (ReadFile(_handle, &reply_ack, 1, &n_read, NULL) == TRUE);
    }

    bool is_open() const
    {
        return (_handle != INVALID_HANDLE_VALUE);
    }

    bool check_state()
    {
        return is_open() && send(0, 0);
    }

private:
    HANDLE _handle;
};

inline int execute(const std::string &command)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    CreateProcess(NULL, const_cast<char*>(command.c_str()), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exit_code;

    GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exit_code;
}

inline bool start_detached(const std::string &command)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    BOOL result = CreateProcess(NULL, const_cast<char*>(command.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

    if(!result)
    {
        std::fprintf(stderr, "start_detached failed: %d\n", GetLastError());
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return (result != 0);
}

#else

inline std::string tmp_dir()
{
//    char *env = getenv("TMPDIR");

//    std::string path;

//    if(env)
//    {
//        path = env;
//    }

//    if(path.empty())
//    {
//        path = "/tmp";
//    }

//    return path + "/";

    // TODO

    return "/tmp/";
}

class LocalSocket
{
public:
    LocalSocket():
        _handle(-1)
    {}

    ~LocalSocket()
    {
        close();
    }

    bool connect(const std::string &server_name)
    {
        struct sockaddr_un bind_addr;
        struct sockaddr_un srv_addr;

        bind_addr.sun_family = AF_LOCAL;
        srv_addr.sun_family = AF_LOCAL;

        strcpy(srv_addr.sun_path, (tmp_dir() + server_name).c_str());

        int handle = ::socket(PF_UNIX, SOCK_STREAM, 0);

        if(handle != -1)
        {
            if(::connect(handle, (sockaddr*)&srv_addr, SUN_LEN(&srv_addr)) == 0)
            {
                _handle = handle;
            }
            else if(std::ifstream(srv_addr.sun_path).good())
            {
                std::fprintf(stderr, "connect failed at socket %d [%s]: %s\n", handle, srv_addr.sun_path, strerror(errno));
            }
        }
        else
        {
            std::fprintf(stderr, "socket creation failed: %s\n", strerror(errno));
        }

        return is_open();
    }

    void close()
    {
        if(_handle != -1)
        {
            ::close(_handle);

            _handle = -1;
        }
    }

    bool send(const void *data, size_t size)
    {
        return (::send(_handle, data, size, MSG_NOSIGNAL) != -1);
    }

    bool wait_ack()
    {
        uint8_t buf = 0;

        return (::recv(_handle, &buf, sizeof(buf), 0) == sizeof(buf));
    }

    bool is_open() const
    {
        return (_handle != -1);
    }

private:
    int _handle;
};

inline int execute(const std::string &command)
{
    int result = std::system(command.c_str());

    return WEXITSTATUS(result);
}

inline bool start_detached(const std::string &command)
{
    int result = std::system((command + " &").c_str());

    if(result != 0)
    {
        std::fprintf(stderr, "start_detached failed: %d\n", result);
    }

    return (result == 0);
}

#endif

}

#endif //ifndef _TRACE_PLATFORM_H_
