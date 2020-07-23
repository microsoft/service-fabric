// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace TracePoints
{
    // Ensure the following is plugged into your config:
    // <parameter name="Federation.Tracing.TraceSessions" value="TracePoints {019DAA0F-E775-471A-AA85-49363C18E179} 0xFFFF" />
    class TracePointTracer
    {
    public:
        explicit TracePointTracer(ProviderMapHandle providerMapHandle)
        {
             eventProviderHandle_ = TracePointsGetEventProvider(providerMapHandle);
        }

        void Trace(std::wstring const & message, UCHAR traceLevel) const
        {
            TracePointsEventWriteString(eventProviderHandle_, traceLevel, 0, message.c_str());
        }

        void Trace(std::wstring const & message, DWORD value, UCHAR traceLevel) const
        {
            std::wostringstream stringStream;
            stringStream << message.c_str() << value;
            Trace(stringStream.str(), traceLevel);
        }

    private:
        EventProviderHandle eventProviderHandle_;
    };
}
