// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Hosting
        {
            namespace TerminateServiceHostReason
            {
                enum Enum
                {
                    RemoveReplica = 0,
                    FabricUpgrade,
                    NodeDeactivation,
                    ApplicationUpgrade,
                    NodeShutdown,

                    LastValidEnum = NodeShutdown
                };

                void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

                DECLARE_ENUM_STRUCTURED_TRACE(TerminateServiceHostReason);
            }
        }
    }
}


