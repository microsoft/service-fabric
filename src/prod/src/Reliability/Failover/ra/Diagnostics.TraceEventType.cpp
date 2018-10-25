// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"
#include "Diagnostics.TraceEventType.h"

using namespace Common;
using namespace Reliability::ReconfigurationAgentComponent;

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Diagnostics
        {
            namespace TraceEventType
            {
                void TraceEventType::WriteToTextWriter(TextWriter & w, Enum const & val)
                {
                    switch (val)
                    {
                    case ReconfigurationSlow:
                        w << "Reconfiguration Slow"; return;
                    case ReconfigurationComplete:
                        w << L"Reconfiguration Complete"; return;
                    case ResourceUsageReport:
                        w << L"ResourceUsageReport"; return;
                    case ReplicaStateChange:
                        w << L"ReplicaStateChange"; return;
                    default:
                        Common::Assert::CodingError("Unknown Trace Event Type {0}", static_cast<int>(val));
                    }
                }

                ENUM_STRUCTURED_TRACE(TraceEventType, ReconfigurationSlow, LastValidEnum);
            }
        }
    }
}
