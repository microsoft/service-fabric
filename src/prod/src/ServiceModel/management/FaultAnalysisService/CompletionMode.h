// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace FaultAnalysisService
    {
        namespace CompletionMode
        {
            enum Enum
            {
                None = 0,
                DoNotVerify = 1,
                Verify = 2,
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            Enum FromPublicApi(FABRIC_COMPLETION_MODE const &);
            FABRIC_COMPLETION_MODE ToPublicApi(Enum const &);
        };
    }
}
