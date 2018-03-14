// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "../../Transport/TransportConfig.h"
#include "Tracer.h"

using namespace Common;
using namespace Transport;

namespace Threadpool {

    StringLiteral TraceType = "threadpool";

    int GetThreadpoolThrottle()
    {
        TransportConfig const & config = TransportConfig::GetConfig();
        return static_cast<int>(config.ThreadThrottle);
    }

    void TraceThreadpoolMsg(int level, const string &msg)
    {
        switch(level)
        {
            case TraceLevel::Error:
                Trace.WriteError(TraceType, "{0}", msg);
                break;
            case TraceLevel::Warning:
                Trace.WriteWarning(TraceType, "{0}", msg);
                break;
            case TraceLevel::Info:
                Trace.WriteInfo(TraceType, "{0}", msg);
                break;
            case TraceLevel::Debug:
                Trace.WriteNoise(TraceType, "{0}", msg);
                break;
        }
    }

    void ThreadpoolAssert(const char* msg)
    {
          StringLiteral fmt(msg, msg + strlen(msg));
          Common::Assert::CodingError(fmt);
    }
}

