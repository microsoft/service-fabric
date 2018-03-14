// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace FaultAnalysisService
    {
        namespace DataLossMode
        {
            enum Enum
            {
                Invalid = 0,
                Partial = 1,
                Full = 2,
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            Common::ErrorCode FromPublicApi(FABRIC_DATA_LOSS_MODE const & publicVal, __out Enum & val);
            FABRIC_DATA_LOSS_MODE ToPublicApi(Enum const & val);
        };
    }
}
