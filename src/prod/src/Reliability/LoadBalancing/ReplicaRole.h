// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        namespace ReplicaRole
        {
            enum Enum
            {
                None = 1024, // the replica is down
                Primary = 1025, // the replica is future primary
                Secondary = 1026, // the replica is future secondary or stateless instance
                StandBy = 1027, // the replica is standby (stateful only)
                Dropped= 1028, // only for upgrade
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
            std::wstring ToString(Enum const & val);
        }
    }
}
