#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sched.h>
#include <pthread.h>
#include <unistd.h>
#include "MinPal.h"
#include "Tracer.h"

namespace KtlThreadpool{

    BOOL QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount)
    {
        BOOL retval = TRUE;
        struct timespec ts;

        if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        {
            ASSERT("clock_gettime(CLOCK_MONOTONIC) failed; errno is %d (%s)\n", errno, strerror(errno));
            retval = FALSE;
            goto EXIT;
        }

        lpPerformanceCount->QuadPart = (LONGLONG)ts.tv_sec * (LONGLONG)tccSecondsToNanoSeconds + (LONGLONG)ts.tv_nsec;

    EXIT:
        return retval;
    }

    BOOL QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency)
    {
        lpFrequency->QuadPart = (LONGLONG) tccSecondsToNanoSeconds;
        return TRUE;
    }

    ULONGLONG GetTickCount64()
    {
        ULONGLONG retval = 0;

        struct timespec ts;
        if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        {
            ASSERT("clock_gettime(CLOCK_MONOTONIC) failed; errno is %d (%s)\n", errno, strerror(errno));
            goto EXIT;
        }
        retval = ((ULONGLONG)ts.tv_sec * tccSecondsToMillieSeconds) + (ts.tv_nsec / tccMillieSecondsToNanoSeconds);

    EXIT:
        return retval;
    }

    DWORD GetTickCount()
    {
        return (DWORD)GetTickCount64();
    }

    DWORD GetCurrentThreadId()
    {
        pid_t tid = (pid_t) syscall(__NR_gettid);
        return (DWORD)tid;
    }

    DWORD GetCurrentProcessId()
    {
        return (DWORD)getpid();
    }

#if __aarch64__
     VOID YieldProcessor()
    {
        sched_yield();
    }
#else
    VOID YieldProcessor()
    {
        __asm__ __volatile__ (
            "rep\n"
            "nop"
        );
    }
#endif

    BOOL SwitchToThread()
    {
        BOOL ret;
        // sched_yield yields to another thread in the current process. This implementation
        // won't work well for cross-process synchronization.
        ret = (sched_yield() == 0);
        return ret;
    }

    #define SWITCH_SLEEP_START_THRESHOLD 8
    #define SWITCH_SLEEP_INTERVAL 5
    bool __SwitchToThread(uint32_t dwSleepMSec, uint32_t dwSwitchCount)
    {
        if (!dwSleepMSec && (dwSwitchCount > SWITCH_SLEEP_START_THRESHOLD))
        {
            dwSleepMSec = SWITCH_SLEEP_INTERVAL;
        }

        if(dwSleepMSec != 0)
        {
            struct timespec tim, remain;
            tim.tv_sec = dwSleepMSec / 1000;
            tim.tv_nsec = (dwSleepMSec % 1000) * 1000000;
            while ((nanosleep(&tim, &remain) < 0) && (errno == EINTR))
            {
                tim.tv_sec = remain.tv_sec;
                tim.tv_nsec = remain.tv_nsec;
            }
        }
        else
            SwitchToThread();

        return true;
    }

    VOID Sleep(uint32_t dwSleepMSec)
    {
        __SwitchToThread(dwSleepMSec, 0);
    }

    DWORD GetCurrentProcessorNumber()
    {
        return sched_getcpu();
    }

    DWORD GetNumActiveProcessors()
    {
        return sysconf(_SC_NPROCESSORS_ONLN);
    }

    VOID GetProcessDefaultStackSize(SIZE_T* stacksize)
    {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_getstacksize(&attr, stacksize);
        return;
    }

    INT GetCPUBusyTime(TP_CPU_INFORMATION *lpPrevCPUInfo) {
        ULONGLONG nLastRecordedCurrentTime = 0;
        ULONGLONG nLastRecordedUserTime = 0;
        ULONGLONG nLastRecordedKernelTime = 0;
        ULONGLONG nKernelTime = 0;
        ULONGLONG nUserTime = 0;
        ULONGLONG nCurrentTime = 0;
        ULONGLONG nCpuBusyTime = 0;
        ULONGLONG nCpuTotalTime = 0;
        DWORD nReading = 0;
        struct rusage resUsage;
        struct timeval tv;
        struct timespec ts;
        static DWORD dwNumberOfProcessors = 0;

        dwNumberOfProcessors = GetNumActiveProcessors();
        if (dwNumberOfProcessors <= 0)
        {
            return 0;
        }

        if (getrusage(RUSAGE_SELF, &resUsage) != -1)
        {
            nKernelTime = (ULONGLONG) resUsage.ru_stime.tv_sec * tccSecondsTo100NanoSeconds +
                          resUsage.ru_stime.tv_usec * tccMicroSecondsTo100NanoSeconds;

            nUserTime = (ULONGLONG) resUsage.ru_utime.tv_sec * tccSecondsTo100NanoSeconds +
                        resUsage.ru_utime.tv_usec * tccMicroSecondsTo100NanoSeconds;

            if (clock_gettime(CLOCK_MONOTONIC, &ts) != -1)
            {
                nCurrentTime = (ULONGLONG) ts.tv_sec * tccSecondsTo100NanoSeconds +
                               ts.tv_nsec / 100;
            }

            nLastRecordedCurrentTime = FILETIME_TO_ULONGLONG(lpPrevCPUInfo->LastRecordedTime.ftLastRecordedCurrentTime);
            nLastRecordedUserTime = FILETIME_TO_ULONGLONG(lpPrevCPUInfo->ftLastRecordedUserTime);
            nLastRecordedKernelTime = FILETIME_TO_ULONGLONG(lpPrevCPUInfo->ftLastRecordedKernelTime);

            if (nLastRecordedCurrentTime != 0)
            {
                if (nCurrentTime > nLastRecordedCurrentTime)
                {
                    nCpuTotalTime = (nCurrentTime - nLastRecordedCurrentTime);
                    nCpuTotalTime *= dwNumberOfProcessors;
                }
                if (nUserTime >= nLastRecordedUserTime && nKernelTime >= nLastRecordedKernelTime)
                {
                    nCpuBusyTime = (nUserTime - nLastRecordedUserTime) + (nKernelTime - nLastRecordedKernelTime);
                }

                if (nCpuTotalTime > 0 && nCpuBusyTime > 0)
                {
                    nReading = (DWORD) ((nCpuBusyTime * 100) / nCpuTotalTime);
                }

                //if (nReading > 100 + 30)   // leave some buffer for inaccuracy
                //{
                //    TP_TRACE(Info, "cpu utilization (%d) > 100\n", nReading);
                //}
                nReading = __min(nReading, 99);
            }

            lpPrevCPUInfo->LastRecordedTime.ftLastRecordedCurrentTime.dwLowDateTime = (DWORD) nCurrentTime;
            lpPrevCPUInfo->LastRecordedTime.ftLastRecordedCurrentTime.dwHighDateTime = (DWORD) (nCurrentTime >> 32);

            lpPrevCPUInfo->ftLastRecordedUserTime.dwLowDateTime = (DWORD) nUserTime;
            lpPrevCPUInfo->ftLastRecordedUserTime.dwHighDateTime = (DWORD) (nUserTime >> 32);

            lpPrevCPUInfo->ftLastRecordedKernelTime.dwLowDateTime = (DWORD) nKernelTime;
            lpPrevCPUInfo->ftLastRecordedKernelTime.dwHighDateTime = (DWORD) (nKernelTime >> 32);
        }
        return (DWORD) nReading;
    }
}
