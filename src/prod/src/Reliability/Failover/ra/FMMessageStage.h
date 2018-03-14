// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace FMMessageStage
        {
            enum Enum
            {
                None,

                ReplicaDown,

                ReplicaUp,

                ReplicaDropped,

                EndpointAvailable,

                ReplicaUpload,

                LastValidEnum = ReplicaUpload
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            DECLARE_ENUM_STRUCTURED_TRACE(FailoverUnitFMMessageStage);
        }
    }
}
