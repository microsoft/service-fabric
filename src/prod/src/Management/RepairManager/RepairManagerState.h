// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        namespace RepairManagerState
        {
            enum Enum
            {
                Unknown = 0,
                Stopped = 1,
                Starting = 2,
                Started = 3,
                Stopping = 4,
                StoppingPendingRestart = 5,
                Closed = 6,

                FirstValidEnum = Unknown,
                LastValidEnum = Closed
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            DECLARE_ENUM_STRUCTURED_TRACE( RepairManagerState )
        };
    }
}
