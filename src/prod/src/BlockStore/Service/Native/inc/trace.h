// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <boost/locale.hpp>
#include "common.h"
#include "../../../inc/BlockStoreCommon.h"
extern std::string g_userServiceTypeName;

#ifndef ETL_TRACING_ENABLED // Mock Cluster/Native Mode
#include <fstream>
#include <chrono>
#include <ctime>

class tracer
{
public :
    tracer()
    {
        _enabled = false;
    }

    void init(const char *service_name, const char *filename)
    {
        _service_name = service_name;
        _file_name = filename;

#ifdef _DEBUG
        // TRACE always on in debug
        _enabled = true;
#else
        if (std::getenv("ENABLE_TRACE"))
            _enabled = true;
        else
            _enabled = false;
#endif
        _stream = std::make_unique<ofstream>();
        _stream->open(_file_name);
    }

    bool enabled() { return _enabled; }

    mutex& mutex() { return _mutex; }

    ofstream *stream() { return _stream.get(); }

    const char *service_name() { return _service_name.c_str(); }

    std::string system_clock_to_string(std::chrono::system_clock::time_point tp)
    {
        std::time_t tt = std::chrono::system_clock::to_time_t(tp);
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&tt), "%c");
        return ss.str();
    }

private :
    string _service_name;
    string _file_name;
    bool _enabled;
    std::mutex _mutex;
    unique_ptr<ofstream> _stream;
};

extern tracer g_tracer;

    #define TRACE_INIT(service_name, file_name) g_tracer.init(service_name, file_name);

    // No need to flush - endl automatically flushes
    #define _LOG(expr) { std::lock_guard<mutex> lock(g_tracer.mutex()); \
        cout << g_tracer.service_name() << "[" << g_tracer.system_clock_to_string(std::chrono::system_clock::now()) << "] " << "[T:" << boost::this_thread::get_id() << "] " << g_userServiceTypeName << ": " << expr << endl;  \
        *g_tracer.stream() << g_tracer.service_name() << " [" << g_tracer.system_clock_to_string(std::chrono::system_clock::now()) << "] " << "[T:" << boost::this_thread::get_id() << "] " << expr << endl;  \
    }

    // Default on
    #define TRACE_ERROR(expr) { _LOG("[ERROR] " << expr); }
    #define TRACE_WARNING(expr) { _LOG("[WARNING] " << expr); }
    #define TRACE_INFO(expr) { _LOG("[INFO] " << expr); }

    // Default off
    // TODO: Change to 0 before merging
    #define TRACE_NOISE(expr) if (1) { _LOG("[NOISE] " << expr); }

#else   // Real Cluster Mode
    // Reference code for LogLevel 
    // namespace LogLevel
    // {
    //     enum Enum
    //     {
    //         Silent = 0,
    //         Critical = 1,
    //         Error = 2,
    //         Warning = 3,
    //         Info = 4,
    //         Noise = 5,
    // 
    //         All = 99
    //     };
    // }

#ifdef ETL_TRACE_TO_FILE_ENABLED
    #define TRACE_INIT(service_name, file_name)                                                                                           \
    {                                                                                                                                     \
        ComPointer<IFabricNodeContextResult> nodeContext;                                                                                 \
        ComPointer<IFabricCodePackageActivationContext> activationContext;                                                                \
        ASSERT_IFNOT(                                                                                                                     \
            ::FabricGetNodeContext(nodeContext.VoidInitializationAddress()) == S_OK,                                                      \
            "Failed to create fabric runtime");                                                                                           \
                                                                                                                                          \
        ASSERT_IFNOT(                                                                                                                     \
            ::FabricGetActivationContext(IID_IFabricCodePackageActivationContext, activationContext.VoidInitializationAddress()) == S_OK, \
            "Failed to get activation context in EnableTracing");                                                                         \
        FABRIC_NODE_CONTEXT const * nodeContextResult = nodeContext->get_NodeContext();                                                   \
                                                                                                                                          \
        TraceProvider::GetSingleton()->SetDefaultLevel(TraceSinkType::ETW, LogLevel::Info);                                               \
                                                                                                                                          \
        wstring traceLog = wformatString("Node Name = {0}, IP = {1}", nodeContextResult->NodeName, nodeContextResult->IPAddressOrFQDN);   \
        TRACE_INFO(string(traceLog.begin(), traceLog.end()));                                                                             \
    }
#else
    #define TRACE_INIT(service_name, file_name)
#endif

    #define TRACE_COMPONENT StringLiteral("SFBlockStoreService")
    #define TRACE_ERROR(expr) { std::ostringstream traceStream; \
        traceStream << g_userServiceTypeName << ":" << expr << endl; \
        Trace.WriteError(TRACE_COMPONENT, "{0}", traceStream.str()); \
    }
    #define TRACE_WARNING(expr) { std::ostringstream traceStream; \
        traceStream << g_userServiceTypeName << ":" << expr << endl; \
        Trace.WriteWarning(TRACE_COMPONENT, "{0}", traceStream.str()); \
    }
    #define TRACE_INFO(expr) { std::ostringstream traceStream; \
        traceStream << g_userServiceTypeName << ":" << expr << endl; \
        Trace.WriteInfo(TRACE_COMPONENT, "{0}", traceStream.str()); \
    }
    #define TRACE_NOISE(expr) { std::ostringstream traceStream; \
        traceStream << g_userServiceTypeName << ":" << expr << endl; \
        Trace.WriteNoise(TRACE_COMPONENT, "{0}", traceStream.str()); \
    }
#endif
