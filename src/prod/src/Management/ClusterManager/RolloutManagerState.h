// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        namespace RolloutManagerState
        {
            enum Enum
            {
                Unknown = 0,
                NotActive = 1,
                Recovery = 2,
                Active = 3,
                Closed = 4,

                FirstValidEnum = Unknown,
                LastValidEnum = Closed
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            DECLARE_ENUM_STRUCTURED_TRACE( RolloutManagerState )
        };
    }
}
