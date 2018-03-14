// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        // Represents the message stage on an RA replica
        namespace ReplicaMessageStage
        {
            enum Enum
            {
                None = 0,
                RAProxyReplyPending = 1,
                RAReplyPending = 2,

                LastValidEnum = RAReplyPending
            };

            DECLARE_ENUM_STRUCTURED_TRACE(ReplicaMessageStage);

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
        }
    }
}
