// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReplicaStates
        {
            enum Enum
            {
                InCreate = 0,
                InBuild = 1,
                Ready = 2,
                InDrop = 3,
                Dropped = 4,
                StandBy = 5,

                LastValidEnum = StandBy
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            Reliability::ReplicaStates::Enum ToReplicaDescriptionState(ReplicaStates::Enum state);
            ReplicaStates::Enum FromReplicaDescriptionState(Reliability::ReplicaStates::Enum state);  

            DECLARE_ENUM_STRUCTURED_TRACE(ReplicaStates);
        }
    }
}
