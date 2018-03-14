// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace RAReplicaOpenMode
        {
            enum Enum
            {
                None = 0,

                // Open of new replica with CR
                Open = 1,

                // Reopen of existing persisted replica
                Reopen = 2,

                // CR of existing persisted replica
                ChangeRole = 3,

                LastValidEnum = ChangeRole
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            DECLARE_ENUM_STRUCTURED_TRACE(RAReplicaOpenMode);
        }
    }
}


