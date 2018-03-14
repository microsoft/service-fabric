// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Communication
        {
            namespace MessageTarget
            {
                enum Enum
                {
                    // Message intended for RA. Example: DeactivateNode
                    RA,

                    // Message intended for FT. Example: AddPrimary
                    FT,
                };
            }
        }
    }
}
