// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace FailoverUnitProxyStates
        {
            // States of a FailoverUnit within RA
            enum Enum
            {
                Opening = 0,
                Opened = 1,
                Closing = 2,
                Closed = 3,

                LastValidEnum = Closed
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(FailoverUnitProxyStates);
        }
    }
}
