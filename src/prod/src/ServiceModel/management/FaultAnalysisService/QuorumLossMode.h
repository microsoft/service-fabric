// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace FaultAnalysisService
    {
        namespace QuorumLossMode
        {
            enum Enum
            {
                Invalid = 0,
                QuorumReplicas = 1,
                AllReplicas = 2,
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            Common::ErrorCode FromPublicApi(FABRIC_QUORUM_LOSS_MODE const & publicMode, __out Enum & val);
            FABRIC_QUORUM_LOSS_MODE ToPublicApi(Enum const & val);
        };
    }
}
