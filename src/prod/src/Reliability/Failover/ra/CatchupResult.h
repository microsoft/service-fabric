// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        // Represents the message stage on an RAP replica
        namespace CatchupResult
        {
            enum Enum
            {
                NotStarted = 0,
                DataLossReported = 1,
                CatchupCompleted = 2,

                LastValidEnum = CatchupCompleted
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(CatchupResult);
        }
    }
}
