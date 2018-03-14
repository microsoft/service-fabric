// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LeaseWrapper
{
    class LeaseAgentEventSource
    {
    public:
        Common::TraceEventWriter<std::wstring, std::wstring, uint64> TTL;

        LeaseAgentEventSource() :
            TTL(Common::TraceTaskCodes::LeaseAgent, 10, "TTL", Common::LogLevel::Info, "{0}@{1} lease valid for {2}ms", "app", "addr", "ms")
        {
        }

        static Common::Global<LeaseAgentEventSource> Events;
    };

}
