// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReplicaProxyStates
        {
            enum Enum
            {
                Ready = 0,
                InBuild = 1,
                InDrop = 2,
                Dropped = 3,

                LastValidEnum = Dropped
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(ReplicaProxyStates);
        }
    }
}
