// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <malloc.h>
#include <sys/stat.h>

using namespace Common;
using namespace std;

namespace
{
    const StringLiteral TraceType("ProcessInfo");

    INIT_ONCE initOnce = INIT_ONCE_STATIC_INIT;
    ProcessInfo* singleton = nullptr;

    BOOL CALLBACK InitFunction(PINIT_ONCE, PVOID, PVOID *)
    {
        singleton = new ProcessInfo();
        return TRUE;
    }
}

ProcessInfo* ProcessInfo::GetSingleton()
{
    BOOL  bStatus = ::InitOnceExecuteOnce(
        &initOnce,
        InitFunction,
        nullptr,
        nullptr);

    ASSERT_IF(!bStatus, "Failed to initialize ProcessInfo singleton");
    return singleton;
}

ProcessInfo::ProcessInfo()
    : proc_self_statm_fd_(open("/proc/self/statm", O_RDONLY))
    , pageSize_(getpagesize())
    , vmSize_()
    , rss_()
    , dataSize_()
{
    ASSERT_IF(proc_self_statm_fd_ < 0, "open(/proc/self/statm) failed: {0}", errno);
}

ProcessInfo::~ProcessInfo()
{
    close(proc_self_statm_fd_);
}

void ProcessInfo::read_proc_self_statm(char* buf, size_t bufSize)
{
    auto len = pread(proc_self_statm_fd_, buf, bufSize, 0);
    Invariant(len > 0);
    buf[len] = 0;
}

size_t ProcessInfo::RefreshDataSize()
{
    char buf[256];
    read_proc_self_statm(buf, sizeof(buf));

    size_t pageCount = 0;
    auto retval = sscanf(buf, "%*Lu %*Lu %*Lu %*Lu %*Lu %Lu", &pageCount); 
    Invariant(retval == 1);

    dataSize_ = pageCount * pageSize_;
    return dataSize_;
}

void ProcessInfo::RefreshAll()
{
    char buf[256];
    read_proc_self_statm(buf, sizeof(buf));
    read_proc_self_status();

    size_t vmPageCount = 0, rssPageCount = 0, dataPageCount = 0;
    auto retval = sscanf(buf, "%Lu %Lu %*Lu %*Lu %*Lu %Lu", &vmPageCount, &rssPageCount, &dataPageCount); 
    Invariant(retval == 3);

    vmSize_ = vmPageCount * pageSize_;
    rss_ = rssPageCount * pageSize_;
    dataSize_ = dataPageCount * pageSize_;
}

void ProcessInfo::read_proc_self_status()
{
    FILE *f = fopen("/proc/self/status", "r"); //no need to cache FILE*, as this is not called on perf critical path
    Invariant(f);
    KFinally([f] { fclose(f); });

    char buffer[2048];
    char *vmPeakStr = nullptr, *rssPeakStr = nullptr, *thrCountStr = nullptr;
    while (!vmPeakStr || !rssPeakStr || !thrCountStr)
    {
        char *line = buffer;
        size_t len = sizeof(buffer);
        if (getline(&line, &len, f) == -1)
        {
            Assert::CodingError("getline failed: {0}", errno);
        }

        if (!strncmp(line, "VmPeak:", 7)) {
            vmPeakStr = &line[7];
            len = strlen(vmPeakStr);
            vmPeakStr[len - 4] = 0;
            vmPeak_ = atoll(vmPeakStr) << 10;
        }
        else if (!strncmp(line, "VmHWM:", 6)) {
            rssPeakStr = &line[6];
            len = strlen(rssPeakStr);
            rssPeakStr[len - 4] = 0;
            rssPeak_ = atoll(rssPeakStr) << 10;
        }
        else if (!strncmp(line, "Threads:", 8))
        {
            thrCountStr = &line[8];
            threadCount_ = atoi(thrCountStr);
        }
    }
}

size_t ProcessInfo::GetRssAndSummary(string& summary)
{
    RefreshAll();
    auto rss = rss_;

    summary = GenerateSummaryString(threadCount_, dataSize_, vmSize_, vmPeak_, rss, rssPeak_);
    return rss; 
}

size_t ProcessInfo::GetDataSizeAndSummary(string & summary)
{
    RefreshAll();
    auto dataSize = dataSize_;

    summary = GenerateSummaryString(threadCount_, dataSize, vmSize_, vmPeak_, rss_, rssPeak_);
    return dataSize;
}

string ProcessInfo::GenerateSummaryString(
    uint thrCount,
    size_t dataSize,
    size_t vmSize,
    size_t vmPeak,
    size_t rss,
    size_t rssPeak)
{
    string summary;
    StringWriterA sw(summary);
    sw.WriteLine(
        "threads = {0}, virtual memory: data = {1}, vmSize = {2}, vmPeak = {3}, rss = {4}, rssPeak = {5}",
        thrCount,
        dataSize ,
        vmSize,
        vmPeak,
        rss,
        rssPeak);

//    Disable the following as it is not supported by jemalloc
//    auto mallocInfoFile = tmpfile(); //use tmpfile as malloc_info output can be quite large
//    KFinally([=] { if (mallocInfoFile) fclose(mallocInfoFile); });
//    if (!mallocInfoFile)
//    {
//        Trace.WriteError(TraceType, "tmpfile failed: {0}", errno);
//        return summary;
//    }
//
//    auto retval = malloc_info(0, mallocInfoFile);
//    if (retval)
//    {
//        Trace.WriteError(TraceType, "malloc_info failed: {0}", errno);
//        return summary;
//    }
//
//    retval = fflush(mallocInfoFile);
//    if (retval)
//    {
//        Trace.WriteError(TraceType, "fflush failed: {0}", errno);
//        return summary;
//    }
//
//    struct stat statSt = {};
//    retval = fstat(fileno(mallocInfoFile), &statSt);
//    if (retval)
//    {
//        Trace.WriteError(TraceType, "fstat failed: {0}", errno);
//        return summary;
//    }
//
//    char buf[1024];
//    int seekOffset = -1 * std::min((size_t)(statSt.st_size), sizeof(buf) - 1);
//    retval = fseek(mallocInfoFile, seekOffset, SEEK_END);
//    if (retval)
//    {
//        Trace.WriteError(TraceType, "fseek failed: {0}", errno);
//        return summary;
//    }
//
//    auto read = fread(buf, 1, sizeof(buf) - 1, mallocInfoFile); 
//    buf[read] = 0;
//    if (!read)
//    {
//        return summary;
//    }
//
//    char* marker = "</heap>";
//    char* ptr = buf;
//    for(;;)
//    {
//        char* newPtr = strstr(ptr, marker);
//        if (!newPtr) break;
//        ptr = newPtr + sizeof(marker);
//    }
//
//    sw.WriteLine("<malloc> // free = \"fast\" + \"rest\"");
//    sw.Write("{0}", ptr);

    return summary;
}
