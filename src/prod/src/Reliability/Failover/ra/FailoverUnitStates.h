// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace FailoverUnitStates
        {
            // States of a FailoverUnit within RA
            enum Enum
            {
                Open = 0,
                Closed = 1,

                LastValidEnum = Closed
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(FailoverUnitStates);
        }
    }
}
