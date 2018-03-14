// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace FailoverUnitLifeCycleTraceState
        {
            enum Enum
            {
                Closed = 0,

                Opening = 1,

                Open = 2,

                Available = 3,

                Closing = 4,

                Down = 5,

                LastValidEnum = Down
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            DECLARE_ENUM_STRUCTURED_TRACE(FailoverUnitLifeCycleTraceState);
        }
    }
}
