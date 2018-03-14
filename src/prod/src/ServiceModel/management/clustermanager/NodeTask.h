// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        namespace NodeTask
        {
            enum Enum
            {
                Invalid = 0,
                Restart = 1,
                Relocate = 2,
                Remove = 3,
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            DECLARE_ENUM_STRUCTURED_TRACE( NodeTask )

            Enum FromPublicApi(FABRIC_NODE_TASK_TYPE);
            FABRIC_NODE_TASK_TYPE ToPublicApi(Enum const);
        };
    }
}
