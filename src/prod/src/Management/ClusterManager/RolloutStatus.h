// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        namespace RolloutStatus
        {
            enum Enum
            {
                // Indicates that the status needs to be refreshed
                // from disk before we can take correct action
                //
                Unknown = 0,

                Pending = 1,
                DeletePending = 2,
                Failed = 3,
                Completed = 4,
                Replacing = 5
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
        };
    }
}
