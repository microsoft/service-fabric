// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Health
        {
            namespace ReplicaHealthEvent
            {
                enum Enum
                {
                    OK = 0,
                    Restart = 1,
                    Close = 2,
                    OpenWarning = 3,
                    ClearOpenWarning = 4,
                    CloseWarning = 5,
                    ClearCloseWarning = 6,
                    Error = 7,
                    ClearError = 8,
                    ServiceTypeRegistrationWarning = 9,
                    ClearServiceTypeRegistrationWarning = 10,
                    ReconfigurationStuckWarning = 11,
                    ClearReconfigurationStuckWarning = 12,
                    LastValidEnum = ClearReconfigurationStuckWarning
                };

                void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

                DECLARE_ENUM_STRUCTURED_TRACE(ReplicaHealthEvent);
            }
        }
    }
}
