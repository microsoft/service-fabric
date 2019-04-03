// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#ifdef ETL_TRACING_ENABLED
#include "../../../../Common/Common.h"
#include "FabricRuntime_.h"
#define TRACE_ERROR(exp)    {std::ostringstream oss; oss << exp; SFBDCtlLibTrace::TraceError(oss.str());}
#define TRACE_WARNING(exp)  {std::ostringstream oss; oss << exp; SFBDCtlLibTrace::TraceWarning(oss.str());}
#define TRACE_INFO(exp)     {std::ostringstream oss; oss << exp; SFBDCtlLibTrace::TraceInfo(oss.str());}
#define TRACE_NOISE(exp)    {std::ostringstream oss; oss << exp; SFBDCtlLibTrace::TraceNoise(oss.str());}
#else
#define TRACE_ERROR(exp)    { std::cout << "[ERROR] " << exp;}
#define TRACE_WARNING(exp)  { std::cout << "[WARNING] " << exp;}
#define TRACE_INFO(exp)     { std::cout << "[INFO] " << exp;}
#define TRACE_NOISE(exp)    { std::cout << "[NOISE] " << exp;}
#endif

#include <string>
#include <vector>
#include <atomic>
//#include "../../inc/BlockStoreCommon.h"
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
