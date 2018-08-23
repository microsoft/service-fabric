// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "Common/TraceEventWriter.h"
#include "Common/TextWriter.h"

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Diagnostics
        {
            namespace TraceEventType
            {
                enum Enum
                {
                    ReconfigurationSlow = 0,
                    ReconfigurationComplete,
                    ResourceUsageReport,
                    ReplicaStateChange,
                    LastValidEnum = ReplicaStateChange
                };

                void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

                DECLARE_ENUM_STRUCTURED_TRACE(TraceEventType);
            }
        }
    }
}
