// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Node
        {
            namespace ServiceTypeUpdateResult
            {
                enum Enum
                {
                    Processed = 0,
                    Stale = 1,
                    RANotOpen = 2,
                    LastValidEnum = RANotOpen
                };

                void WriteToTextWriter(Common::TextWriter & w, Enum const& value);

                DECLARE_ENUM_STRUCTURED_TRACE(ServiceTypeUpdateResult);
            }
        }
    }
}
