// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#ifdef ETL_TRACING_ENABLED
#include "../../../Common/Common.h"
#include "FabricRuntime_.h"
#define TRACE_ERROR(...)    {SFBDCtlLibTrace::TraceError(SFBDCtlLibTrace::format(__VA_ARGS__));}
#define TRACE_WARNING(...)  {SFBDCtlLibTrace::TraceWarning(SFBDCtlLibTrace::format(__VA_ARGS__));}
#define TRACE_INFO(...)     {SFBDCtlLibTrace::TraceInfo(SFBDCtlLibTrace::format(__VA_ARGS__));}
#define TRACE_NOISE(...)    {SFBDCtlLibTrace::TraceNoise(SFBDCtlLibTrace::format(__VA_ARGS__));}
#else
#define TRACE_ERROR(...)    {printf("[ERROR] %s", SFBDCtlLibTrace::format(__VA_ARGS__).data());}
#define TRACE_WARNING(...)  {printf("[WARNING] %s", SFBDCtlLibTrace::format(__VA_ARGS__).data());}
#define TRACE_INFO(...)     {printf("[INFO] %s", SFBDCtlLibTrace::format(__VA_ARGS__).data());}
#define TRACE_NOISE(...)    {printf("[NOISE] %s", SFBDCtlLibTrace::format(__VA_ARGS__).data());}
#endif

#include <string>
#include <vector>
#include <atomic>
#include "../../inc/BlockStoreCommon.h"
using namespace std;

class SFBDCtlLibTrace
{
private:
    static string tracePrefix_;
public:

    static void SetTracePrefix(string val) { tracePrefix_ = "[" + val + "]"; }
    template <class T>
    static string format(const T *fmt, ...);

#ifdef ETL_TRACING_ENABLED
    static void TraceError(string expr);
    static void TraceWarning(string expr);
    static void TraceInfo(string expr);
    static void TraceNoise(string expr);
#ifdef ETL_TRACE_TO_FILE_ENABLED
    static atomic<bool> fInitialized_;
    static void TraceInit();
#endif
#endif
};