// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace TracePoints
{
    class TracePointPipeClient
    {
        DENY_COPY(TracePointPipeClient);
    public:
        TracePointPipeClient(std::wstring const & pipeName, TracePointTracer const& tracer);

        void WriteString(std::wstring const & message);

        std::wstring ReadString();

        ~TracePointPipeClient();

    private:
        std::wstring pipeName_;
        HANDLE pipe_;
        const TracePointTracer & tracer_;
    };
}
