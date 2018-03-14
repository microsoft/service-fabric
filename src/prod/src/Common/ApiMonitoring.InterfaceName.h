// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    namespace ApiMonitoring
    {
        namespace InterfaceName
        {
            enum Enum
            {
                Invalid = 0,
                IStatefulServiceReplica = 1,
                IStatelessServiceInstance = 2,
                IStateProvider = 3,
                IReplicator = 4,

                LastValidEnum = IReplicator
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            DECLARE_ENUM_STRUCTURED_TRACE(InterfaceName);
        }
    }
}
